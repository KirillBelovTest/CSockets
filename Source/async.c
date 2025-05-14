#pragma region header

#undef UNICODE

#define SECOND 1000000
#define MININTERVAL 1000
#define _DEBUG 1
#define RESET "\033[0m"
#define RED "\033[91m"
#define GREEN "\033[92m"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define FD_SETSIZE 4096
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
    #define GETSOCKETERRNO() (WSAGetLastError())
    #pragma comment (lib, "Ws2_32.lib")
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <wchar.h>
    #include <netinet/tcp.h>
    #include <sys/select.h>
    #include <time.h>
    #define INVALID_SOCKET -1
    #define NO_ERROR 0
    #define SOCKET_ERROR -1
    #define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
    inline void nopp() {}
    #define SOCKET int
    #define ISVALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define GETSOCKETERRNO() (errno)
    #define BYTE uint8_t
    #define BOOL int
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

#include "async.h"

#pragma endregion

#pragma region server data

struct Server_st {
    SOCKET listenSocket;
    size_t clientsCapacity;
    size_t bufferSize;
    struct timeval timeout;
    SOCKET *clients;
    fd_set clientsReadSet;
    size_t clientsReadSetLength;
    size_t clientsLength;
    BYTE *buffer;
    mint taskId;
    WolframLibraryData libData;
};

#pragma endregion

#pragma region server create

int serverCreate(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket =    (SOCKET)MArgument_getInteger(Args[0]); // positive integer
    size_t clientsCapacity = (size_t)MArgument_getInteger(Args[1]); // 1024 by default
    size_t bufferSize =      (size_t)MArgument_getInteger(Args[2]); // 64 kB by default
    long timeout =           (long)MArgument_getInteger(Args[3]);   // 1 s by default

    void *ptr = malloc(sizeof(struct Server_st));
    if (!ptr) {
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(listenSocket = %I64d) returns NULL\n\n", 
            RED, RESET, listenSocket);
        #endif

        return LIBRARY_FUNCTION_ERROR;
    }

    Server server = (Server)ptr;

    struct timeval tv;
    tv.tv_sec  = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;

    server->listenSocket = listenSocket;
    server->clientsCapacity = clientsCapacity;
    server->clientsLength = 0;
    server->clientsReadSetLength = 0;
    server->bufferSize = bufferSize;
    server->timeout = tv;
    server->libData = libData;

    server->clients = malloc(clientsCapacity * sizeof(SOCKET));
    if (!server->clients){
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(clients length = %d) returns NULL\n\n", RED, RESET, clientsCapacity);
        #endif

        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    server->buffer = (char*)malloc(bufferSize * sizeof(char));
    if (!server->buffer){
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(buffer size = %d) returns NULL\n\n", RED, RESET, bufferSize);
        #endif

        free(server->clients);
        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[serverCreate->SUCCESS]%s\n\tserver pointer = %p\n\tlisten socket id = %I64d\n\tclients length = %d\n\tbuffer size = %d\t\ntimeout = %I64d sec and %I64d usec\n\n",
        GREEN, RESET, ptr, listenSocket, clientsCapacity, bufferSize, tv.tv_sec, tv.tv_usec);
    #endif

    uint64_t serverPtr = (uint64_t)(uintptr_t)ptr;
    MArgument_setInteger(Res, serverPtr);
}

#pragma endregion

#pragma region server destroy

void serverDestroy(Server server){
    CLOSESOCKET(server->listenSocket); 

    size_t clientsLength = server->clientsLength;
    for (size_t i = 0; i < clientsLength; i++){
        if (ISVALIDSOCKET(server->clients[i])){
            CLOSESOCKET(server->clients[i]);
        }
    }
    
    free(server->clients);
    free(server->buffer);
    free(server);
}

#pragma endregion

#pragma region server select

