#pragma region header

#undef UNICODE
#define BUFLEN 8192

#ifdef __linux__ 
    #include <string.h>
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #define INVALID_SOCKET -1
    #define NO_ERROR 0
    #define SOCKET_ERROR -1
    #define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
    inline void nopp() {}
#elif _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SLEEP Sleep
#else
    #include <string.h>
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <wchar.h>
    #define INVALID_SOCKET -1
    #define NO_ERROR 0
    #define SOCKET_ERROR -1
    #define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
    #define SLEEP sleep
    inline void nopp() {}
#endif

#if defined(_WIN32)
    #define SOCKET SOCKET
    #define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
    #define GETSOCKETERRNO() (WSAGetLastError())
#else
    #define SOCKET int
    #define ISVALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define GETSOCKETERRNO() (errno)
    #define BYTE uint8_t
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef __linux__

#elif _WIN32
    #pragma comment (lib, "Ws2_32.lib")
#else

#endif

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

#pragma endregion

#pragma region initialization

DLLEXPORT mint WolframLibrary_getVersion() {
    printf("[WolframLibrary_getVersion] \n\tcurrent version: %d\n\n", WolframLibraryVersion); 
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    #ifdef _WIN32
        int iResult; 
        WSADATA wsaData; 

        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            printf("[WolframLibrary_initialize] \n\tWSAStartup failed with error: %d\n\n", iResult);
            return LIBRARY_FUNCTION_ERROR;
        }
    #endif

    printf("[WolframLibrary_initialize] \n\tlibrary initialized\n\n"); 
    return LIBRARY_NO_ERROR;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    #ifdef _WIN32
        WSACleanup(); 
    #endif 

    printf("[WolframLibrary_uninitialize] \n\tlibrary uninitialized\n\n"); 
    return; 
}

#pragma endregion

#pragma region socketOpen[portName_String] -> socketId_Integer 

DLLEXPORT int socketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    char* listenPortName = MArgument_getUTF8String(Args[0]); 
    SOCKET listenSocket = INVALID_SOCKET; 
    
    struct addrinfo *address = NULL; 
    struct addrinfo addressHints; 

    ZeroMemory(&addressHints, sizeof(addressHints));
    addressHints.ai_family = AF_INET;
    addressHints.ai_socktype = SOCK_STREAM;
    addressHints.ai_protocol = IPPROTO_TCP;
    addressHints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, listenPortName, &addressHints, &address); 
    if (iResult != 0) {
        libData->Message("getaddrinfo failed with error: %d\n", iResult);
        return LIBRARY_FUNCTION_ERROR;
    }
    
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        libData->Message("socket failed with error: %d\n", GETSOCKETERRNO());
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        libData->Message("bind failed with error: %d\n", GETSOCKETERRNO());
        freeaddrinfo(address);
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    freeaddrinfo(address);
    MArgument_setInteger(Res, listenSocket);
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketConnect[hostName_String, portName_String] -> socketId_Integer 

DLLEXPORT int socketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    char *host = MArgument_getUTF8String(Args[0]);
    char *port = MArgument_getUTF8String(Args[1]);
    struct addrinfo *addr = NULL, hints;
    int iResult;

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(host, port, &hints, &addr);
    if (iResult != 0) {
        printf("[SocketConnect ERROR]\n\tgetaddrinfo failed with error: %d\n\n", iResult);
        return LIBRARY_FUNCTION_ERROR;
    }

    ConnectSocket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol); 
    if (ConnectSocket == INVALID_SOCKET) {
        printf("[SocketConnect ERROR]\n\tError at socket(): %ld\n\n", WSAGetLastError());
        freeaddrinfo(addr);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = connect(ConnectSocket, addr->ai_addr, (int)addr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }
    
    freeaddrinfo(addr);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("[SocketConnect ERROR]\n\tUnable to connect to server!\n\n");
        return LIBRARY_FUNCTION_ERROR;
    }

    printf("[SocketConnect]\n\tsocket %d connected to %s:%s\n\n", ConnectSocket, host, port); 
    MArgument_setInteger(Res, ConnectSocket); 
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketWrite[socketId_Integer, data: ByteArray[<>], dataLength_Integer, bufferLength_Integer] -> socketId_Integer

