#include"../include/RedisServer.h"
#include<iostream>
#include<thread>
#include<chrono>

int main(int argc, char *argv[]){
    int port = 8000;
    if(argc>=2) port = std::stoi(argv[1]);
    RedisServer server(port);
    std::thread persistance([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(180));
            // dump the database
        }
    });
    persistance.detach();
    server.run();
    return 0;
}