void serverSelect(Server server) {
    fd_set *clientsReadSet = &server->clientsReadSet;
    FD_ZERO(clientsReadSet);
    int maxFd = server->listenSocket;
    FD_SET(server->listenSocket, clientsReadSet);
    SOCKET client;

    #ifdef _DEBUG
    printf("%s[socketSelect->CALL]%s\n\tselect(len = %zd, timeout = %I64d) sockets = (",
        GREEN, RESET, server->clientsLength, server->timeout.tv_sec * 1000000 + server->timeout.tv_usec); 
    #endif

    struct timeval *tv = &server->timeout;
    size_t count = server->clientsLength;
    for (size_t i = 0; i < count; i++)
    {
        client = server->clients[i];
        FD_SET(client, clientsReadSet);
        if (client > maxFd) maxFd = client;

        #ifdef _DEBUG
        printf("%I64d ", client);
        #endif
    }

    #ifdef _DEBUG
    printf(")\n\n");
    #endif

    server->clientsReadSetLength = select(maxFd + 1, clientsReadSet, NULL, NULL, tv);
}

#pragma endregion

#pragma region server accept

void serverAccept(Server server){
    if (server->clientsReadSetLength > 0 && FD_ISSET(server->listenSocket, &server->clientsReadSet)) {
        SOCKET client = accept(server->listenSocket, NULL, NULL);
        if (client > 0) {
            server->clients[server->clientsLength] = client;
            server->clientsLength++;
            DataStore data = server->libData->ioLibraryFunctions->createDataStore();
            server->libData->ioLibraryFunctions->DataStore_addInteger(data, server->listenSocket);
            server->libData->ioLibraryFunctions->DataStore_addInteger(data, client);
            server->libData->ioLibraryFunctions->raiseAsyncEvent(server->taskId, "Accept", data);
        }
    }
}

#pragma endregion

#pragma region server recv

void serverRecv(Server server) {
    if (server->clientsReadSetLength > 0) {
        size_t count = server->clientsLength;
        fd_set *readfds = &server->clientsReadSet;
        SOCKET client;
        int readyCount = server->clientsReadSetLength;
        int j = 0;
        
        for (size_t i = 0; i < count; i++) {
            client = server->clients[i];

            if (FD_ISSET(client, readfds)) {
                j++;

                int result = recv(client, server->buffer, server->bufferSize, 0);
                int err = GETSOCKETERRNO();
                if (result > 0) {
                    MNumericArray narr;
                    server->libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &result, narr);
                    BYTE *array = server->libData->numericarrayLibraryFunctions->MNumericArray_getData(narr);
                    memcpy(array, server->buffer, result);

                    DataStore data = server->libData->ioLibraryFunctions->createDataStore();
                    server->libData->ioLibraryFunctions->DataStore_addInteger(data, server->listenSocket);
                    server->libData->ioLibraryFunctions->DataStore_addInteger(data, client);
                    server->libData->ioLibraryFunctions->DataStore_addInteger(data, data);
                    server->libData->ioLibraryFunctions->raiseAsyncEvent(server->taskId, "Recv", data);
                } else if (result == 0 || (result < 0 && (err == 10038 || err == 10053))) {
                    server->clients[i] = INVALID_SOCKET;
                    DataStore data = server->libData->ioLibraryFunctions->createDataStore();
                    server->libData->ioLibraryFunctions->DataStore_addInteger(data, server->listenSocket);
                    server->libData->ioLibraryFunctions->DataStore_addInteger(data, client);
                    server->libData->ioLibraryFunctions->raiseAsyncEvent(server->taskId, "Close", data);
                } else {
                    printf("recv unexpected\n\n");
                }
            }

            if (j > readyCount) break;
        }
    }
}

#pragma endregion

#pragma region server listener task

static void socketListenerTask(mint taskId, void* vtarg)
{
    Server server = (Server)vtarg;
}

//socketListen[serverPtr_Integer]: taskId_Integer
DLLEXPORT int socketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    mint taskId;

    taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(socketListenerTask, server);
    server->taskId = taskId;

    #ifdef _DEBUG
    printf("[socketListen]\n\tlisten socket id = %I64d in taks with id = %I64d\n\n", server->listenSocket, taskId);
    #endif

    MArgument_setInteger(Res, taskId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion