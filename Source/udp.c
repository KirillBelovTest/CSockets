#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

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
    SLEEP(1000 * ms);
    #endif

    return;
}

#else // Assume POSIX
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

// Listen specific port
DLLEXPORT int udpSocketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int port = MArgument_getInteger(Args[0]);

    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket create error");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Socket bind error");
        #ifdef _WIN32
        closesocket(sockfd);
        #else
        close(sockfd);
        #endif
        return -1;
    }

    MArgument_setInteger(Res, sockfd);
    return LIBRARY_NO_ERROR; 
}

MNumericArray createByteArray(WolframLibraryData libData, BYTE *data, const mint dataLength){
    MNumericArray nArray;
    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &dataLength, &nArray);

    BYTE *array = libData->numericarrayLibraryFunctions->MNumericArray_getData(nArray);
    memcpy(array, data, dataLength); 
    return nArray;
}

// Read from the socket
DLLEXPORT int udpSocketRead(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int sockId = MArgument_getInteger(Args[0]);
    size_t bufferSize = MArgument_getInteger(Args[1]); 
    
    char *buffer; 

    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    int n = recvfrom(sockId, buffer, bufferSize, 0, (struct sockaddr *)&cliaddr, &len);

    if (n < 0) {
        return LIBRARY_FUNCTION_ERROR; // reading error
    } else if (n == 0) {
        return LIBRARY_FUNCTION_ERROR; // connection closed
    }

    buffer[n] = '\0';

    MArgument_setMNumericArray(Res, createByteArray(libData, buffer, n)); 

    return LIBRARY_NO_ERROR; 
}

// Connect to specific host
DLLEXPORT int udpSocketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char* host = MArgument_getUTF8String(Args[0]); 
    int port = MArgument_getInteger(Args[1]); 

    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket create error");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        perror("Wrong host address");
        #ifdef _WIN32
        closesocket(sockfd);
        #else
        close(sockfd);
        #endif
        return LIBRARY_FUNCTION_ERROR;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Server connection error");
        #ifdef _WIN32
        closesocket(sockfd);
        #else
        close(sockfd);
        #endif
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, sockfd); 
    
    libData->UTF8String_disown(host); 
    
    return LIBRARY_NO_ERROR;
}

// Sending data using the socket
DLLEXPORT int udpSocketSend(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int sockId = MArgument_getInteger(Args[0]);
    MNumericArray marr = MArgument_getMNumericArray(Args[1]);
    int dataLength = MArgument_getInteger(Args[2]);

    char *data = (BYTE *)libData->numericarrayLibraryFunctions->MNumericArray_getData(marr);

    int n = send(sockId, data, dataLength, 0);
    if (n < 0) {
        return LIBRARY_FUNCTION_ERROR; // writeing error
    }

    MArgument_setInteger(Res, n);  

    return LIBRARY_NO_ERROR; // sent data length
}

// Close the socket
DLLEXPORT int udpSocketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int sockId = MArgument_getInteger(Args[0]);

    #ifdef _WIN32
    if (closesocket(sockId) < 0) {
        fprintf(stderr, "Socket close error: %d\n", WSAGetLastError());
        return LIBRARY_FUNCTION_ERROR;
    }
    #else
    if (close(sockId) < 0) {
        perror("Socket close error");
        return LIBRARY_FUNCTION_ERROR;
    }
    #endif

    MArgument_setInteger(Res, sockId); 
    return LIBRARY_NO_ERROR;
}

// ready for read
DLLEXPORT int udpSocketReadReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int sockId = MArgument_getInteger(Args[0]);
    int timeoutUSec = MArgument_getInteger(Args[1]);

    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockId, &readfds);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutUSec;

    int result = select(sockId + 1, &readfds, NULL, NULL, &timeout);
    if (result < 0) {
        perror("select error");
        return -1;
    } else if (result == 0) {
        return 0; // timeout, socket not ready
    }

    MArgument_setInteger(Res, FD_ISSET(sockId, &readfds) ? 1 : 0);
    return LIBRARY_NO_ERROR; 
}

// ready for write
DLLEXPORT int udpSocketWriteReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int sockId = MArgument_getInteger(Args[0]);
    int timeoutUSec = MArgument_getInteger(Args[1]);
    
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&writefds);
    FD_SET(sockId, &writefds);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutUSec;

    int result = select(sockId + 1, NULL, &writefds, NULL, &timeout);
    if (result < 0) {
        perror("select error");
        return -1;
    } else if (result == 0) {
        return 0; // timeout, socket not ready
    }

    MArgument_setInteger(Res, FD_ISSET(sockId, &writefds) ? 1 : 0);
    return LIBRARY_NO_ERROR; 
}