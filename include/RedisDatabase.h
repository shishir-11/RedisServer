#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include<string>
#include<mutex>
#include<vector>
#include<unordered_map>
#include<iostream>
#include<fstream>
#include<sstream>
#include<chrono>
class RedisDatabase{
public:
    // get singleton instance
    static RedisDatabase& getInstance();

    bool flushAll();

    void set(std::string key, std::string value);
    bool get(const std::string &key, std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string &key);
    bool del(const std::string &key);
    bool expire(const std::string& key, int seconds);
    bool rename(const std::string &oldKey, const std::string &newKey);
    
    // persistance: Dump/load the database from a file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

    std::mutex db_mutex;
    std::unordered_map<std::string,std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string,std::string>> hash_store;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry_store;
};

#endif