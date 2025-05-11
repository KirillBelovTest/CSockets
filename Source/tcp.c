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
#include "WolframCompileLibrary.h"
#include "WolframRawArrayLibrary.h"
#include "WolframImageLibrary.h"

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
    printf("%s[WolframLibrary_initialize]%s\n\tinitialization\n\n", GREEN, RESET);
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

#pragma region socket open

//socketOpen[host, port, nonBlocking, noDelay, sndBufferSize, rcvBufferSize]: socketId
DLLEXPORT int socketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char* host = MArgument_getUTF8String(Args[0]); // localhost by default
    char* port = MArgument_getUTF8String(Args[1]); // positive integer as string
    int nonBlocking = (int)MArgument_getInteger(Args[2]); // 0 | 1 default 0 == blocking mode
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

#pragma region socket close

//socketClose[socketId]: successState
DLLEXPORT int socketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); // positive integer

    #ifdef _DEBUG
    printf("%s[socketClose]%s\n\tsocketId = %I64d\n\n", GREEN, RESET, socketId);
    #endif

    MArgument_setInteger(Res, CLOSESOCKET(socketId));
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socket connect

//socketConnect[host, port, nonBlocking, noDelay, keepAlive, sndBufSize, rcvBufSize]: socketId
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

#pragma region socket select

//socketSelect[{socket1, socket2, ..}, length, timeout]: {socket2, ..}
DLLEXPORT int socketSelect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    MTensor socketIdsList = MArgument_getMTensor(Args[0]); // list of sockets
    size_t length = (size_t)MArgument_getInteger(Args[1]); // number of sockets
    int timeout = (int)MArgument_getInteger(Args[2]); // timeout in microseconds

    int err;
    SOCKET *socketIds = (SOCKET*)libData->MTensor_getIntegerData(socketIdsList);
    SOCKET socketId;
    SOCKET maxFd = 0;
    fd_set readfds;
    FD_ZERO(&readfds);

    #ifdef _DEBUG
    printf("%s[socketSelect->CALL]%s\n\tselect(len = %zd, timeout = %d) sockets = (",
        GREEN, RESET, length, timeout); 
    #endif

    for (size_t i = 0; i < length; i++) {
        socketId = socketIds[i];
        if (socketId > maxFd) maxFd = socketId;
        FD_SET(socketId, &readfds);

        #ifdef _DEBUG
        printf("%I64d ", socketId);
        #endif
    }

    #ifdef _DEBUG
    printf(")\n\n");
    #endif

    struct timeval tv;
    tv.tv_sec  = timeout / 1000000;
    tv.tv_usec = timeout % 1000000;

    int result = select((int)(maxFd + 1), &readfds, NULL, NULL, &tv);
    if (result >= 0) {
        mint len = (mint)result;
        MTensor readySocketsTensor;
        libData->MTensor_new(MType_Integer, 1, &len, &readySocketsTensor);
        SOCKET *readySockets = (SOCKET*)libData->MTensor_getIntegerData(readySocketsTensor);
        
        #ifdef _DEBUG
        printf("%s[socketSelect->SUCCESS]%s\n\tselect() returns sockets = (", 
            GREEN, RESET);
        #endif

        int j = 0;
        for (size_t i = 0; i < length; i++) {
            socketId = socketIds[i];
            if (FD_ISSET(socketId, &readfds)) { 
                readySockets[j] = socketId;
                j++;

                #ifdef _DEBUG
                printf("%I64d ", socketId);
                #endif
            }
        }

        #ifdef _DEBUG
        printf(")\n\n");
        #endif

        MArgument_setMTensor(Res, readySocketsTensor);
        return LIBRARY_NO_ERROR;
    } else {
        err = GETSOCKETERRNO();
        #ifdef _DEBUG
        printf("%s[socketSelect->ERROR]%s\n\tselect() returns error = %d",
            RED, RESET, err); 
        #endif

        if (
            #ifdef _WIN32
            err == WSAEINTR
            #else
            err == EINTR
            #endif
        ) {
            libData->Message("tryagain");
        } else if (
            #ifdef _WIN32
            err == WSAEINVAL
            #else
            err == EINVAL
            #endif
        ){
            libData->Message("argserr");
        } else if (
            #ifdef _WIN32
            err == WSAENOTSOCK
            #else
            err == EBADF
            #endif
        ) {
            libData->Message("invldsock");
        }

        return LIBRARY_FUNCTION_ERROR;
    }
}

#pragma endregion

#pragma region socket check

