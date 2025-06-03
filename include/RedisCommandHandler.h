#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include<string>
#include<vector>
#include<sstream>
#include<algorithm>
#include<iostream>
class RedisCommandHandler{
public:
    RedisCommandHandler();
    std::string processCommand(const std::string& commandLine);
};

#endif