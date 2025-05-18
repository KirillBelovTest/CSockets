#pragma region header

#undef UNICODE

#define SECOND 1000000
#define MININTERVAL 1000
#define _DEBUG 1
#define RESET "\033[0m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define BLUE "\033[94m"

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

#pragma region time print

const char* getCurrentTime() {
    static char time_buffer[64];
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    #ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    snprintf(time_buffer + strlen(time_buffer), 
             sizeof(time_buffer) - strlen(time_buffer), 
             ".%03d", st.wMilliseconds);
    #else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(time_buffer + strlen(time_buffer), 
             sizeof(time_buffer) - strlen(time_buffer), 
             ".%06ld", tv.tv_usec);
    #endif
    
    return time_buffer;
}

#pragma endregion

#pragma region errors

// acceptErrorMessage: maps accept() errors to Wolfram messages
void acceptErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("acceptRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("acceptDelayRetry");

  #ifdef _WIN32
  else if (err == WSAEMFILE || err == WSAENOBUFS || err == WSAENETDOWN || err == WSAEINVAL)
  #else
  else if (err == EMFILE || err == ENFILE || err == ENOBUFS || err == ENOMEM || err == EOPNOTSUPP)
  #endif
    libData->Message("acceptCloseSocket");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("acceptFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("acceptReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("acceptRestartProcess");
  #endif

  else
    libData->Message("acceptUnexpectedError");
}

// recvErrorMessage: maps recv() errors to Wolfram messages
void recvErrorMessage(WolframLibraryData libData, int err) {
  if (err == 0) {
    libData->Message("recvGracefulClose");
    return;
  }

  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("recvRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("recvDelayRetry");

  #ifdef _WIN32
  else if (err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAESHUTDOWN || err == WSAENOTCONN)
  #else
  else if (err == ECONNRESET || err == ECONNABORTED || err == ESHUTDOWN || err == ENOTCONN)
  #endif
    libData->Message("recvCloseSocket");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("recvFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("recvReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("recvRestartProcess");
  #endif

  else
    libData->Message("recvUnexpectedError");
}

// selectErrorMessage: maps select() errors to Wolfram messages
void selectErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("selectRetry");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEFAULT || err == WSAEINVAL || err == WSAENOBUFS)
  #else
  else if (err == EBADF || err == EFAULT || err == EINVAL || err == ENOMEM)
  #endif
    libData->Message("selectClose");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("selectFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("selectReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("selectRestartProcess");
  #endif

  else
    libData->Message("selectUnexpectedError");
}

// sendErrorMessage: maps send() errors to Wolfram messages
void sendErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("sendRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("sendDelayRetry");

  #ifdef _WIN32
  else if (err == WSAENOTCONN || err == WSAESHUTDOWN || err == WSAECONNRESET)
  #else
  else if (err == ENOTCONN || err == EPIPE || err == ECONNRESET || err == ESHUTDOWN)
  #endif
    libData->Message("sendCloseSocket");

  #ifdef _WIN32
  else if (err == WSAEMSGSIZE)
  #else
  else if (err == EMSGSIZE)
  #endif
    libData->Message("sendMsgSize");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("sendFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("sendReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("sendRestartProcess");
  #endif

  else
    libData->Message("sendUnexpectedError");
}

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

    #ifdef _DEBUG
    printf("%s\n%s[socketOpen->CALL]%s\n\tfor address %s:%s\n\n", 
        getCurrentTime(), 
        BLUE, RESET, 
        host, port
    );
    #endif

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
        printf("%s\n%s[socketOpen->ERROR]%s\n\tgetaddrinfo(%s:%s) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            host, port, GETSOCKETERRNO()
        );
        #endif
        
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*create socket*/
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tsocket(%s:%s) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            host, port, GETSOCKETERRNO()
        );
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
        printf("%s\n%s[socketOpen->ERROR]%s\n\tbind(%I64d) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
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
        printf("%s\n%s[socketOpen->ERROR]%s\n\tioctlsocket(%I64d, FIONBIO) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
        #endif

        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*reducing [a b] [c d] [e] -> [a b c d e]*/
    iResult = setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, TCP_NODELAY) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
        #endif

        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*ping <-> pong*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_KEEPALIVE) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
        #endif
        
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]<=*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvBufSize, sizeof(rcvBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_RCVBUF) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
        #endif
        
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*size of [..]=>*/
    iResult = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sndBufSize, sizeof(sndBufSize));
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tsetsockopt(%I64d, SO_SNDBUF) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
        #endif
        
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    /*wait clients*/
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        #ifdef _DEBUG
        printf("%s\n%s[socketOpen->ERROR]%s\n\tblisten(%I64d) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            listenSocket, GETSOCKETERRNO()
        );
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
    printf("%s\n%s[socketClose->CALL]%s\n\tsocketId = %I64d\n\n", 
        getCurrentTime(), 
        BLUE, RESET, 
        socketId
    );
    #endif

    mint result = true;

    if (socketId > 0) {
        result = CLOSESOCKET(socketId);
    }

    if (!result) {
        #ifdef _DEBUG
        printf("%s\n%s[socketClose->ERROR]%s\n\tsocketId = %I64d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            socketId
        );
        #endif

        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, result);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socket get addr info

//socketAddress[host, port]: addressPtr
DLLEXPORT int socketAddress(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char *host = MArgument_getUTF8String(Args[0]); // localhost by default
    char *port = MArgument_getUTF8String(Args[1]); // positive integer as string
    
    #ifdef _DEBUG
    printf("%s\n%s[socketAddress->CALL]%s\n\t%s:%s\n\n", 
        getCurrentTime(), 
        BLUE, RESET, 
        host, port
    );
    #endif

    int result;
    struct addrinfo *address = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(host, port, &hints, &address);
    if (result != 0){
        #ifdef _DEBUG
        printf("%s\n%s[socketConnect->ERROR]%s\n\tgetaddrinfo(%s:%s) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            host, port, GETSOCKETERRNO()
        );
        #endif
        
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);
    MArgument_setInteger(Res, address);
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

    #ifdef _DEBUG
    printf("%s\n%s[socketConnect->CALL]%s\n\tfor address %s:%s\n\n", 
        getCurrentTime(), 
        BLUE, RESET, 
        host, port
    );
    #endif

    int iResult;
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo *address = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(host, port, &hints, &address);
    if (iResult != 0){
        #ifdef _DEBUG
        printf("%s\n%s[socketConnect->ERROR]%s\n\tgetaddrinfo(%s:%s) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            host, port, GETSOCKETERRNO()
        );
        #endif
        
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        return LIBRARY_FUNCTION_ERROR;
    }

    connectSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (connectSocket == INVALID_SOCKET){
        #ifdef _DEBUG
        printf("%s\n%s[socketConnect->ERROR]%s\n\tsocket(%s:%s) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            host, port, GETSOCKETERRNO()
        );
        #endif

        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

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
        printf("%s\n%s[socketConnect->ERROR]%s\n\tioctlsocket(%I64d, FIONBIO) returns error = %d\n\n", 
            getCurrentTime(), 
            RED, RESET, 
            connectSocket, GETSOCKETERRNO()
        );
        #endif
        
        libData->UTF8String_disown(host);
        libData->UTF8String_disown(port);
        freeaddrinfo(address);
        CLOSESOCKET(connectSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = connect(connectSocket, address->ai_addr, (int)address->ai_addrlen);
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

    /*address no longer needed*/
    freeaddrinfo(address);
    libData->UTF8String_disown(host);
    libData->UTF8String_disown(port);

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

//socketSelect[socketList, length, timeout]: {socket2, ..}
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

        selectErrorMessage(libData, err);
        return LIBRARY_FUNCTION_ERROR;
    }
}

#pragma endregion

#pragma region socket check

//socketCheck[sockets, length]: validSockets
DLLEXPORT int socketCheck(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    MTensor socketList = MArgument_getMTensor(Args[0]); // list of sockets
    size_t length = (size_t)MArgument_getInteger(Args[1]); // number of sockets

    SOCKET *sockets = (SOCKET*)libData->MTensor_getIntegerData(socketList);
    SOCKET sock;
    mint validCount = 0;
    int opt;
    int err;
    socklen_t len = sizeof(opt);

    #ifdef _DEBUG
    printf("%s[socketCheck->CALL]%s\n\tcheck(",
        GREEN, RESET); 
    #endif

    for (size_t i = 0; i < length; i++) {
        sock = sockets[i];

        #ifdef _DEBUG
        printf("%I64d ", sock); 
        #endif

        #ifdef _WIN32
        int result = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char*)&opt, &len);
        #else
        int result = fcntl(sock, F_GETFL)
        #endif

        err = GETSOCKETERRNO();

        if (result >= 0 || 
            #ifdef _WIN32
            err != WSAENOTSOCK
            #else
            err != EBADF
            #endif
        ) {
            sockets[validCount] = sock;
            validCount++;
        }
    }

    #ifdef _DEBUG
    printf(")\n\n"); 
    #endif

    MTensor validSocketsList;
    libData->MTensor_new(MType_Integer, 1, &validCount, &validSocketsList);
    SOCKET *validSockets = libData->MTensor_getIntegerData(validSocketsList);

    #ifdef _DEBUG
    printf("%s[socketCheck->SUCCESS]%s\n\tcheck(",
        GREEN, RESET); 
    #endif

    for (mint i = 0; i < validCount; i++) {
        #ifdef _DEBUG
        printf("%I64d ", sockets[i]); 
        #endif
        validSockets[i] = sockets[i];
    }

    #ifdef _DEBUG
    printf(")\n\n"); 
    #endif
    
    MArgument_setMTensor(Res, validSocketsList);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socket accept

