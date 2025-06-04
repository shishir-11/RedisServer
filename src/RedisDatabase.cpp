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
    kv_store.clear();
    hash_store.clear();
    list_store.clear();
}