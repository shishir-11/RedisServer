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
    return true;
}

bool RedisDatabase::load(const std::string &filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename,std::ios::binary);
    if(!ifs) return false;
    kv_store.clear();
    list_store.clear();
    hash_store.clear();

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
    auto it = kv_store.find(key);
    if(it!=kv_store.end()){
        value = it->second;
        return true;
    }
    return false;

}
std::vector<std::string> RedisDatabase::keys(){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> res;
    for(const auto &pair:kv_store) res.push_back(pair.first);
    for(const auto &pair:list_store) res.push_back(pair.first);
    for(const auto &pair:hash_store) res.push_back(pair.first);
    return res;
}
std::string RedisDatabase:: type(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);
    if(kv_store.find(key)!=kv_store.end()){
        return "string";
    }
    if(list_store.find(key)!=list_store.end()) return "list";
    if(hash_store.find(key)!=hash_store.end()) return "hash";
    else return "none";
}

bool RedisDatabase::del(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
    erased|=kv_store.erase(key) > 0;
    erased|=list_store.erase(key)>0;
    erased|= hash_store.erase(key)>0;
    return erased;
}

bool RedisDatabase::expire(const std::string& key, int seconds){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool exist = (kv_store.find(key)!=kv_store.end())||(list_store.find(key)!=list_store.end())||
                 (hash_store.find(key)!=hash_store.end());

    if(!exist) return false;
    expiry_store[key] = std::chrono::system_clock::now() + std::chrono::seconds(seconds);
    return true;
}

bool RedisDatabase::rename(const std::string &oldKey, const std::string &newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
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
    auto it = list_store.find(key);
    if (it != list_store.end()) {
        return it->second; 
    }
    return {}; 
}

ssize_t RedisDatabase::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end()) 
        return it->second.size();
    return 0;
}

void RedisDatabase::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].insert(list_store[key].begin(), value);
}

void RedisDatabase::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    list_store[key].push_back(value);
}

bool RedisDatabase::lpop(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = list_store.find(key);
    if (it != list_store.end() && !it->second.empty()) {
        value = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    }
    return false;
}
