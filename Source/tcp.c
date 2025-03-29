#pragma region header

#undef UNICODE

#define SECOND 1000000
#define MILISECOND 1000
#define _DEBUG 1

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
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

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

#pragma endregion

#pragma region initialization

DLLEXPORT mint WolframLibrary_getVersion() {
    #ifdef _DEBUG
    printf("[WolframLibrary_getVersion]\n\tWolframLibraryVersion = %d\n\n", WolframLibraryVersion);
    #endif

    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    #ifdef _DEBUG
    printf("[WolframLibrary_initialize]\n\tinitialization\n\n");
    #endif

    #ifdef _WIN32
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) return LIBRARY_FUNCTION_ERROR;
    #endif

    return LIBRARY_NO_ERROR;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    #ifdef _DEBUG
    printf("[WolframLibrary_uninitialize]\n\tuninitialization\n\n");
    #endif

    #ifdef _WIN32
    WSACleanup();
    #else
    SLEEP(SECOND);
    #endif

    return;
}

#pragma endregion

#pragma region server struct

typedef struct Server_st {
    SOCKET listenSocket;
    SOCKET *clients;
    size_t clientsLength;
    size_t clientsLengthMax;
    size_t clientsLengthInvalid;
    BYTE *buffer;
    size_t bufferSize;
    size_t sndBufSize;
    size_t rcvBufSize;
    char *host;
    char *port;
    WolframLibraryData libData;
}* Server;

DLLEXPORT int serverGetListenSocket(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    MArgument_setInteger(Res, server->listenSocket);
    return LIBRARY_NO_ERROR;
}

//socketGetPort[socketId_Integer]: port_Integer
DLLEXPORT int socketGetPort(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    SOCKET socketId = MArgument_getInteger(Args[0]);
    struct  sockaddr_in sin;
    int port;
    int addrlen = sizeof(sin);

    getsockname(socketId, (struct sockaddr *)&sin, &addrlen);
    port = ntohs(sin.sin_port);

    #ifdef _DEBUG
    printf("[socketHostname]\n\tsocket id = %I64d hostname = %d\n\n", (int64_t)socketId, port);
    #endif

    MArgument_setInteger(Res, port);
    return LIBRARY_NO_ERROR;
}

//socketGetHostname[socketId_Integer]: hostname_String
DLLEXPORT int socketGetHostname(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    SOCKET socketId = MArgument_getInteger(Args[0]);
    struct sockaddr_in sin;
    char *str;
    str = (void*)malloc(INET_ADDRSTRLEN * sizeof(char));
    int addrlen = sizeof(sin);

    getsockname(socketId, (struct sockaddr *)&sin, &addrlen);
    struct in_addr ip = sin.sin_addr;
    inet_ntop(AF_INET, &ip, str, INET_ADDRSTRLEN);

    #ifdef _DEBUG
    printf("[socketHostname]\n\tsocket id = %I64d hostname = %s\n\n", (int64_t)socketId, str);
    #endif

    MArgument_setUTF8String(Res, str);
    
    free(str);

    return LIBRARY_NO_ERROR;
}

DLLEXPORT int serverGetClients(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);

    MTensor clients;
    mint type = MType_Integer;
    mint dims[1];
    mint rank = 1;
    int err;
    dims[0] = server->clientsLength;
    
    err = libData->MTensor_new(type, rank, dims, &clients);
    if (err != LIBRARY_NO_ERROR) {
        #ifdef _DEBUG
        printf("[getServerClients]\n\tMTensor_new() error = %d\n\n", err);
        #endif
        return LIBRARY_FUNCTION_ERROR;
    }

    mint *array = libData->MTensor_getIntegerData(clients);
    #ifdef _DEBUG
    printf("[getServerClients]\n\tclientsLength = %zd\n\n", server->clientsLength);
    #endif
    memcpy(array, (mint *)server->clients, server->clientsLength * sizeof(SOCKET));

    MArgument_setMTensor(Res, clients); 

    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region timer

#ifdef _WIN32
HANDLE create_timer(void) {
    return CreateWaitableTimer(NULL, TRUE, NULL);
}

void destroy_timer(HANDLE timer) {
    if (timer) CloseHandle(timer);
}
#endif

void SLEEP(uint64_t usec, void* timer_ptr) {
    #ifdef _WIN32
        HANDLE timer = (HANDLE)timer_ptr;
        if (!timer) return;
    
        LARGE_INTEGER li;
        li.QuadPart = -(LONGLONG)(usec * 10); // microseconds to 100-nanosecond intervals
        if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
            return;
        }
    
        WaitForSingleObject(timer, INFINITE);
    #else
        (void)timer_ptr; // unused on POSIX
        struct timespec ts;
        ts.tv_sec = usec / 1000000;
        ts.tv_nsec = (usec % 1000000) * 1000;
        nanosleep(&ts, NULL);
    #endif
}

#pragma endregion

#pragma region server

//getServerListenSocket[serverPtr_Integer]: listenSocketId_Integer
DLLEXPORT int getServerListenSocket(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    MArgument_setInteger(Res, server->listenSocket); 
    return LIBRARY_NO_ERROR; 
}

void addClient(Server server, SOCKET client){
    #ifdef _DEBUG
    printf("[addClient]\n\tadded new client id = %d\n\n", (int)client);
    #endif

    server->clients[server->clientsLength] = client;
    server->clientsLength++;

    if (server->clientsLength == server->clientsLengthMax){
        #ifdef _DEBUG
        printf("[addClient]\n\tresize clients array from length = %d to length = %d\n\n",
            (int)server->clientsLengthMax,
            (int)server->clientsLengthMax * 2
        );
        #endif

        server->clientsLengthMax *= 2;
        server->clients = realloc(server->clients, server->clientsLengthMax * sizeof(SOCKET));
    }
}

void removeInvalidClients(Server server){
    size_t j = 0;
    for (size_t i = 0; i < server->clientsLength; i++){
        if (server->clients[i] > 0) {
            server->clients[j] = server->clients[i];
            j++;
        }
        #ifdef _DEBUG
        else {
            printf("[removeInvalidClients]\n\tremove client with id = %I64d\n\n", server->clients[i]);
        }
        #endif
    }

    server->clientsLength = j;
    server->clientsLengthInvalid = 0;
    while ((2 * server->clientsLength < server->clientsLengthMax) && server->clientsLengthMax > 1) {
        server->clientsLengthMax = server->clientsLengthMax / 2;
        server->clients = realloc(server->clients, sizeof(SOCKET) * server->clientsLengthMax);
        #ifdef _DEBUG
        printf("[removeInvalidClients]\n\tresize clients array from length = %zd to length = %zd\n\n",
            server->clientsLengthMax * 2,
            server->clientsLengthMax
        );
        #endif
    }
}

#pragma endregion

#pragma region numeric arrays

MNumericArray createByteArray(WolframLibraryData libData, BYTE *data, const mint dataLength){
    MNumericArray nArray;
    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &dataLength, &nArray);

    BYTE *array = libData->numericarrayLibraryFunctions->MNumericArray_getData(nArray);
    memcpy(array, data, dataLength); 
    return nArray;
}

#pragma endregion

#pragma region socketOpen