//socketAccept[listenSocket]: clientSocket
DLLEXPORT int socketAccept(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket = (SOCKET)MArgument_getInteger(Args[0]);
    SOCKET client = accept(listenSocket, NULL, NULL);
    if (client == INVALID_SOCKET){

        int err = GETSOCKETERRNO();
        #ifdef _DEBUG
        printf("%s[serverAccept->ERROR]%s\n\taccept(listenSocket = %I64d) returns error = %d\n\n", 
            RED, RESET, listenSocket, err);
        #endif

        acceptErrorMessage(libData, err);
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

//socketRecv[socketId, bufferSize]: byteArray
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
    }
    
    free(buffer);
    int err = GETSOCKETERRNO();
    
    #ifdef _DEBUG
    printf("%s[serverRecv->ERROR]%s\n\trecv(socket = %I64d) returns error = %d\n\n", 
        RED, RESET, client, err);
    #endif

    recvErrorMessage(libData, err);
    return LIBRARY_FUNCTION_ERROR;
}

#pragma endregion

#pragma region socket send

//socketSend[socketId, data, dataLength]: sentLength
DLLEXPORT int socketSend(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); // positive integer
    MNumericArray mArr = MArgument_getMNumericArray(Args[1]); 
    int dataLength = MArgument_getInteger(Args[2]); // positive integer

    int result;
    BYTE *data = (BYTE*)libData->numericarrayLibraryFunctions->MNumericArray_getData(mArr);

    result = send(socketId, (char*)data, dataLength, 0);
    if (result > 0) {
        #ifdef _DEBUG
        printf("%s\n%s[socketSend->SUCCESS]%s\n\tsend(socket id = %I64d) sent = %d bytes\n\n", 
            getCurrentTime(), GREEN, RESET, socketId, result);
        #endif

        MArgument_setInteger(Res, result);
        return LIBRARY_NO_ERROR;
    } 
    
    int err = GETSOCKETERRNO();

    #ifdef _DEBUG
    printf("%s[socketSend->ERROR]%s\n\tsend(socket id = %I64d) returns error = %d\n\n", 
        RED, RESET, socketId, err);
    #endif

    sendErrorMessage(libData, err);
    return LIBRARY_FUNCTION_ERROR;
}

#pragma endregion

#pragma region server data

//server data
typedef struct Server_st *Server;

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

