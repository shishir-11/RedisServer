#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <string>
#include <atomic>
#include<iostream>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<vector>
#include<thread>
#include<cstring>
#include<signal.h>

class RedisServer{
public:
    RedisServer(int port);
    void run();
    void shutdown();
private:
    int port;
    int server_socket;
    std::atomic<bool> running;

    void setupSignalHandler(); 
};

#endif