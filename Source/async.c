/*
    ASYNC TCP SERVER, LISTENER and CLIENT
*/

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
    size_t clientsLength;
    BYTE *buffer;
    WolframLibraryData libData;
};

#pragma endregion

#pragma region server create

DLLEXPORT int serverCreate(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
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

DLLEXPORT void serverSelect(Server server) {

}

#pragma endregion

static void socketListenerTask(mint taskId, void* vtarg)
{
    Server server = (Server)vtarg;
    WolframLibraryData libData = server->libData;
    WolframIOLibrary_Functions ioFuncs = libData->ioLibraryFunctions;
    mbool (*asynchronousTaskAliveQ)(mint) = ioFuncs->asynchronousTaskAliveQ;
    mint (*removeAsynchronousTask)(mint) = ioFuncs->removeAsynchronousTask;

    int iResult;
    fd_set read_set;
    fd_set err_set; 

    SOCKET client; 
    SOCKET listenSocket = server->listenSocket;

    size_t bufferSize = server->bufferSize;
    char *buffer = server->buffer;

    size_t clientsLength;
    SOCKET *clients = server->clients;

    BOOL sleepMode = True;

    #ifdef _WIN32
    HANDLE timer = create_timer();
    if (!timer) {
        fprintf(stderr, "Failed to create timer\n");
        exit(EXIT_FAILURE);
    }
    #else
    void* global_timer = NULL; 
    #endif

    while(asynchronousTaskAliveQ(taskId))
    {
        if (sleepMode){
            SLEEP(MININTERVAL, timer);
            removeInvalidClients(server);
        }

        client = accept(listenSocket, NULL, NULL);
        if (ISVALIDSOCKET(client)) {
            sleepMode = False;
            addClient(server, client);
            pushEvent(libData, taskId, listenSocket, client, "Accepted");

            #ifdef _DEBUG
            printf("[socketListenerTask]\n\taccepted socket id = %d\n\n", (int)client);
            #endif
        }

        clientsLength = server->clientsLength; 
        if (clientsReadyQ(server, &read_set)) {
            sleepMode = False;

            for (int i = 0; i < clientsLength; i++) {
                client = clients[i];
                if (FD_ISSET(client, &read_set)) {
                    #ifdef _DEBUG
                    printf("[socketListenerTask]\n\ttry recv from socket %d\n\n", (int)(client));
                    #endif

                    iResult = recv(client, buffer, server->bufferSize, 0);
                    
                    #ifdef _DEBUG
                    printf("[socketListenerTask]\n\trecv result = %d for socket %d\n\n", iResult, (int)(client));
                    #endif

                    if (iResult > 0) {
                        pushNumericArrayEvent(libData, taskId, listenSocket, client, "Received", buffer, (mint)iResult);

                        #ifdef _DEBUG
                        printf("[socketListenerTask]\n\treceived %d bytes from socket %d\n\n", iResult, (int)(client));
                        #endif
                    
                    } else if (iResult == 0){
                        pushEvent(libData, taskId, listenSocket, client, "Closed");
                        clients[i] = INVALID_SOCKET;
                        
                        #ifdef _DEBUG
                        printf("[socketListenerTask]\n\tsocket %I64d closed\n\n", client);
                        #endif
                    }
                    else {
                        int err = GETSOCKETERRNO();
                        if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK || err == 10035 || err == 35) {
                            
                            #ifdef _DEBUG
                            printf("[socketListenerTask]\n\tsocket %I64d is not ready for read\n\n", client);
                            #endif
                            
                        } else {
                            pushEvent(libData, taskId, listenSocket, client, "Closed");
                            clients[i] = INVALID_SOCKET;

                            #ifdef _DEBUG
                            printf("[socketListenerTask]\n\tclosed socket id = %d\n\n", (int)(client));
                            #endif
    
                        }
                    }
                }
                
            }
        } else {
            sleepMode = True;
        }
    }

    destroyServer(server);

    #ifdef _WIN32
    destroy_timer(timer);
    #endif

}

