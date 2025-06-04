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
        if(tokens.size()<2) response<<"-Error: Echo requires a message\r\n";
        else response<<"+"<<tokens[1]<<"\r\n";
    }else if(cmd=="FLUSHALL"){
        db.flushAll();
        response<<"+OK\r\n";
    }else if(cmd=="SET"){
        if(tokens.size()<3) response<<"-Error: SET requires key and value\r\n";
        else{
            db.set(tokens[1],tokens[2]);
            response<<"+OK\r\n";
        }
    }else if(cmd=="GET"){
        if(tokens.size()<2) response<<"-Error: Get requires key\r\n";
        else{
            std::string val;
            if(!db.get(tokens[1],val)){
                response<<"$-1\r\n";
            }else response<<"$"<<val.size()<<"\r\n"<<val<<"\r\n";
        }
    }else if(cmd=="KEYS"){
        std::vector<std::string> allKeys = db.keys();
        response<<"*"<<allKeys.size()<<"\r\n";
        for(auto &key:allKeys){
            response<<"$"<<key.size()<<"\r\n"<<key<<"\r\n";
        }
    }else if(cmd=="TYPE"){
        if(tokens.size()<2) response<<"-Error: TYPE requires key\r\n";
        else response<<"+"<<db.type(tokens[1])<<"\r\n";
    }else if(cmd=="DEL"){
        if(tokens.size()<2){
            response<<"-Error: DEL requires key\r\n";
        }else{
            bool res = db.del(tokens[1]);
            response<<":"<<(res?1:0)<<"\r\n";
        }
    }else if(cmd=="EXPIRE"){
        if(tokens.size()<3){
            response<<"-Error: EXPIRE required KEY and TIME in seconds\r\n";
        }else{
            if(db.expire(tokens[1],std::stoi(tokens[2]))) response<<"+OK\r\n";
            // else response<<"-"
        }
    }else if(cmd=="RENAME"){
        if(tokens.size()<3){
            response<<"-Error: EXPIRE required OLD_KEY and NEW_KEY\r\n";
        }else{
            db.rename(tokens[1],tokens[2]);
            response<<"+OK\r\n";
        }
    }
    else{
        response<<"-Error invalid command\r\n";
    }
    return response.str();
}