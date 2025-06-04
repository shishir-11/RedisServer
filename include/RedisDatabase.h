#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include<string>

class RedisDatabase{
public:
    // get singleton instance
    static RedisDatabase& getInstance();

    // persistance: Dump/load the database from a file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);
     
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;
};

#endif