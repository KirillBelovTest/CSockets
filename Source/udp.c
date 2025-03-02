#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SOCKET_TYPE;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
typedef int SOCKET_TYPE;
#endif

//#include "wstp.h"
#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

#define MAX_BUFFER_SIZE 8192  // Set maximum buffer size as 8KB

DLLEXPORT mint WolframLibrary_getVersion() {
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    #ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) return LIBRARY_FUNCTION_ERROR;
    #endif
    return LIBRARY_NO_ERROR;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    #ifdef _WIN32
    WSACleanup();
    #endif
}

// Common function to close sockets
int closeSocket(SOCKET_TYPE sockId) {
    #ifdef _WIN32
    return closesocket(sockId);
    #else
    return close(sockId);
    #endif
}

// Function to rise an event when socket is ready for reading
/*void riseEvent(WolframLibraryData libData, SOCKET_TYPE sockfd) {
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    char *buffer = (char *)malloc(MAX_BUFFER_SIZE);
    if (buffer == NULL) {
        return LIBRARY_FUNCTION_ERROR;
    }

    int n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        perror("Error reading from socket");
        free(buffer);
        return LIBRARY_FUNCTION_ERROR;
    } else if (n == 0) {
        free(buffer);
        return LIBRARY_FUNCTION_ERROR;
    }
    
    WSLINK link = libData->getWSLINK(libData);
    //WSPutFunction(link, "EvaluatePacket", 1);
    WSPutFunction(link, "EvaluatePacket", 1);
    WSPutFunction(link, "Echo", 1);
    WSPutArray(link, buffer);
    WSEndPacket(link);
    WSFlush(link);

    libData->processWSLINK(link); 
    
    WSNextPacket(link); 
    WSNewPacket(link);
}*/

// Structure for passing data to the thread function
typedef struct {
    SOCKET_TYPE sockfd;
    WolframLibraryData libData;
    int timeoutUSec;
    volatile int* stopFlag;  // Pointer to a stop flag
} ThreadArgs;

// Thread function for asynchronous socket read check
static void socketReadCheckThread(mint taskId, void* args) 
{
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    fd_set readfds;
    struct timeval timeout;
    WolframLibraryData libData = threadArgs->libData;
    int sockfd = threadArgs->sockfd;

    DataStore ds;
    mint dims[1];
    MNumericArray data;
    char *buffer = (char*)malloc(MAX_BUFFER_SIZE * sizeof(char));

    while (libData->ioLibraryFunctions->asynchronousTaskAliveQ(taskId)) {
        FD_ZERO(&readfds);
        FD_SET(threadArgs->sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = threadArgs->timeoutUSec;

        int result = select(threadArgs->sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(threadArgs->sockfd, &readfds)) {
            
            struct sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);

            int n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&cliaddr, &len);
            if (n < 0) {
                perror("Error reading from socket");
                free(buffer);
                return LIBRARY_FUNCTION_ERROR;
            } else if (n == 0) {
                free(buffer);
                return LIBRARY_FUNCTION_ERROR;
            }

            dims[0] = n;
            libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, dims, &data);
            memcpy(libData->numericarrayLibraryFunctions->MNumericArray_getData(data), buffer, n);

            buffer[n] = '\0';

            ds = libData->ioLibraryFunctions->createDataStore();
            libData->ioLibraryFunctions->DataStore_addInteger(ds, sockfd);
            libData->ioLibraryFunctions->DataStore_addMNumericArray(ds, data);
            libData->ioLibraryFunctions->raiseAsyncEvent(taskId, "Received", ds);
        }

        #ifdef _WIN32
        Sleep(10); // Sleep for 10 milliseconds
        #else
        struct timespec req = {0, 10 * 1000000L}; // 10 milliseconds in nanoseconds
        nanosleep(&req, NULL);
        #endif
    }

    free(threadArgs);  // Clean up allocated memory
    return 0;
}

DLLEXPORT int startAsyncSocketReadCheck(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }

    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    int timeoutUSec = MArgument_getInteger(Args[1]);
    
    if (sockId < 0) {
        return LIBRARY_FUNCTION_ERROR;
    }

    volatile int* stopFlag = (volatile int*)malloc(sizeof(volatile int));
    if (stopFlag == NULL) {
        return LIBRARY_FUNCTION_ERROR;
    }
    *stopFlag = 0; // Initialize the flag to 0 (running)

    ThreadArgs* threadArgs = (ThreadArgs*)malloc(sizeof(ThreadArgs));
    if (threadArgs == NULL) {
        free((void*)stopFlag);
        return LIBRARY_FUNCTION_ERROR;
    }

    threadArgs->sockfd = sockId;
    threadArgs->libData = libData;
    threadArgs->timeoutUSec = timeoutUSec;
    threadArgs->stopFlag = stopFlag;

    int taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(socketReadCheckThread, threadArgs);

    MArgument_setInteger(Res, taskId);  // Return the taskid
    return LIBRARY_NO_ERROR;
}