//serverCreate[listenSocket, clientsCapacity, bufferSize, selectTimeout]: serverPtr
DLLEXPORT serverCreate(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket =    (SOCKET)MArgument_getInteger(Args[0]); // positive integer
    size_t clientsCapacity = (size_t)MArgument_getInteger(Args[1]); // 1024 by default
    size_t bufferSize =      (size_t)MArgument_getInteger(Args[2]); // 64 kB by default
    long timeout =           (long)MArgument_getInteger(Args[3]);   // 1 s by default

    #ifdef _DEBUG
    printf("%s[serverCreate->CALL]%s\n\tlistenSocket = %I64d\n\tclientsCapacity = %zd\n\tbufferSize = %zd\n\ttimeout = %ld\n\n", 
        BLUE, RESET, listenSocket, clientsCapacity, bufferSize, timeout);
    #endif

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
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(clients length = %zd) returns NULL\n\n", RED, RESET, clientsCapacity);
        #endif

        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    server->buffer = (char*)malloc(bufferSize * sizeof(char));
    if (!server->buffer){
        #ifdef _DEBUG
        printf("%s[serverCreate->ERROR]%s\n\tmalloc(buffer size = %zd) returns NULL\n\n", RED, RESET, bufferSize);
        #endif

        free(server->clients);
        free(ptr);
        return LIBRARY_FUNCTION_ERROR;
    }

    #ifdef _DEBUG
    printf("%s[serverCreate->SUCCESS]%s\n\tserver pointer = %p\n\tlisten socket id = %I64d\n\tclients length = %zd\n\tbuffer size = %zd\n\ttimeout = %ld sec and %ld usec\n\n",
        GREEN, RESET, ptr, listenSocket, clientsCapacity, bufferSize, tv.tv_sec, tv.tv_usec);
    #endif

    uint64_t serverPtr = (uint64_t)(uintptr_t)ptr;
    MArgument_setInteger(Res, serverPtr);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region server destroy

void serverDestroy(Server server){
    #ifdef _DEBUG
    printf("%s[serverDestroy->CALL]%s\n\tserver pointer = %p\n\tlisten socket id = %I64d\n\tclients length = %zd\n\tbuffer size = %zd\n\ttimeout = %ld sec and %ld usec\n\n",
        BLUE, RESET, server, server->listenSocket, server->clientsLength, server->bufferSize, server->timeout.tv_sec, server->timeout.tv_usec);
    #endif

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
    struct timeval *tv = &server->timeout;
    int maxFd = server->listenSocket;

    FD_ZERO(clientsReadSet);
    FD_SET(maxFd, clientsReadSet);
    SOCKET client;

    #ifdef _DEBUG
    printf("%s\n%s[serverSelect->CALL]%s\n\tselect(len = %zd, timeout = %ld) sockets = (%I64d",
        getCurrentTime(), BLUE, RESET, 
        server->clientsLength + 1, server->timeout.tv_sec * 1000000 + server->timeout.tv_usec, server->listenSocket); 
    #endif

    size_t count = server->clientsLength;
    for (size_t i = 0; i < count; i++)
    {
        client = server->clients[i];
        if (client != INVALID_SOCKET) {
            FD_SET(client, clientsReadSet);
            if (client > maxFd) maxFd = client;

            #ifdef _DEBUG
            printf(", %I64d", client);
            #endif
        }
    }

    #ifdef _DEBUG
    printf(")\n\n");
    #endif

    server->clientsReadSetLength = select(maxFd + 1, clientsReadSet, NULL, NULL, tv);
    if (server->clientsReadSetLength < 0) {
        int err = GETSOCKETERRNO();
        #ifdef _DEBUG
        printf("%s[serverSelect->ERROR]%s\n\tselect() returns error = %d\n\n", 
            RED, RESET, err); 
        #endif
    } else {
        #ifdef _DEBUG
        printf("%s[serverSelect->SUCCESS]%s\n\tselect() returns %zd sockets\n\n", 
            GREEN, RESET, server->clientsReadSetLength);
        #endif
    }
}

#pragma endregion

#pragma region server raise event

void serverRaiseEvent(Server server, const char *eventName, SOCKET client) {
    #ifdef _DEBUG
    printf("%s\n%s[socketRaiseEvent->CALL]%s\n\tlisten socket id = %I64d\n\tclient socket id = %I64d\n\n", 
        getCurrentTime(), BLUE, RESET, server->listenSocket, client);
    #endif

    DataStore data = server->libData->ioLibraryFunctions->createDataStore();
    server->libData->ioLibraryFunctions->DataStore_addInteger(data, server->listenSocket);
    server->libData->ioLibraryFunctions->DataStore_addInteger(data, client);
    server->libData->ioLibraryFunctions->raiseAsyncEvent(server->taskId, eventName, data);
}

#pragma endregion

#pragma region server raise data event

void serverRaiseDataEvent(Server server, const char *eventName, SOCKET client, BYTE *buffer, int len) {
    #ifdef _DEBUG
    printf("%s\n%s[socketRaiseDataEvent->CALL]%s\n\tlisten socket id = %I64d\n\tclient socket id = %I64d\n\treceived data length = %d\n\n", 
        getCurrentTime(), BLUE, RESET, server->listenSocket, client, len);
    #endif
    
    MNumericArray arr;
    server->libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &len, &arr);
    BYTE *arrData = server->libData->numericarrayLibraryFunctions->MNumericArray_getData(arr);
    memcpy(arrData, server->buffer, len);

    DataStore data = server->libData->ioLibraryFunctions->createDataStore();
    server->libData->ioLibraryFunctions->DataStore_addInteger(data, server->listenSocket);
    server->libData->ioLibraryFunctions->DataStore_addInteger(data, client);
    server->libData->ioLibraryFunctions->DataStore_addMNumericArray(data, arr);
    server->libData->ioLibraryFunctions->raiseAsyncEvent(server->taskId, eventName, data);
    server->libData->numericarrayLibraryFunctions->MNumericArray_disown(arr);
}

#pragma endregion

#pragma region server accept

