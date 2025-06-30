# Introduction
In this project I try to make a custom implementation of a Redis Server that follows the RESP Protocol.
Redis is a in-memory key-value store that is often used for caching by web servers.
I have currently used the STL containters provided by C++ for most of the implementation but plan to add a custom hash table which also grows when keys increase and implement a custom timeout and extend to support sorted sets data structures.
**To build and run**
```bash
$ git clone https://github.com/shishir-11/RedisServer.git
$ cd RedisServer
$ make
$ ./bin/redis-server
```
This will get the redis-server up and running. 

## Components
Divided the work into 3 different divisions: `CommandHandler`, `Database`,`Server`
`Command Handler`: handles the parsing, logic and execution of the various commands supported by the server. 
`Database`: spins up a database instance in the main memory. Handles the read and write. Also creates a dump which is ocassionally updated and data is loaded from whenever starting again.
`Server`: provides the functionality to accept client connections and communicate over TCP port and maintain the run state and quit gracefully

# Commands Supported

## General Commands

| Command             | Description                            |
|---------------------|----------------------------------------|
| `PING`              | Check if the server is alive.          |
| `ECHO <message>`    | Return the given message.              |
| `INFO`              | Display server information.            |
| `FLUSHALL`          | Delete all keys in the database.       |

## String Commands

| Command                   | Description                                 |
|---------------------------|---------------------------------------------|
| `SET <key> <value>`       | Set a string value to a key.                |
| `GET <key>`               | Get the string value of a key.              |
| `DEL <key>`               | Delete the specified key.                   |
| `EXPIRE <key> <seconds>`  | Set a time-to-live (TTL) on a key.          |
| `RENAME <old> <new>`      | Rename a key.                               |
| `TYPE <key>`              | Get the data type of a key.                 |
| `KEYS`                    | List all keys in the database.              |

## List Commands

| Command                                | Description                                  |
|----------------------------------------|----------------------------------------------|
| `LLEN <key>`                           | Get the length of the list.                  |
| `LPUSH <key> <value> [value ...]`      | Prepend value(s) to the list.                |
| `RPUSH <key> <value> [value ...]`      | Append value(s) to the list.                 |
| `LPOP <key>`                           | Remove and return the first element.         |
| `RPOP <key>`                           | Remove and return the last element.          |
| `LREM <key> <count> <value>`           | Remove occurrences of value from the list.   |
| `LINDEX <key> <index>`                 | Get element at a specific index.             |
| `LSET <key> <index> <value>`           | Set list element at index.                   |
| `LGET <key>`                           | Get all elements of the list. *(custom)*     |

## Hash Commands

| Command                                            | Description                                      |
|----------------------------------------------------|--------------------------------------------------|
| `HSET <key> <field> <value>`                      | Set a field in a hash.                           |
| `HGET <key> <field>`                              | Get a field's value from a hash.                 |
| `HEXISTS <key> <field>`                           | Check if a field exists in a hash.               |
| `HDEL <key> <field>`                              | Delete a field from a hash.                      |
| `HGETALL <key>`                                   | Get all fields and values in a hash.             |
| `HKEYS <key>`                                     | Get all field names in a hash.                   |
| `HVALS <key>`                                     | Get all field values in a hash.                  |
| `HLEN <key>`                                      | Get number of fields in a hash.                  |
| `HMSET <key> <field1> <val1> ...`                 | Set multiple fields in a hash.                   |