//socketCheck[{socket1, socket2, ..}, length]: {1, 0, ..}
DLLEXPORT int socketCheck(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    MTensor socketList = MArgument_getMTensor(Args[0]); // list of sockets
    size_t length = (size_t)MArgument_getInteger(Args[1]); // number of sockets

    SOCKET *sockets = (SOCKET*)libData->MTensor_getIntegerData(socketList);
    SOCKET sock;

    for (size_t i = 0; i < length; i++) {
        sock = sockets[i];
        sockets[i] = true; 

        int opt;
        socklen_t len = sizeof(opt);

        #ifdef _WIN32
        int result = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char*)&opt, &len);
        #else
        int result = fcntl(sock, F_GETFL)
        #endif

        if (result < 0) {
            sockets[i] = GETSOCKETERRNO() != WSAENOTSOCK;
        }
    }
    
    MArgument_setMTensor(Res, socketList);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socket accept

DLLEXPORT int socketAccept(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket = (SOCKET)MArgument_getInteger(Args[0]);
    SOCKET client = accept(listenSocket, NULL, NULL);
    if (client == INVALID_SOCKET){

        #ifdef _DEBUG
        printf("%s[serverAccept->ERROR]%s\n\taccept(listenSocket = %I64d) returns error = %d\n\n", 
            RED, RESET, listenSocket, GETSOCKETERRNO());
        #endif

        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[socketAccept->SUCCESS]%s\n\taccept(listenSocket = %I64d) new client id = %I64d\n\n", 
        GREEN, RESET, listenSocket, client);
    #endif

    MArgument_setInteger(Res, client);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socket recv

DLLEXPORT int socketRecv(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET client = (SOCKET)MArgument_getInteger(Args[0]);
    mint bufferSize = (mint)MArgument_getInteger(Args[1]);
    
    BYTE *buffer = malloc(bufferSize * sizeof(BYTE));
    int result = recv(client, buffer, bufferSize, 0);
    if (result > 0){
        #ifdef _DEBUG
        printf("%s[serverRecv->SUCCESS]%s\n\trecv(socket = %I64d) received = %d bytes\n\n", 
            GREEN, RESET, client, result);
        #endif

        mint len = (mint)result;
        MNumericArray byteArray;
        libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &len, &byteArray);
        BYTE *array = libData->numericarrayLibraryFunctions->MNumericArray_getData(byteArray);
        memcpy(array, buffer, result);

        free(buffer);
        MArgument_setMNumericArray(Res, byteArray);
        return LIBRARY_NO_ERROR;
    } else if (result == 0) {
        #ifdef _DEBUG
        printf("%s[serverRecv->ERROR]%s\n\trecv(socket = %I64d) socket close and returns error = %d\n\n", 
            RED, RESET, client, GETSOCKETERRNO());
        #endif

        libData->Message("sockclose");
        return LIBRARY_FUNCTION_ERROR;
    } else {
        int err = GETSOCKETERRNO();

        #ifdef _DEBUG
        printf("%s[serverRecv->ERROR]%s\n\trecv(socket = %I64d) returns error = %d\n\n", 
            RED, RESET, client, err);
        #endif

        if (err == 10053) {
            libData->Message("sockclose");
        } else if (err == 10038) {
            libData->Message("nonsock");
        }
        return LIBRARY_FUNCTION_ERROR;
    }
}

#pragma endregion

#pragma region send

//socketBinaryWrite[socketId_Integer, data: ByteArray[<>], dataLength_Integer]: sentLength_Integer
DLLEXPORT int socketSend(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); // positive integer
    MNumericArray mArr = MArgument_getMNumericArray(Args[1]); 
    int dataLength = MArgument_getInteger(Args[2]); // positive integer

    int result;
    BYTE *data = (BYTE*)libData->numericarrayLibraryFunctions->MNumericArray_getData(mArr);

    result = send(socketId, data, dataLength, 0);
    if (result > 0) {
        #ifdef _DEBUG
        printf("%s[socketSend->SUCCESS]%s\n\tsend(socket id = %I64d) sent = %d bytes\n\n", 
            GREEN, RESET, socketId, result);
        #endif

        MArgument_setInteger(Res, result);
        return LIBRARY_NO_ERROR;
    }
    else if (result == SOCKET_ERROR) {
        int err = GETSOCKETERRNO();

        #ifdef _DEBUG
        printf("%s[socketSend->ERROR]%s\n\tsend(socket id = %I64d) returns error = %d\n\n", 
            RED, RESET, socketId, err);
        #endif

        if (err == WSAEINVAL) {
            libData->Message("csockargex");
        } else {
            libData->Message("csockunexpected");
        }

        return LIBRARY_FUNCTION_ERROR;
    }
}

#pragma endregion