int _socketWrite(SOCKET socketId, BYTE *data, int dataLength, int bufLen){
    int iResult; 
    int writeLength; 
    BYTE *buf; 

    for (int i = 0; i < dataLength; i += bufLen){
        buf = &data[i]; 
        writeLength = dataLength - i > bufLen ? bufLen : dataLength - i; 

        iResult = send(socketId, buf, writeLength, 0); 
        if (iResult == SOCKET_ERROR) {
            return SOCKET_ERROR; 
        }
    }

    free(buf); 
    return dataLength; 
}

DLLEXPORT int socketWrite(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    MNumericArray mArr = MArgument_getMNumericArray(Args[1]); 
    BYTE *data = libData->numericarrayLibraryFunctions->MNumericArray_getData(mArr); 
    int dataLength = MArgument_getInteger(Args[2]); 
    int bufLen = MArgument_getInteger(Args[3]); 
    
    iResult = _socketWrite(clientId, data, dataLength, bufLen); 
    if (iResult == SOCKET_ERROR) {
        wprintf("[SocketWrite]\n\tsend failed with error: %d\n\n", GETSOCKETERRNO());
        CLOSESOCKET(clientId);
        MArgument_setInteger(Res, GETSOCKETERRNO()); 
        return LIBRARY_FUNCTION_ERROR; 
    }
    
    printf("[SocketWrite]\n\twrite %d bytes\n\n", dataLength);
    MArgument_setInteger(Res, clientId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadyQ[socketId_Integer] -> True | False 

int _socketReadyQ(SOCKET socketId){
    int iResult; 
    BYTE *buf; 
    
    iResult = recv(socketId, buf, 1, MSG_PEEK);
    
    if (iResult == SOCKET_ERROR){
        return False; 
    }

    return True; 
}

DLLEXPORT int socketReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); 
    MArgument_setBoolean(Res, _socketReadyQ(socketId)); 
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadMessage[socketId_Integer, bufferLength_Integer, maxMessageLength_Integer] -> ByteArray[<>] 

int _socketReadMessage(SOCKET socketId, BYTE *buf, int bufLen, int maxMessageLen){
    int iResult; 
    int len = 0; 
    int ptr = 0; 
    buf = (BYTE*)malloc(buf, bufLen * sizeof(BYTE)); 

    do
    {
        iResult = recv(socketId, &buf[ptr], bufLen, 0); 
        if (iResult > 0) {
            len += iResult; 
            if (iResult == bufLen){
                realloc(buf, len + bufLen); 
            }
        }

    } while (iResult > 0 || len < maxMessageLen); 

    return len; 
    
}

DLLEXPORT int socketReadMessage(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); 
    SOCKET bufLen = MArgument_getInteger(Args[1]); 
    SOCKET maxMessageLen = MArgument_getInteger(Args[2]); 
    
    BYTE *buf; 
    MNumericArray nArray; 

    int iResult; 

    iResult = _socketReadMessage(socketId, buf, bufLen, maxMessageLen); 
    if (iResult == 0) {
        return LIBRARY_FUNCTION_ERROR; 
    }

    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_Bit8, 1, &iResult, &nArray); 
    memcpy((uint8_t*) libData->numericarrayLibraryFunctions->MNumericArray_getData(nArray), buf, iResult); 

    MArgument_setMNumericArray(Res, nArray); 
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketClose[socketId_Integer] -> socketId_Integer

DLLEXPORT int SocketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);
    printf("[CloseSocket]\n\tsocketId = %d\n\n", socketId);
    MArgument_setInteger(Res, CLOSESOCKET(socketId));
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketListen[socketid_Integer] -> taskId_Integer

