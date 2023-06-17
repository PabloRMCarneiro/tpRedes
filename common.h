#pragma once
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 1024

#define MAX_EQUIPMENT 15

enum COMMAND_TYPE
{   
    REQ_ADDPEER = 1,
    REQ_RMPEER = 2,
    REQ_ADDC2P = 3,
    REQ_REMCFP = 4,
    REQ_ADD = 5,
    REQ_REM = 6,
    RES_ADD = 7,
    RES_LIST = 8,
    REQ_INF = 9,
    RES_INF = 10,
    ERROR = 11,
    OK = 12,
};

enum ERROR
{
    EQUIPMENT_NOT_FOUND = 1,
    EQUIPMENT_SOURCE_NOT_FOUND = 2,
    EQUIPMENT_TARGET_NOT_FOUND = 3,
    EQUIPMENT_LIMIT_EXCEEDED = 4,
    PEER_LIMIT_EXCEEDED = 5,
    PEER_NOT_FOUND = 6,
};

const char *ERROR_MESSAGES[] = {"Equipment not found", "Source equipment not found", "Target equipment not found", "Equipment limit exceeded", "Peer limit exceeded", "Peer not found"};

const char *OK_MESSAGES[] = {"Success"};

void logexit(const char *msg);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage);
void sendMessage(const int socket, char *msg);
void sendMessageTo(const int socket, char *msg, const struct sockaddr *addr);