DLLEXPORT int stopAsyncSocketReadCheck(mint Argc, MArgument *Args, MArgument Res) {
    if (Argc < 1) {
        return LIBRARY_FUNCTION_ERROR;
    }

    void* stopFlagPtr = (void*)(uintptr_t)MArgument_getInteger(Args[0]);
    if (stopFlagPtr != NULL) {
        volatile int* stopFlag = (volatile int*)stopFlagPtr;
        *stopFlag = 1;  // Set the flag to signal the thread to stop
        free((void*)stopFlag);
        return LIBRARY_NO_ERROR;
    }
    
    return LIBRARY_FUNCTION_ERROR;
}

// Listen to a specific host and port
DLLEXPORT int udpSocketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }

    const char *host = MArgument_getUTF8String(Args[0]);
    int port = MArgument_getInteger(Args[1]);
    SOCKET_TYPE sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket create error");
        return LIBRARY_FUNCTION_ERROR;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        closeSocket(sockfd);
        return LIBRARY_FUNCTION_ERROR;
    }

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Socket bind error");
        closeSocket(sockfd);
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, sockfd);
    libData->UTF8String_disown((void*)host);
    return LIBRARY_NO_ERROR;
}

MNumericArray createByteArray(WolframLibraryData libData, char *data, const mint dataLength){
    MNumericArray nArray;
    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &dataLength, &nArray);

    char *array = (char *)libData->numericarrayLibraryFunctions->MNumericArray_getData(nArray);
    memcpy(array, data, dataLength); 
    return nArray;
}

// Read from the socket
DLLEXPORT int udpSocketRead(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }

    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    if (sockId < 0) {
        return LIBRARY_FUNCTION_ERROR;
    }

    size_t bufferSize = (size_t)MArgument_getInteger(Args[1]); 

    if (bufferSize <= 0 || bufferSize > MAX_BUFFER_SIZE) {
        return LIBRARY_FUNCTION_ERROR;
    }

    char *buffer = (char *)malloc(bufferSize);
    if (buffer == NULL) {
        return LIBRARY_FUNCTION_ERROR;
    }

    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    int n = recvfrom(sockId, buffer, bufferSize - 1, 0, (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        perror("Error reading from socket");
        free(buffer);
        return LIBRARY_FUNCTION_ERROR;
    } else if (n == 0) {
        free(buffer);
        return LIBRARY_FUNCTION_ERROR;
    }

    buffer[n] = '\0';
    MArgument_setMNumericArray(Res, createByteArray(libData, buffer, n)); 

    free(buffer);
    return LIBRARY_NO_ERROR;
}

// Connect to a specific host
DLLEXPORT int udpSocketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }
    
    char* host = MArgument_getUTF8String(Args[0]); 
    int port = MArgument_getInteger(Args[1]); 

    SOCKET_TYPE sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket create error");
        libData->UTF8String_disown(host);
        return LIBRARY_FUNCTION_ERROR;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        perror("Wrong host address");
        closeSocket(sockfd);
        libData->UTF8String_disown(host);
        return LIBRARY_FUNCTION_ERROR;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Server connection error");
        closeSocket(sockfd);
        libData->UTF8String_disown(host);
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, sockfd); 
    libData->UTF8String_disown(host);
    
    return LIBRARY_NO_ERROR;
}

// Sending data using the socket
DLLEXPORT int udpSocketSend(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    if (Argc < 3) {
        return LIBRARY_FUNCTION_ERROR;
    }

    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    if (sockId < 0) {
        return LIBRARY_FUNCTION_ERROR;
    }

    MNumericArray marr = MArgument_getMNumericArray(Args[1]);
    int dataLength = MArgument_getInteger(Args[2]);

    char *data = (char *)libData->numericarrayLibraryFunctions->MNumericArray_getData(marr);
    if (data == NULL) {
        return LIBRARY_FUNCTION_ERROR;
    }

    if (dataLength > libData->numericarrayLibraryFunctions->MNumericArray_getFlattenedLength(marr)) {
        return LIBRARY_FUNCTION_ERROR;
    }

    int n = send(sockId, data, dataLength, 0);
    if (n < 0) {
        perror("Socket data sending error");
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, n);
    return LIBRARY_NO_ERROR;
}

// Close the socket
DLLEXPORT int udpSocketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    if (Argc < 1) {
        return LIBRARY_FUNCTION_ERROR;
    }
    
    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    if (closeSocket(sockId) < 0) {
        #ifdef _WIN32
        fprintf(stderr, "Socket close error: %d\n", WSAGetLastError());
        #else
        perror("Socket close error");
        #endif
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, sockId); 
    return LIBRARY_NO_ERROR;
}

// 1 - ready for read; 0 - not ready; else - lib error
DLLEXPORT int udpSocketReadReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }

    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    if (sockId < 0) {
        return LIBRARY_FUNCTION_ERROR;
    }

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
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, (result > 0 && FD_ISSET(sockId, &readfds)) ? 1 : 0);
    return LIBRARY_NO_ERROR; 
}

// ready for write
DLLEXPORT int udpSocketWriteReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){    
    if (Argc < 2) {
        return LIBRARY_FUNCTION_ERROR;
    }

    SOCKET_TYPE sockId = MArgument_getInteger(Args[0]);
    if (sockId < 0) {
        return LIBRARY_FUNCTION_ERROR;
    }

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
        return LIBRARY_FUNCTION_ERROR;
    }

    MArgument_setInteger(Res, (result > 0 && FD_ISSET(sockId, &writefds)) ? 1 : 0);
    return LIBRARY_NO_ERROR; 
}