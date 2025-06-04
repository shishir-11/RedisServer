#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

static RedisServer* globalServer = nullptr;

// void RedisServer::setupSignalHandler
void signalHandler(int signum){
    if(globalServer){
        std::cout<<"Shutting down...\nSignal caught "<<signum<<"\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler(){
    signal(SIGINT,signalHandler);

}

RedisServer::RedisServer(int port): port{port}, server_socket{-1}, running{true}{
    globalServer = this;
}

void RedisServer::shutdown(){
    running = false;
    if(server_socket!=-1){
        close(server_socket);
    }
    std::cout<<"Server Shutdown\n";
}

void RedisServer::run(){
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0){
        std::cerr<<"Error creating server socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_socket,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
        std::cerr<<"Error Binding Server Socket\n";
        return;
    }

    // if(listen(server_socket,SOMAXCONN))
    if(listen(server_socket,10)<0){
        std::cerr<<"Error listening on Server Socket\n";
        return;
    }

    std::cout<<"Redis Server Listening On Port"<<port<<"\n";

    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    while(running){
        int client_socket = accept(server_socket,nullptr,nullptr);
        if(client_socket<0){
            if(running){
                std::cerr<<"Error accepting client connection\n";
            }
            break;
        }

        threads.emplace_back([client_socket,&cmdHandler](){
            char buffer[1024];
            while(true){
                memset(buffer,0,sizeof(buffer));
                int bytes = recv(client_socket,buffer,sizeof(buffer)-1,0);
                if(bytes<=0) break;
                std::string request(buffer,bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket,response.c_str(),response.size(),0);
            }
            close(client_socket);
        });

        for(auto& t:threads){
            if(t.joinable()) t.join();
        }

        if(RedisDatabase::getInstance().dump("dump.my_rdb")){
            std::cout<<"Database Dumped to dump.my_rdb\n";
        }else{
            std::cerr<<"Error dumping Database\\n";
        }
    }
}