typedef struct socket_task_args_st {
    WolframLibraryData libData; 
    SOCKET listentSocket; 
}* socket_task_args; 

static void SocketListenerTask(mint taskId, void* vtarg)
{
    SOCKET *clients = (SOCKET*)malloc(4 * sizeof(SOCKET));
    int clientsLength = 0; 
    int clientsMaxLength = 4; 

    int iResult; 
    int iMode = 1; 

    size_t buflen = BUFLEN; 
    BYTE *buf = malloc(BUFLEN * sizeof(BYTE));
    mint dims[1]; 
    MNumericArray data; 

    SOCKET clientSocket = INVALID_SOCKET; 
	socket_task_args targ = (socket_task_args)vtarg; 
    SOCKET listenSocket = targ->listentSocket; 
	WolframIOLibrary_Functions ioLib = targ->libData->ioLibraryFunctions; 
    WolframNumericArrayLibrary_Functions numLib = targ->libData->numericarrayLibraryFunctions; 

	DataStore ds; 
    free(targ); 
    
    #ifdef _WIN32 
        iResult = ioctlsocket(listenSocket, FIONBIO, &iMode); 
    #else
        iResult = fcntl(listenSocket, O_NONBLOCK, &iMode); 
    #endif

    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %d\n", iResult);
    }
	
	while(ioLib->asynchronousTaskAliveQ(taskId))
	{
        SLEEP(1);

        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            printf("NEW CLIENT: %d\n", clientSocket);
            clients[clientsLength++] = clientSocket; 

            if (clientsLength == clientsMaxLength){
                clientsMaxLength *= 2; 
                clients = (SOCKET*)realloc(clients, clientsMaxLength * sizeof(SOCKET)); 
            }
        }

        for (size_t i = 0; i < clientsLength; i++)
        {
            iResult = recv(clients[i], buf, buflen, 0); 
            if (iResult > 0){            
                printf("CURRENT NUMBER OF CLIENTS: %d\n", clientsLength);
                printf("MAX NUMBER OF CLIENTS: %d\n", clientsMaxLength);
                printf("RECEIVED %d BYTES\n", iResult);
                dims[0] = iResult; 
                numLib->MNumericArray_new(MNumericArray_Type_UBit8, 1, dims, &data); 
                memcpy(numLib->MNumericArray_getData(data), buf, iResult);
                
                ds = ioLib->createDataStore();
                ioLib->DataStore_addInteger(ds, listenSocket);
                ioLib->DataStore_addInteger(ds, clients[i]);
                ioLib->DataStore_addMNumericArray(ds, data);

                ioLib->raiseAsyncEvent(taskId, "RECEIVED_BYTES", ds);
            }
        }
	}

    printf("STOP ASYNCHRONOUS TASK %d\n", taskId); 
    for (size_t i = 0; i < clientsLength; i++)
    {
        CLOSESOCKET(clients[i]);
    }
    CLOSESOCKET(listenSocket);

    free(clients);
    free(buf);
}

DLLEXPORT int SocketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    SOCKET listenSocket = MArgument_getInteger(Args[0]); 
    mint taskId;

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("error during call listen(%d)", listenSocket);
        libData->Message("listenerror");
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    socket_task_args threadArg = (socket_task_args)malloc(sizeof(struct socket_task_args_st));
    threadArg->libData=libData; 
    threadArg->listentSocket=listenSocket;
    taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(SocketListenerTask, threadArg);

    MArgument_setInteger(Res, taskId); 
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketListenerTaskRemove[taskId_Integer] -> taskId_Integer 

DLLEXPORT int SocketListenerTaskRemove(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    mint taskId = MArgument_getInteger(Args[0]); 
    MArgument_setInteger(Res, libData->ioLibraryFunctions->removeAsynchronousTask(taskId)); 
    return LIBRARY_NO_ERROR;
}

#pragma endregion