void serverAccept(Server server){
    #ifdef _DEBUG
    printf("%s[serverAccept->CALL]%s\n\taccept(listenSocket = %I64d)\n\n", 
        BLUE, RESET, server->listenSocket);
    #endif

    if (server->clientsReadSetLength > 0 && FD_ISSET(server->listenSocket, &server->clientsReadSet)) {
        SOCKET client = accept(server->listenSocket, NULL, NULL);
        if (client != INVALID_SOCKET) {
            #ifdef _DEBUG
            printf("%s[serverAccept->SUCCESS]%s\n\taccept(listenSocket = %I64d) new client id = %I64d\n\n", 
                GREEN, RESET, server->listenSocket, client);
            #endif

            server->clients[server->clientsLength] = client;
            server->clientsLength++;
            serverRaiseEvent(server, "Accept", client);
        }
    }
}

#pragma endregion

#pragma region server recv

void serverRecv(Server server) {
    #ifdef _DEBUG
    printf("%s[socketRecv->CALL]%s\n\tlisten socket id = %I64d\n\tclients length = %zd\n\n", 
        BLUE, RESET, server->listenSocket, server->clientsLength);
    #endif
    
    if (server->clientsReadSetLength > 0) {
        size_t count = server->clientsLength;
        fd_set *readfds = &server->clientsReadSet;
        SOCKET client;
        int readyCount = server->clientsReadSetLength;
        int j = 0;

        for (size_t i = 0; i < count; i++) {
            client = server->clients[i];
            if (client != INVALID_SOCKET && FD_ISSET(client, readfds)) {
                j++;

                int result = recv(client, server->buffer, server->bufferSize, 0);
                int err = GETSOCKETERRNO();
                mint arrLen = (mint)result;
                if (result > 0) {
                    serverRaiseDataEvent(server, "Recv", client, server->buffer, result);
                } else if (result == 0 || (result < 0 && (err == 10038 || err == 10053))) {
                    server->clients[i] = INVALID_SOCKET;
                    serverRaiseEvent(server, "Close", client);
                } else {
                    printf("recv unexpected\n\n");
                    serverRaiseEvent(server, "Unexpected", client);
                }
            }

            if (j > readyCount) break;
        }
    }
}

#pragma endregion

#pragma region server check

void serverCheck(Server server) {
    #ifdef _DEBUG
    printf("%s[serverCheck->CALL]%s\n\tlisten socket id = %I64d\n\tclients length = %zd\n\n", 
        BLUE, RESET, 
        server->listenSocket, server->clientsLength);
    #endif
    
    if (server->clientsReadSetLength <= 0) {
        SOCKET *sockets = server->clients;
        size_t count = server->clientsLength;
        SOCKET sock;
        mint validCount = 0;
        
        int opt;
        int err;
        socklen_t len = sizeof(opt);

        for (size_t i = 0; i < count; i++) {
            sock = sockets[i];

            if (sock > 0) {
                #ifdef _WIN32
                int result = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char*)&opt, &len);
                #else
                int result = fcntl(sock, F_GETFL)
                #endif

                err = GETSOCKETERRNO();
                if (result >= 0 || 
                    #ifdef _WIN32
                    err != WSAENOTSOCK
                    #else
                    err != EBADF
                    #endif
                ) {
                    sockets[validCount] = sock;
                    validCount++;
                }
            }
        }

        server->clientsLength = validCount;
    }
}

#pragma endregion

#pragma region server listener task

static void serverListenerTask(mint taskId, void* vtarg)
{
    Server server = (Server)vtarg;
    
    #ifdef _DEBUG
    printf("%s[serverListenerTask->CALL]%s\n\tlisten socket id = %I64d\n\ttask id = %I64d\n\n", 
        BLUE, RESET, server->listenSocket, taskId);
    #endif
    
    while (server->libData->ioLibraryFunctions->asynchronousTaskAliveQ(taskId))
    {
        serverSelect(server);
        serverCheck(server);
        serverAccept(server);
        serverRecv(server);
    }
    serverDestroy(server);
}

//socketListen[serverPtr_Integer]: taskId_Integer
DLLEXPORT int serverListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    Server server = (Server)MArgument_getInteger(Args[0]);
    mint taskId;

    taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(serverListenerTask, server);
    server->taskId = taskId;

    #ifdef _DEBUG
    printf("%s[socketListen->CALL]%s\n\tlisten socket id = %I64d in taks with id = %I64d\n\n", BLUE, RESET, server->listenSocket, taskId);
    #endif

    MArgument_setInteger(Res, taskId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion