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

#pragma endregion

#pragma region initialization

DLLEXPORT mint WolframLibrary_getVersion() {
    #ifdef _DEBUG
    printf("%s[WolframLibrary_getVersion]%s\n\tWolframLibraryVersion = %d\n\n", GREEN, RESET, WolframLibraryVersion);
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

#pragma region socketOpen

//socketOpen[host_String, port_String, blocking, noDelay, sndBufferSize, rcvBufferSize]: socketId_Integer
DLLEXPORT int socketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char* host = MArgument_getUTF8String(Args[0]); // localhost by default
    char* port = MArgument_getUTF8String(Args[1]); // positive integer as string
    int nonBlocking = (int)MArgument_getInteger(Args[2]); // 0 | 1 default 1 == non-blocking mode
    int keepAlive = (int)MArgument_getInteger(Args[3]); // 0 | 1 default 1 == enable keep-alive
    int noDelay = (int)MArgument_getInteger(Args[4]); // 0 | 1 default 1 == disable Nagle's algorithm
    size_t sndBufSize = (size_t)MArgument_getInteger(Args[5]); // 256 kB by default
    size_t rcvBufSize = (size_t)MArgument_getInteger(Args[6]); // 256 kB by default

    int iResult;
    SOCKET listenSocket = INVALID_SOCKET;
    struct addrinfo hints;
    struct addrinfo *address = NULL;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    /*resolve hints -> address */
    iResult = getaddrinfo(host, port, &hints, &address);
    if (iResult != 0) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tgetaddrinfo(%s:%s) returns error = %d\n\n", RED, RESET, host, port, GETSOCKETERRNO());
        #endif
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*create socket*/
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tsocket(%s:%s) returns error = %d\n\n", RED, RESET, host, port, GETSOCKETERRNO());
        #endif
        freeaddrinfo(address);
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*host:port <=> socket*/
    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tbind(%I64d) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        freeaddrinfo(address);
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*address no longer needed*/
    freeaddrinfo(address);
    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);

    /*set blocking mode*/
    #ifdef _WIN32
    iResult = ioctlsocket(listenSocket, FIONBIO, &nonBlocking);
    #else
    int flags = fcntl(listenSocket, F_GETFL, 0);
    flags |= O_NONBLOCK;
    flags |= O_ASYNC;
    iResult = fcntl(listenSocket, F_SETFL, flags, &nonBlocking);
    #endif
    if (iResult != NO_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tioctlsocket(%I64d, FIONBIO) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*reducing [a b] [c d] [e] -> [a b c d e]*/
    iResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, TCP_NODELAY) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*ping <-> pong*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_KEEPALIVE) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]<=*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvBufSize, sizeof(rcvBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_RCVBUF) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]=>*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBufSize, sizeof(sndBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_SNDBUF) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*wait clients*/
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketOpen->ERROR]%s\n\tblisten(%I64d) returns error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[socketOpen->SUCCESS]%s\n\tsocket id = %I64d\n\n", GREEN, RESET, listenSocket);
    #endif

    MArgument_setInteger(Res, listenSocket);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketClose

/*socketClose[socketId_Integer]: socketId_Integer*/
DLLEXPORT int socketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); // positive integer

    #ifdef _DEBUG
    printf("%s[socketClose]%s\n\tid = %I64d\n\n", GREEN, RESET, socketId);
    #endif

    MArgument_setInteger(Res, CLOSESOCKET(socketId));
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketConnect

//socketConnect[host_String, port_String]: socketId_Integer
DLLEXPORT int socketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char *host = MArgument_getUTF8String(Args[0]); // localhost by default
    char *port = MArgument_getUTF8String(Args[1]); // positive integer as string
    int nonBlocking = (int)MArgument_getInteger(Args[2]); // 0 | 1 default 0 == blocking mode
    int noDelay = (int)MArgument_getInteger(Args[3]); // 0 | 1 default 1 == disable Nagle's algorithm
    int keepAlive = (int)MArgument_getInteger(Args[4]); // 0 | 1 default 1 == enable keep-alive
    int sndBufSize = (int)MArgument_getInteger(Args[5]); // 256 kB by default
    int rcvBufSize = (int)MArgument_getInteger(Args[6]); // 256 kB by default

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
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tgetaddrinfo(%s:%s) returns error = %d\n\n", RED, RESET, host, port, GETSOCKETERRNO());
        #endif
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    connectSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (connectSocket == INVALID_SOCKET){
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tsocket(%s:%s) returns error = %d\n\n", RED, RESET, host, port, GETSOCKETERRNO());
        #endif
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = connect(connectSocket, address->ai_addr, (int)address->ai_addrlen);
    freeaddrinfo(address);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tconnect(%s:%s) returns error = %d\n\n", RED, RESET, host, port, GETSOCKETERRNO());
        #endif
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        freeaddrinfo(address);
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);
    freeaddrinfo(address);

    /*address no longer needed*/
    freeaddrinfo(address);
    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);

    /*set blocking mode*/
    #ifdef _WIN32
    iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlocking);
    #else
    int flags = fcntl(listenSocket, F_GETFL, 0);
    flags |= O_NONBLOCK;
    flags |= O_ASYNC;
    iResult = fcntl(listenSocket, F_SETFL, flags, &nonBlocking);
    #endif
    if (iResult != NO_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tioctlsocket(%I64d, FIONBIO) returns error = %d\n\n", RED, RESET, connectSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*reducing [a b] [c d] [e] -> [a b c d e]*/
    iResult = setsockopt(connectSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tsetsockopt(%I64d, TCP_NODELAY) returns error = %d\n\n", RED, RESET, connectSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*ping <-> pong*/
    iResult = setsockopt(connectSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tsetsockopt(%I64d, SO_KEEPALIVE) returns error = %d\n\n", RED, RESET, connectSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]<=*/
    iResult = setsockopt(connectSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvBufSize, sizeof(rcvBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tsetsockopt(%I64d, SO_RCVBUF) returns error = %d\n\n", RED, RESET, connectSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]=>*/
    iResult = setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBufSize, sizeof(sndBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s[socketConnect->ERROR]%s\n\tsetsockopt(%I64d, SO_SNDBUF) returns error = %d\n\n", RED, RESET, connectSocket, GETSOCKETERRNO());
        #endif
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[socketConnect->SUCCESS]%s\n\tsocket id = %I64d\n\n", GREEN, RESET, connectSocket);
    #endif

    MArgument_setInteger(Res, connectSocket);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region server struct

typedef struct Server_st {
    SOCKET listenSocket;
    SOCKET *clients;
    SOCKET maxClientId;
    size_t clientsLength;
    size_t clientsLengthMax;
    BYTE *buffer;
    size_t bufferSize;
    WolframLibraryData libData;
}* Server;

#pragma endregion

#pragma region server create

//serverCreate[listenSocketId, clientLengthMax, bufferSize]: serverPtr_Integer
DLLEXPORT int serverCreate(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket =    (SOCKET)MArgument_getInteger(Args[0]); // positive integer
    size_t clientLengthMax = (size_t)MArgument_getInteger(Args[1]); // 1024 by default
    size_t bufferSize =      (size_t)MArgument_getInteger(Args[2]); // 256 kB by default

    void *ptr = malloc(sizeof(struct Server_st));
    if (!ptr) {
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(listenSocket = %I64d) error = %d\n\n", RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif

        return LIBRARY_FUNCTION_ERROR;
    }

    Server server = (Server)ptr;

    server->listenSocket = listenSocket;
    server->maxClientId = 0;
    server->clientsLength = 0;
    server->clientsLengthMax = clientLengthMax;
    server->bufferSize = bufferSize;

    server->clients = malloc(clientLengthMax * sizeof(SOCKET));
    if (!server->clients){
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(clients length = %d) returns error = %d\n\n", RED, RESET, clientLengthMax, GETSOCKETERRNO());
        #endif

        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    server->buffer = (char*)malloc(bufferSize * sizeof(char));
    if (!server->buffer){
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(buffer size = %d) returns error = %d\n\n", RED, RESET, bufferSize, GETSOCKETERRNO());
        #endif

        free(server->clients);
        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[serverCreate->SUCCESS]%s\n\tserver pointer = %p\n\tlisten socket id = %I64d\n\tclients length = %d\n\tbuffer size = %d\n\n",
        GREEN, RESET, ptr, listenSocket, clientLengthMax, bufferSize);
    #endif

    uint64_t serverPtr = (uint64_t)(uintptr_t)ptr;
    MArgument_setInteger(Res, serverPtr);
}

#pragma endregion

#pragma region server add client

DLLEXPORT int serverGetListenSocket(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    MArgument_setInteger(Res, server->listenSocket);

    #ifdef _DEBUG
    printf("[serverGetListenSocket]\n\tlistenSocket = %I64d\n\n", server->listenSocket);
    #endif

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

    MArgument_setInteger(Res, port);

    #ifdef _DEBUG
    printf("[socketGetPort]\n\tsocket id = %I64d port = %d\n\n", (int64_t)socketId, port);
    #endif

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

    MArgument_setUTF8String(Res, str);
    
    #ifdef _DEBUG
    printf("[socketGetHostname]\n\tsocket id = %I64d hostname = %s\n\n", (int64_t)socketId, str);
    #endif

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
    memcpy(array, (mint *)server->clients, server->clientsLength * sizeof(SOCKET));

    MArgument_setMTensor(Res, clients); 

    #ifdef _DEBUG
    printf("[serverGetClients]\n\tclientsLength = %zd\n\n", server->clientsLength);
    #endif

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

void addClient(Server server, SOCKET client){
    #ifdef _DEBUG
    printf("[addClient]\n\tadded new client id = %d\n\n", (int)client);
    #endif

    if (server->clientsLength == 0){
        server->maxSocket = client;
    } else if (client > server->maxSocket){
        server->maxSocket = client;
    }

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
    if (2 * server->clientsLengthInvalid <= server->clientsLength) return;

    size_t j = 0;
    server->maxSocket = 0;
    size_t clientsLength = server->clientsLength; 
    for (size_t i = 0; i < clientsLength; i++){
        if (server->clients[i] > 0) {
            server->clients[j] = server->clients[i];
            server->maxSocket = server->clients[i] > server->maxSocket ? server->clients[i] : server->maxSocket;
            j++;
        }
    }

    server->clientsLength = j;
    server->clientsLengthInvalid = 0;
    while ((2 * server->clientsLength <= server->clientsLengthMax) && server->clientsLengthMax > 1) {
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

#pragma region socketListen

void pushNumericArrayEvent(WolframLibraryData libData, mint taskId, SOCKET listenSocket, SOCKET client, char *eventName, BYTE *buffer, mint size){
    MNumericArray numericArray = createByteArray(libData, buffer, size);
    DataStore dataStore = libData->ioLibraryFunctions->createDataStore(); 
    libData->ioLibraryFunctions->DataStore_addInteger(dataStore, listenSocket); 
    libData->ioLibraryFunctions->DataStore_addInteger(dataStore, client);
    libData->ioLibraryFunctions->DataStore_addMNumericArray(dataStore, numericArray);
    libData->ioLibraryFunctions->raiseAsyncEvent(taskId, eventName, dataStore); 
}

void pushEvent(WolframLibraryData libData, mint taskId, SOCKET listenSocket, SOCKET client, char *event){
    DataStore dataStore = libData->ioLibraryFunctions->createDataStore(); 
    libData->ioLibraryFunctions->DataStore_addInteger(dataStore, listenSocket); 
    libData->ioLibraryFunctions->DataStore_addInteger(dataStore, client);
    libData->ioLibraryFunctions->raiseAsyncEvent(taskId, event, dataStore); 
}

void destroyServer(Server server){
    CLOSESOCKET(server->listenSocket); 
    size_t clientsLength = server->clientsLength;

    for (size_t i = 0; i < clientsLength; i++){
        if (ISVALIDSOCKET(server->clients[i])){
            CLOSESOCKET(server->clients[i]);
        }
    }
    
    free(server->buffer);
    free(server->clients);
    free(server->port);
    free(server->host);
}

bool clientsReadyQ(Server server, fd_set *read_set) {
    size_t clientsLength = server->clientsLength;
    SOCKET *clients = server->clients;
    SOCKET maxSocket = server->maxSocket;
    
    printf("[clientsReadyQ]\n\tmaxSocket = %d\n\n", maxSocket);

    if (!server->clients || clientsLength == 0) {
        return false;
    }

    FD_ZERO(read_set);

    printf("[clientsReadyQ]\n\tclients[%d]: [", clientsLength);
    for (size_t i = 0; i < clientsLength; i++) {
        printf("%d, ", clients[i]);
        if (clients[i] != INVALID_SOCKET) {
            FD_SET(clients[i], read_set);
        }
    }
    printf("]\n\n");

    struct timeval timeout = {10, 0}; // 0 seconds, 0 microseconds

    int result = select((int)(maxSocket + 1), read_set, NULL, NULL, &timeout);

    if (result == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("[clientsReadyQ]\n\tselect() error = %d\n\n", GETSOCKETERRNO());
        #endif
        return false;
    }

    printf("[clientsReadyQ]\n\tready clients = %d\n\n", result);

    return (result > 0);
}

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