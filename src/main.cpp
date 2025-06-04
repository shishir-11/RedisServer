#include"../include/RedisServer.h"
#include "../include/RedisDatabase.h"
#include<iostream>
#include<thread>
#include<chrono>

int main(int argc, char *argv[]){
    int port = 8000;
    if(argc>=2) port = std::stoi(argv[1]);
    if(RedisDatabase::getInstance().load("dump.my_rdb")){
        std::cout<<"Database Loaded from dump.my_rdb\n";
    }else{
        std::cout<<"No dump found or load failed. Starting with empty db\n";
    }
    RedisServer server(port);
    std::thread persistance([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(180));
            // dump the database
            if(!RedisDatabase::getInstance().dump("dump.my_rdb")){
                std::cerr<<"Error Dumping Database\n";
            }else{
                std::cout<<"Database dumped to dump.my_rdb\n";
            }
        }
    });
    persistance.detach();
    server.run();
    return 0;
}