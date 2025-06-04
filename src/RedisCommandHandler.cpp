#include"../include/RedisCommandHandler.h"
#include"../include/RedisDatabase.h"

// RESP Parser
// *2\r\n$4\r\n\PING\r\n$4\r\nTEST\r\n
// *2 array has two elements 
// $4 next string has 4 elements
// PING
// TEST

std::vector<std::string> parseRespCommand(const std::string &input){
    std::vector<std::string> tokens;
    if(input.empty()) return tokens;

    // if it doesnt start with * fallbackk to splitting with whitespace
    if(input[0]!='*'){
        std::istringstream iss(input);
        std::string token;
        while(iss>>token){
            tokens.push_back(token);
        }
        return tokens;
    }

    size_t pos = 0;
    //expect * followed by number of elements 
    if(input[pos]!='*') return tokens;
    pos++;

    // carriage return , line feed
    size_t crlf = input.find("\r\n",pos);
    if(crlf==std::string::npos) return tokens;

    int num_elements = std::stoi(input.substr(pos,crlf-pos));
    pos = crlf+2;

    for(int i=0;i<num_elements;i++){
        if(pos>=input.size() or input[pos]!='$') break; //format error
        pos++; //skip `$`
        crlf = input.find("\r\n",pos);
        if(crlf==std::string::npos) break;
        int len = std::stoi(input.substr(pos,crlf-pos));
        pos = crlf+2;

        if(pos+len>input.size()) break;
        std::string token = input.substr(pos,len);
        tokens.push_back(token);
        pos+=len+2; //skips token and crlf
    }
    return tokens;
}

RedisCommandHandler::RedisCommandHandler(){} 

std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    //use RESP parser
    auto tokens = parseRespCommand(commandLine);
    if(tokens.empty()) return "-Error:Empty command\r\n";
    // for(auto token:tokens) std::cout<<token<<"\n";
    // std::cout<<commandLine<<"\n";
    std::string cmd = tokens[0];
    std::transform(cmd.begin(),cmd.end(), cmd.begin(),::toupper);
    std::ostringstream response;

    //connect to db
    RedisDatabase& db = RedisDatabase::getInstance();
    //check commands

    if(cmd=="PING"){
        response<<"+PONG\r\n";
    }else if(cmd=="ECHO"){
        //...
    }else{
        response<<"-Error invalid command\r\n";
    }
    return response.str();
}