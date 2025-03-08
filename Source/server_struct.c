#ifdef _WIN32
    #include <windows.h>
#else
    #define SOCKET int
#endif

#include <stdio.h>
#include <stdlib.h>

typedef struct Server_st {
    SOCKET listenSocket;
    size_t clientsLength;
    size_t clientsLengthMax;
    size_t clientsLengthInvalid;
    SOCKET *clients;
    size_t bufferSize;
    BYTE *buffer;
}* Server;

void add_client(Server server, SOCKET client){
    #ifdef _DEBUG
    printf("[add_client]\n\tadded new client id = %d\n\n", (int)client);
    #endif

    server->clients[server->clientsLength] = client;
    server->clientsLength++;

    if (server->clientsLength == server->clientsLengthMax){
        #ifdef _DEBUG
        printf("[add_client]\n\tresize clients array from length = %d to length = %d\n\n",
            server->clientsLengthMax,
            server->clientsLengthMax * 2
        );
        #endif

        server->clientsLengthMax *= 2;
        server->clients = realloc(server->clients, server->clientsLengthMax * sizeof(SOCKET));
    }
}

void remove_invalid_clients(Server server){
    size_t initClientLength = server->clientsLength;
    size_t clientsLength = 0;
    size_t clientsLengthMax = server->clientsLengthMax;

    for (size_t i = 0; i < initClientLength; i++){
        if (server->clients[i] > 0) {
            server->clients[clientsLength] = server->clients[i];
            clientsLength++;
        }

        #ifdef _DEBUG
        else {
            printf("[remove_invalid_clients]\n\tremove client with id = %d\n\n", server->clients[i]);
        }
        #endif
    }

    while ((2 * clientsLength < clientsLengthMax) && clientsLengthMax > 1) {
        clientsLengthMax = clientsLengthMax / 2;
        
        #ifdef _DEBUG
        printf("[remove_invalid_clients]\n\tresize clients array from length = %d to length = %d\n\n",
            clientsLengthMax * 2,
            clientsLengthMax
        );
        #endif
    }

    server->clientsLength = clientsLength;
    server->clientsLengthMax = clientsLengthMax;
    server->clients = realloc(server->clients, sizeof(SOCKET) * clientsLengthMax);
}