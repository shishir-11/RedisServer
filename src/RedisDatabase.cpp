#include "../include/RedisDatabase.h"

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::dump(const std::string &filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream ofs(filename, std::ios::binary);
    if(!ofs) return false;
    
    for(const auto &kv: kv_store){
        ofs<<"K "<<kv.first<<" "<<kv.second<<"\n";
    }
    for(const auto&kv: list_store){
        ofs<<"L "<<kv.first;
        for(const auto &ele: kv.second ) ofs<<" "<<ele;
        ofs<<"\n";
    }
    for(const auto &kv: hash_store){
        ofs<<"H "<<kv.first;
        for(const auto& field_val: kv.second) ofs<<" "<<field_val.first<<":"<<field_val.second;
        ofs<<"\n";
    }
    for(const auto&kv: expiry_store){
        ofs<<"T "<<kv.first<<std::chrono::system_clock::to_time_t(kv.second)<<"\n";
    }
    return true;
}

bool RedisDatabase::load(const std::string &filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename,std::ios::binary);
    if(!ifs) return false;
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    expiry_store.clear();
    std::string line;
    while(std::getline(ifs,line)){
        std::istringstream iss(line);
        char type;
        iss>>type;
        if(type=='K'){
            std::string key, value;
            iss>>key>>value;
            kv_store[key] = value;
        }else if(type=='L'){
            std::string key;
            iss>>key;
            std::string item;
            std::vector<std::string> list;
            while(iss>>item){
                list.push_back(item);
            }
            list_store[key] = list;
        }else if(type=='H'){
            std::string key;
            iss>>key;
            std::unordered_map<std::string,std::string> hash;
            std::string pair;
            while(iss>>pair){
                auto pos = pair.find(":");
                if(pos!=std::string::npos){
                    std::string field = pair.substr(0,pos);
                    std::string value = pair.substr(pos+1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }else if(type=='T'){
            std::string key;
            std::time_t t;
            iss>>key>>t;
            expiry_store[key] = std::chrono::system_clock::from_time_t(t);
        }
    }     
    return true;
}

bool RedisDatabase::flushAll(){
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    hash_store.clear();
    list_store.clear();
    return true;
}

void RedisDatabase::set(std::string key, std::string value){
    std::lock_guard<std::mutex> loc(db_mutex);
    kv_store[key] = value;
}
bool RedisDatabase::get(const std::string &key, std::string& value){
    std::lock_guard<std::mutex> loc(db_mutex);
    removeExpired();
    auto it = kv_store.find(key);
    if(it!=kv_store.end()){
        value = it->second;
        return true;
    }
    return false;

}
std::vector<std::string> RedisDatabase::keys(){
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    std::vector<std::string> res;
    for(const auto &pair:kv_store) res.push_back(pair.first);
    for(const auto &pair:list_store) res.push_back(pair.first);
    for(const auto &pair:hash_store) res.push_back(pair.first);
    return res;
}
std::string RedisDatabase:: type(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    if(kv_store.find(key)!=kv_store.end()){
        return "string";
    }
    if(list_store.find(key)!=list_store.end()) return "list";
    if(hash_store.find(key)!=hash_store.end()) return "hash";
    else return "none";
}

bool RedisDatabase::del(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    bool erased = false;
    erased|=kv_store.erase(key) > 0;
    erased|=list_store.erase(key)>0;
    erased|= hash_store.erase(key)>0;
    return erased;
}

bool RedisDatabase::expire(const std::string& key, int seconds){
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    bool exist = (kv_store.find(key)!=kv_store.end())||(list_store.find(key)!=list_store.end())||
                 (hash_store.find(key)!=hash_store.end());

    if(!exist) return false;
    expiry_store[key] = std::chrono::system_clock::now() + std::chrono::seconds(seconds);
    return true;
}

bool RedisDatabase::rename(const std::string &oldKey, const std::string &newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    bool found = false;

    auto itKv = kv_store.find(oldKey);
    if(itKv!=kv_store.end()){
        kv_store[newKey] = itKv->second;
        kv_store.erase(itKv);
        found = true;
    }

    auto litKv = list_store.find(oldKey);
    if(litKv!=list_store.end()){
        list_store[newKey] = litKv->second;
        list_store.erase(litKv);
        found = true;
    }

    auto htKv = hash_store.find(oldKey);
    if(htKv!=hash_store.end()){
        hash_store[newKey] = htKv->second;
        hash_store.erase(htKv);
        found = true;
    }

    auto itExp = expiry_store.find(oldKey);
    if(itExp!=expiry_store.end()){
        expiry_store[newKey] = itExp->second;
        expiry_store.erase(itExp);
        found = true;
    }
    return found;
}

std::vector<std::string> RedisDatabase::lget(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second; 
    }
    return {}; 
}

ssize_t RedisDatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it != list_store.end()) 
        return it->second.size();
    return 0;
}

void RedisDatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();
    list_store[key].insert(list_store[key].begin(), value);
}

void RedisDatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    list_store[key].push_back(value);
}

bool RedisDatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    }
    return false;
}

bool RedisDatabase::rpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.back();
        it->second.pop_back();
        return true;
    }
    return false;
}

bool RedisDatabase::lindex(const std::string& key, int index, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it == list_store.end()) 
        return false;

    const auto& lst = it->second;
    if (index < 0)
        index = lst.size() + index;
    if (index < 0 || index >= static_cast<int>(lst.size()))
        return false;
    
    value = lst[index];
    return true;
}

bool RedisDatabase::lset(const std::string& key, int index, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = list_store.find(key);
    if (it == list_store.end()) 
        return false;

    auto& lst = it->second;
    if (index < 0)
        index = lst.size() + index;
    if (index < 0 || index >= static_cast<int>(lst.size()))
        return false;
    
    lst[index] = value;
    return true;
}

int RedisDatabase::lrem(const std::string& key, int count, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    int removed = 0;
    auto it = list_store.find(key);
    if (it == list_store.end()) 
        return 0;

    auto& lst = it->second;

    if (count == 0) {
        // Remove all occurrences
        auto new_end = std::remove(lst.begin(), lst.end(), value);
        removed = std::distance(new_end, lst.end());
        lst.erase(new_end, lst.end());
    } else if (count > 0) {
        // Remove from head to tail
        for (auto iter = lst.begin(); iter != lst.end() && removed < count; ) {
            if (*iter == value) {
                iter = lst.erase(iter);
                ++removed;
            } else {
                ++iter;
            }
        }
    } else {
        // Remove from tail to head (count is negative)
        // First collect indices in reverse to avoid iterator invalidation
        std::vector<int> matching_indices;
        for (int i = lst.size() - 1; i >= 0; --i) {
            if (lst[i] == value) {
                matching_indices.push_back(i);
                if (++removed == -count) break;
            }
        }
        // Reset counter to actual number removed
        removed = matching_indices.size();
        for (int idx : matching_indices) {
            lst.erase(lst.begin() + idx);
        }
    }

    return removed;
}


bool RedisDatabase::hset(const std::string& key, const std::string& field, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    hash_store[key][field] = value;
    return true;
}

bool RedisDatabase::hget(const std::string& key, const std::string& field, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        auto f = it->second.find(field);
        if (f != it->second.end()) {
            value = f->second;
            return true;
        }
    }
    return false;
}

bool RedisDatabase::hexists(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = hash_store.find(key);
    if (it != hash_store.end())
        return it->second.find(field) != it->second.end();
    return false;
}

bool RedisDatabase::hdel(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = hash_store.find(key);
    if (it != hash_store.end())
        return it->second.erase(field) > 0;
    return false;
}

std::unordered_map<std::string, std::string> RedisDatabase::hgetall(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    if (hash_store.find(key) != hash_store.end())
        return hash_store[key];
    return {};
}

std::vector<std::string> RedisDatabase::hkeys(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    std::vector<std::string> fields;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair: it->second)
            fields.push_back(pair.first);
    }
    return fields;
}

std::vector<std::string> RedisDatabase::hvals(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    std::vector<std::string> values;
    auto it = hash_store.find(key);
    if (it != hash_store.end()) {
        for (const auto& pair: it->second)
            values.push_back(pair.second);
    }
    return values;
}

ssize_t RedisDatabase::hlen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    auto it = hash_store.find(key);
    return (it != hash_store.end()) ? it->second.size() : 0;
}

bool RedisDatabase::hmset(const std::string& key, const std::vector<std::pair<std::string, std::string>>& fieldValues) {
    std::lock_guard<std::mutex> lock(db_mutex);
    removeExpired();

    for (const auto& pair: fieldValues) {
        hash_store[key][pair.first] = pair.second;
    }
    return true;
}

void RedisDatabase::removeExpired() {
    auto now = std::chrono::system_clock::now();
    for (auto it = expiry_store.begin(); it != expiry_store.end(); ) {
        if (now > it->second) {
            // Remove from all stores
            kv_store.erase(it->first);
            list_store.erase(it->first);
            hash_store.erase(it->first);
            it = expiry_store.erase(it);
        } else {
            ++it;
        }
    }
}