//socketListen[serverPtr_Integer]: taskId_Integer
DLLEXPORT int socketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    mint taskId;

    taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(socketListenerTask, server);

    #ifdef _DEBUG
    printf("[socketListen]\n\tlisten socket id = %I64d in taks with id = %I64d\n\n", server->listenSocket, taskId);
    #endif

    MArgument_setInteger(Res, taskId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketListenerTaskRemove

//socketListenerTaskRemove[taskId_Integer]: taskId_Integer
DLLEXPORT int socketListenerTaskRemove(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    mint taskId = MArgument_getInteger(Args[0]);

    #ifdef _DEBUG
    printf("[socketListenerTaskRemove]\n\tremove taks with id = %I64d\n\n", taskId);
    #endif

    MArgument_setInteger(Res, libData->ioLibraryFunctions->removeAsynchronousTask(taskId));
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketWrite

#define TIMEOUT_INIT_MS 10
#define TIMEOUT_MAX_MULTIPLIER 100

static fd_set global_write_set;
static struct timeval global_socktimeout = {0, 1000}; // 1 ms

int socketReadyForWriteQ(SOCKET socketId) {
    FD_ZERO(&global_write_set);
    FD_SET(socketId, &global_write_set);

    struct timeval socktimeout = global_socktimeout;

    return select(socketId + 1, NULL, &global_write_set, NULL, &socktimeout) == 1;
}

int socketWrite(SOCKET socketId, BYTE *data, int dataLength, int bufferSize) {
    int sentBytes, chunkSize;
    int timeout = TIMEOUT_INIT_MS;
    
    #ifdef _WIN32
    HANDLE timer = create_timer();
    if (!timer) {
        fprintf(stderr, "Failed to create timer\n");
        exit(EXIT_FAILURE);
    }
    #else
    void* timer = NULL; // POSIX does not require a timer
    #endif

    for (int offset = 0; offset < dataLength;) {
        chunkSize = (dataLength - offset > bufferSize) ? bufferSize : dataLength - offset;

        if (socketReadyForWriteQ(socketId)) {
            sentBytes = send(socketId, (char*)&data[offset], chunkSize, 0);

            if (sentBytes == SOCKET_ERROR) {
                int err = GETSOCKETERRNO();
                printf("[socketWrite] Error sending %d bytes on socket %d at offset %d, error %d\n", dataLength, (int)socketId, offset, err);

                if (err == 10035 || err == 35) {  // EWOULDBLOCK or similar
                    #ifdef _DEBUG
                    printf("[socketWrite] Write paused for %d ms\n", timeout);
                    #endif
                } else if (err == 10038) {
                    #ifdef _DEBUG
                    printf("[socketWrite] Write error for %d\n", socketId);
                    #endif
                    return SOCKET_ERROR;
                } else {
                    return SOCKET_ERROR;
                }
            } else {
                offset += sentBytes;
                timeout = TIMEOUT_INIT_MS; 
                continue;
            }
        } 
        #ifdef _DEBUG
        else {
            printf("[socketWrite] Socket not ready, pausing for %d ms\n", timeout);
        }
        #endif

        SLEEP(timeout * MININTERVAL, timer);
        timeout += TIMEOUT_INIT_MS;

        if (timeout > TIMEOUT_INIT_MS * TIMEOUT_MAX_MULTIPLIER) {
            #ifdef _DEBUG
            printf("[socketWrite] Timeout exceeded\n");
            #endif
            return SOCKET_ERROR;
        }
    }

    return dataLength;
}

//socketBinaryWrite[socketId_Integer, data: ByteArray[<>], dataLength_Integer, bufferLength_Integer]: socketId_Integer
DLLEXPORT int socketBinaryWrite(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET clientId = MArgument_getInteger(Args[0]);
    MNumericArray mArr = MArgument_getMNumericArray(Args[1]);

    int iResult;
    BYTE *data = (BYTE *)libData->numericarrayLibraryFunctions->MNumericArray_getData(mArr);
    int dataLength = MArgument_getInteger(Args[2]);
    int bufferSize = MArgument_getInteger(Args[3]);

    iResult = socketWrite(clientId, data, dataLength, bufferSize);
    if (iResult == SOCKET_ERROR) {
        CLOSESOCKET(clientId);
        MArgument_setInteger(Res, GETSOCKETERRNO());
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, clientId);
    libData->numericarrayLibraryFunctions->MNumericArray_disown(mArr);
    #ifdef _DEBUG
    printf("[socketBinaryWrite]\n\tsocket id = %I64d sent = %d bytes\n\n", clientId, iResult);
    #endif
    return LIBRARY_NO_ERROR;
}

//socketWriteString[socketId_Integer, data_String, dataLength_Integer, bufferLength_Integer]: socketId_Integer
DLLEXPORT int socketWriteString(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult;
    SOCKET socketId = MArgument_getInteger(Args[0]);
    char* data = MArgument_getUTF8String(Args[1]);
    int dataLength = MArgument_getInteger(Args[2]);
    int bufferSize = MArgument_getInteger(Args[3]);

    iResult = socketWrite(socketId, data, dataLength, bufferSize);
    if (iResult == SOCKET_ERROR) {
        CLOSESOCKET(socketId);
        MArgument_setInteger(Res, GETSOCKETERRNO());
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, socketId);
    libData->UTF8String_disown(data);
    #ifdef _DEBUG
    printf("[socketWriteString]\n\tsocket id = %I64d sent = %d bytes\n\n", socketId, iResult);
    #endif
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadyQ

//socketReadyQ[socketId_Integer]: readyQ: True | False
DLLEXPORT int socketReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);

    int iResult;
    static BYTE *buffer = NULL; 
    if (buffer == NULL) {
        buffer = (BYTE *)malloc(sizeof(BYTE));
    }

    iResult = recv(socketId, buffer, 1, MSG_PEEK);
    if (iResult == SOCKET_ERROR){
        MArgument_setBoolean(Res, False);
    } else {
        MArgument_setBoolean(Res, True);
    }

    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadMessage

//socketReadMessage[socketId_Integer, bufferSize_Integer]: ByteArray[<>]
DLLEXPORT int socketReadMessage(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);
    int bufferSize = MArgument_getInteger(Args[1]);

    static BYTE *buffer = NULL; 
    if (buffer == NULL) {
       buffer = (BYTE*)malloc(bufferSize * sizeof(BYTE));
    }

    int iResult;
    int length = 0;

    iResult = recv(socketId, buffer, bufferSize, 0);
    if (iResult > 0) {
        MArgument_setMNumericArray(Res, createByteArray(libData, buffer, iResult));
    } else {
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("[socketReadMessage]\n\tsocket id = %I64d received = %d bytes\n\n", socketId, iResult);
    #endif

    return LIBRARY_NO_ERROR;
}

#pragma endregion

bool socketAliveQ(SOCKET socketId) {
    if (socketId == INVALID_SOCKET) {
        return false;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socketId, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0; // 0 seconds, 0 microseconds

    int result = select(socketId + 1, &readfds, NULL, NULL, &timeout);
    if (result == -1) {
        // select error
        return false;
    }

    if (result == 0) {
        // Нет активности — считаем, что соединение живо
        return true;
    }

    // Сокет готов к чтению — проверим, не закрыт ли он
    char buffer;
#ifdef _WIN32
    int flags = MSG_PEEK;
    int recvResult = recv(socketId, &buffer, 1, flags);
#else
    ssize_t recvResult = recv(socketId, &buffer, 1, MSG_PEEK);
#endif

    if (recvResult == 0) {
        // Получен TCP FIN — соединение закрыто клиентом
        return false;
    } else if (recvResult < 0) {
    #ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return true;  // Просто нет данных, соединение живо
        } else {
            return false; // Другая ошибка — считаем соединение разорванным
        }
    #else
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return true;  // Соединение живо
        } else {
            return false; // Соединение разорвано
        }
    #endif
    }

    // Есть данные для чтения — соединение живо
    return true;
}