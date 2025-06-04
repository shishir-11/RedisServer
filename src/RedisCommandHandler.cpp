#include"../include/RedisCommandHandler.h"
#include"../include/RedisDatabase.h"

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

static std::string handleLget(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-Error: LGET requires a key\r\n";

    auto elems = db.lget(tokens[1]);
    std::ostringstream oss;
    oss << "*" << elems.size() << "\r\n";
    for (const auto& e : elems) {
        oss << "$" << e.size() << "\r\n"
            << e << "\r\n";
    }
    return oss.str();
}

static std::string handleLlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: LLEN requires key\r\n";
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleLpush(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: LPUSH requires key and value\r\n";
    for (size_t i = 2; i < tokens.size(); ++i) {
        db.lpush(tokens[1], tokens[i]);
    }
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleRpush(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: RPUSH requires key and value\r\n";
    for (size_t i = 2; i < tokens.size(); ++i) {
        db.rpush(tokens[1], tokens[i]);
    }    
    ssize_t len = db.llen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleLpop(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: LPOP requires key\r\n";
    std::string val;
    if (db.lpop(tokens[1], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string handleRpop(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: RPOP requires key\r\n";
    std::string val;
    if (db.rpop(tokens[1], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string handleLrem(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) 
        return "-Error: LREM requires key, count and value\r\n";
    try {
        int count = std::stoi(tokens[2]);
        int removed = db.lrem(tokens[1], count, tokens[3]);
        return ":" +std::to_string(removed) + "\r\n";
    } catch (const std::exception&) {
        return "-Error: Invalid count\r\n";
    }
}

static std::string handleLindex(const std::vector<std::string>&tokens, RedisDatabase&db){
    if(tokens.size()<3){
        return "-Error: LINDEX requires KEY and INDEX";
    }
    try{
        int index = std::stoi(tokens[2]);
        std::string value;
        if(db.lindex(tokens[1],index,value)){
            return "$"+std::to_string(value.size())+"\r\n"+value+"\r\n";
        }else return "$-1\r\n";
    } catch (const std::exception&) {
        return "-Error: Invalid index\r\n";
    }
}

static std::string handleLset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) 
        return "-Error: LEST requires key, index and value\r\n";
    try {
        int index = std::stoi(tokens[2]);
        if (db.lset(tokens[1], index, tokens[3]))
            return "+OK\r\n";
        else 
            return "-Error: Index out of range\r\n";
    } catch (const std::exception&) {
        return "-Error: Invalid index\r\n";
    }
}

static std::string handleHset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) 
        return "-Error: HSET requires key, field and value\r\n";
    db.hset(tokens[1], tokens[2], tokens[3]);
    return ":1\r\n";
}

static std::string handleHget(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: HSET requires key and field\r\n";
    std::string value;
    if (db.hget(tokens[1], tokens[2], value))
        return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    return "$-1\r\n";
}

static std::string handleHexists(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: HEXISTS requires key and field\r\n";
    bool exists = db.hexists(tokens[1], tokens[2]);
    return ":" + std::to_string(exists ? 1 : 0) + "\r\n";
}

static std::string handleHdel(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: HDEL requires key and field\r\n";
    bool res = db.hdel(tokens[1], tokens[2]);
    return ":" + std::to_string(res ? 1 : 0) + "\r\n";
}

static std::string handleHgetall (const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HGETALL requires key\r\n";
    auto hash = db.hgetall(tokens[1]);
    std::ostringstream oss;
    oss << "*" << hash.size() * 2 << "\r\n";
    for (const auto& pair: hash) {
        oss << "$" << pair.first.size() << "\r\n" << pair.first << "\r\n";
        oss << "$" << pair.second.size() << "\r\n" << pair.second << "\r\n";
    }
    return oss.str();
}

static std::string handleHkeys(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HKEYS requires key\r\n";
    auto keys = db.hkeys(tokens[1]);
    std::ostringstream oss;
    oss << "*" << keys.size() << "\r\n";
    for (const auto& key: keys) {
        oss << "$" << key.size() << "\r\n" << key << "\r\n";
    }
    return oss.str();
}

static std::string handleHvals(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HVALS requires key\r\n";
    auto values = db.hvals(tokens[1]);
    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& val: values) {
        oss << "$" << val.size() << "\r\n" << val << "\r\n";
    }
    return oss.str();
}

static std::string handleHlen(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HLEN requires key\r\n";
    ssize_t len = db.hlen(tokens[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string handleHmset(const std::vector<std::string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4 || (tokens.size() % 2) == 1) 
        return "-Error: HMSET requires key followed by field value pairs\r\n";
    std::vector<std::pair<std::string, std::string>> fieldValues;
    for (size_t i = 2; i < tokens.size(); i += 2) {
        fieldValues.emplace_back(tokens[i], tokens[i+1]);
    }
    db.hmset(tokens[1], fieldValues);
    return "+OK\r\n";
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
    else if(cmd=="LLEN"){
        return handleLlen(tokens,db);
    }
    else if (cmd == "LPUSH")
        return handleLpush(tokens, db);
    else if (cmd == "RPUSH")
        return handleRpush(tokens, db);
    else if (cmd == "LPOP")
        return handleLpop(tokens, db);
    else if (cmd == "RPOP")
        return handleRpop(tokens, db);
    else if (cmd == "LREM")
        return handleLrem(tokens, db);
    else if (cmd == "LINDEX")
        return handleLindex(tokens, db);
    else if (cmd == "LSET")
        return handleLset(tokens, db);
    else if(cmd=="LGET")
        return handleLget(tokens,db);
    else if (cmd == "HSET") 
        return handleHset(tokens, db);
    else if (cmd == "HGET") 
        return handleHget(tokens, db);
    else if (cmd == "HEXISTS") 
        return handleHexists(tokens, db);
    else if (cmd == "HDEL") 
        return handleHdel(tokens, db);
    else if (cmd == "HGETALL") 
        return handleHgetall(tokens, db);
    else if (cmd == "HKEYS") 
        return handleHkeys(tokens, db);
    else if (cmd == "HVALS") 
        return handleHvals(tokens, db);
    else if (cmd == "HLEN") 
        return handleHlen(tokens, db);
    else if (cmd == "HMSET") 
        return handleHmset(tokens, db);
    else{
        response<<"-Error invalid command\r\n";
    }
    return response.str();
}