//socketOpen[host_String, port_String, bufferSize, modeNoDelay, mode]: socketId_Integer
DLLEXPORT int socketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char* host = MArgument_getUTF8String(Args[0]);
    char* port = MArgument_getUTF8String(Args[1]);
    
    int bufferSize = (size_t)MArgument_getInteger(Args[2]);
    size_t sndBufSize = (size_t)MArgument_getInteger(Args[3]);
    size_t rcvBufSize = (size_t)MArgument_getInteger(Args[4]);
    int iModeNoDelay = (int)MArgument_getInteger(Args[5]); // 0 | 1 default 0
    int iMode = (int)MArgument_getInteger(Args[6]); // 0 | 1 default 1
    
    int iResult;
    SOCKET listenSocket = INVALID_SOCKET;
    struct addrinfo hints;
    struct addrinfo *address = NULL;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(host, port, &hints, &address);
    if (iResult != 0) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tgetaddrinfo() for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        return LIBRARY_FUNCTION_ERROR;
    }

    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tsocket() for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&iModeNoDelay, sizeof(iModeNoDelay));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tsetsockopt(TCP_NODELAY) for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&iMode, sizeof(iMode));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tsetsockopt(SO_KEEPALIVE) for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvBufSize, sizeof(rcvBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tsetsockopt(SO_RCVBUF) for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBufSize, sizeof(sndBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tsetsockopt(SO_SNDBUF) for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tbind() for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tblisten() for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _WIN32
    iResult = ioctlsocket(listenSocket, FIONBIO, &iMode);
    #else
    int flags = fcntl(listenSocket, F_GETFL, 0);
    flags |= O_NONBLOCK;
    flags |= O_ASYNC;
    iResult = fcntl(listenSocket, F_SETFL, flags, &iMode);
    #endif
    if (iResult != NO_ERROR) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tioctlsocket(FIONBIO) for port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
    }

    void *ptr = malloc(sizeof(struct Server_st));
    if (!ptr) {
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tmalloc() for Server struct on port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    Server server = (Server)ptr;

    server->listenSocket = listenSocket;

    server->clients = malloc(2 * sizeof(SOCKET));
    if (!server->clients){
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tmalloc() for clients array on port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    server->clients[0] = INVALID_SOCKET;
    server->clients[1] = INVALID_SOCKET;
    
    server->clientsLength = 0;
    server->clientsLengthMax = 2;
    server->clientsLengthInvalid = 0;

    server->bufferSize = bufferSize;
    server->buffer = (char*)malloc(bufferSize * sizeof(char));
    if (!server->buffer){
        #ifdef _DEBUG
        printf("[socketOpen->ERROR]\n\tmalloc() for buffer on port = %s returns error = %d\n\n", port, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        free(server->clients);
        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    server->host = (char*)malloc(strlen(host) + 1);
    server->port = (char*)malloc(strlen(port) + 1);
    strcpy(server->host, host);
    strcpy(server->port, port);

    server->libData = libData;

    freeaddrinfo(address);
    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);

    uint64_t serverPtr = (uint64_t)(uintptr_t)ptr;

    MArgument_setInteger(Res, serverPtr);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketClose

//socketClose[socketId_Integer]: socketId_Integer
DLLEXPORT int socketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);
    #ifdef _DEBUG
    printf("[socketClose]\n\tid = %I64d\n\n", socketId);
    #endif
    MArgument_setInteger(Res, CLOSESOCKET(socketId));
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketListen

static void socketListenerTask(mint taskId, void* vtarg)
{
    Server server = (Server)vtarg;
	WolframLibraryData libData = server->libData;

    int iResult;
    int len; 
    SOCKET clientSocket = INVALID_SOCKET;
    char *buffer = (char*)malloc(server->bufferSize * sizeof(char));
    mint dims[1];
    MNumericArray data;
	DataStore ds;
    BYTE *array; 
    SOCKET client; 
    SOCKET listenSocket = server->listenSocket;
    size_t bufferSize = server->bufferSize;
    SOCKET *clients = server->clients;
    size_t clientsLength = server->clientsLength;
    size_t clientsLengthMax = server->clientsLengthMax;
    size_t clientsLengthInvalid = server->clientsLengthInvalid;

    time_t readTime = time(NULL);
    BOOL sleepMode = True;

    #ifdef _WIN32
    HANDLE global_timer = create_timer();
    if (!global_timer) {
        fprintf(stderr, "Failed to create timer\n");
        exit(EXIT_FAILURE);
    }
    #else
    void* global_timer = NULL; // Для POSIX таймер не нужен
    #endif

    //funcions

    WolframIOLibrary_Functions ioFuncs = libData->ioLibraryFunctions;
    WolframNumericArrayLibrary_Functions naFuncs = libData->numericarrayLibraryFunctions;
    DataStore (*createDataStoreFunc)() = ioFuncs->createDataStore;
    void (*addIntegerFunc)(DataStore, mint) = ioFuncs->DataStore_addInteger;
    void (*addMNumericArrayFunc)(DataStore, MNumericArray) = ioFuncs->DataStore_addMNumericArray;
    void (*raiseAsyncEventFunc)(mint, char*, DataStore) = ioFuncs->raiseAsyncEvent;
    mbool (*asynchronousTaskAliveQFunc)(mint) = ioFuncs->asynchronousTaskAliveQ;
    mint (*removeAsynchronousTaskFunc)(mint) = ioFuncs->removeAsynchronousTask;
    errcode_t (*newNArrFunc)(const numericarray_data_t, const mint, const mint*, MNumericArray*) = naFuncs->MNumericArray_new;
    void (*disownNArrFunc)(MNumericArray) = naFuncs->MNumericArray_disown;
    void *(*getNArrDataFunc)(MNumericArray) = naFuncs->MNumericArray_getData;

	while(asynchronousTaskAliveQFunc(taskId))
	{
        if (sleepMode){
            SLEEP(MILISECOND, global_timer);
        }

        clientSocket = accept(listenSocket, NULL, NULL);
        if (ISVALIDSOCKET(clientSocket)) {
            #ifdef _DEBUG
            printf("[socketListenerTask]\n\taccepted socket id = %d\n\n", (int)clientSocket);
            #endif
            addClient(server, clientSocket);
            clientsLength = server->clientsLength;
            clientsLengthMax = server->clientsLengthMax;

            ds = createDataStoreFunc();
            addIntegerFunc(ds, listenSocket);
            addIntegerFunc(ds, clientSocket);
            addIntegerFunc(ds, 0);
            raiseAsyncEventFunc(taskId, "Accepted", ds);

            sleepMode = False;
        }

        sleepMode = True;
        len = clientsLength; 
        for (int i = 0; i < len; i++) {
            client = clients[i];
            if (client != INVALID_SOCKET) {
                iResult = recv(client, buffer, bufferSize, 0);
                if (iResult > 0) {
                    #ifdef _DEBUG
                    printf("[socketListenerTask]\n\treceived %d bytes from socket %d\n\n", iResult, (int)(client));
                    #endif
                    sleepMode = False;
                    dims[0] = iResult;
                    newNArrFunc(MNumericArray_Type_UBit8, (mint)1, dims, &data);
                    memcpy(getNArrDataFunc(data), buffer, iResult);
                    ds = createDataStoreFunc();
                    addIntegerFunc(ds, listenSocket);
                    addIntegerFunc(ds, client);
                    addMNumericArrayFunc(ds, data);
                    raiseAsyncEventFunc(taskId, "Received", ds);
                } else if (iResult == 0) {
                    #ifdef _DEBUG
                    printf("[socketListenerTask]\n\tclosed socket id = %d\n\n", (int)(client));
                    #endif
                    ds = createDataStoreFunc();
                    addIntegerFunc(ds, listenSocket);
                    addIntegerFunc(ds, client);
                    addIntegerFunc(ds, 0);
                    raiseAsyncEventFunc(taskId, "Closed", ds);
                    clients[i] = INVALID_SOCKET;
                    clientsLengthInvalid++;

                    if (2 * clientsLengthInvalid > clientsLengthMax) {
                        removeInvalidClients(server);
                        clientsLength = server->clientsLength;
                        clientsLengthMax = server->clientsLengthMax;
                        clientsLengthInvalid = server->clientsLengthInvalid;
                    }
                }
            }
        }
	}

    for (int i = 0; i < clientsLength; i++) {
        #ifdef _DEBUG
        printf("[socketListenerTask]\n\tclose socket id = %d\n\n", (int)(clients[i]));
        #endif
        ds = createDataStoreFunc();
        addIntegerFunc(ds, listenSocket);
        addIntegerFunc(ds, clients[i]);
        addIntegerFunc(ds, 0);        
        raiseAsyncEventFunc(taskId, "Closed", ds); 
        CLOSESOCKET(clients[i]);
    }

    #ifdef _WIN32
    destroy_timer(global_timer);
    #endif

    free(server->clients);
    free(buffer);
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

#pragma region socketConnect

//socketConnect[host_String, port_String]: socketId_Integer
DLLEXPORT int socketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char *host = MArgument_getUTF8String(Args[0]);
    char *port = MArgument_getUTF8String(Args[1]);

    int iResult;
    int iMode = 1;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo *address = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(host, port, &hints, &address);
    if (iResult != 0){
        return LIBRARY_FUNCTION_ERROR;
    }

    connectSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (connectSocket == INVALID_SOCKET){
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = connect(connectSocket, address->ai_addr, (int)address->ai_addrlen);
    freeaddrinfo(address);
    if (iResult == SOCKET_ERROR) {
        CLOSESOCKET(connectSocket);
        connectSocket = INVALID_SOCKET;
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("[socketConnect]\n\tsocketId = %I64d connect to %s:%s\n\n", connectSocket, host, port);
    #endif

    #ifdef _WIN32
    iResult = ioctlsocket(connectSocket, FIONBIO, &iMode);
    #else
    iResult = fcntl(connectSocket, O_NONBLOCK, &iMode);
    #endif

    MArgument_setInteger(Res, connectSocket);

    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);

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

        SLEEP(timeout * MILISECOND, timer);
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