#pragma region header 

#undef UNICODE

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

#ifdef _WIN32
    #pragma comment (lib, "Ws2_32.lib")
#endif

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

#pragma endregion

#pragma region initialization 

DLLEXPORT mint WolframLibrary_getVersion() {
    printf("[WolframLibrary_getVersion]\nlibrary version: %d\n\n", WolframLibraryVersion);
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    #ifdef _WIN32
        int iResult; 
        WSADATA wsaData; 

        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            return LIBRARY_FUNCTION_ERROR;
        }
    #endif

    printf("[WolframLibrary_initialize]\ninitialized\n\n"); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) { 
    #ifdef _WIN32
        WSACleanup(); 
    #endif 

    printf("[WolframLibrary_uninitialize]\nuninitialized\n\n"); 
    return; 
}

#pragma endregion

#pragma region internal 

static void socketListenerTask(mint taskId, void* vtarg); 

int socketWrite(SOCKET socketId, BYTE *data, int dataLength, int bufferSize){
    int iResult;
    int writeLength;
    char *buffer;

    for (int i = 0; i < dataLength; i += bufferSize){
        buffer = (char*)&data[i]; 
        writeLength = dataLength - i > bufferSize ? bufferSize : dataLength - i; 

        iResult = send(socketId, buffer, writeLength, 0);
        if (iResult == SOCKET_ERROR) {
            if (GETSOCKETERRNO() == 10035) {
                SLEEP(2);
                printf("[SocketWrite]\nerror 10035\n\n");
                i -= bufferSize;
            } else 
            {
                return SOCKET_ERROR;
            }
        }
    }

    return dataLength;
}

MNumericArray createByteArray(WolframLibraryData libData, BYTE *data, const mint dataLength){
    MNumericArray nArray;
    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, &dataLength, &nArray);
    memcpy((uint8_t*) libData->numericarrayLibraryFunctions->MNumericArray_getData(nArray), data, dataLength);
    return nArray;
}

typedef struct Server_st {
    SOCKET listenSocket;
    SOCKET *clients;
    int clientsLength;
    int clientsLengthMax;
    int bufferSize;
}* Server;

typedef struct SocketListenerTaskArgs_st {
    WolframLibraryData libData; 
    Server server;
}* SocketListenerTaskArgs; 

#pragma endregion

#pragma region socketOpen[host_String, port_String]: socketId_Integer 

DLLEXPORT int socketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    char* host = MArgument_getUTF8String(Args[0]);
    char* port = MArgument_getUTF8String(Args[1]);
    
    int iResult; 
    SOCKET listenSocket = INVALID_SOCKET; 
    struct addrinfo hints; 
    struct addrinfo *address = NULL; 
    int iMode = 1;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(host, port, &hints, &address);
    if (iResult != 0) {
        printf("[socketOpen]\ngetaddrinfo error: %d\n\n", iResult);
        return LIBRARY_FUNCTION_ERROR;
    }

    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        printf("[socketOpen]\nsocket error: %d\n\n", (int)GETSOCKETERRNO());
        freeaddrinfo(address);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("[socketOpen]\nbind error: %d\n\n", (int)GETSOCKETERRNO());
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("[socketOpen]\nerror during call listen(%d)\n\n", (int)listenSocket);
        CLOSESOCKET(listenSocket);
        return LIBRARY_FUNCTION_ERROR;
    }

    ioctlsocket(listenSocket, FIONBIO, &iMode);
    freeaddrinfo(address);

    printf("[socketOpen]\nopened socket id: %d\n\n", (int)listenSocket);
    MArgument_setInteger(Res, listenSocket);
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketClose[socketId_Integer]: socketId_Integer 

DLLEXPORT int socketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);
    printf("[socketClose]\nsocket id: %d\n\n", (int)socketId);
    MArgument_setInteger(Res, CLOSESOCKET(socketId));
    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketListen[socketid_Integer, bufferSize_Integer]: taskId_Integer 

DLLEXPORT int socketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET listenSocket = MArgument_getInteger(Args[0]);
    int bufferSize = MArgument_getInteger(Args[1]);
    
    mint taskId;
    SOCKET *clients = (SOCKET*)malloc(4 * sizeof(SOCKET));
    Server server = (Server)malloc(sizeof(struct Server_st));

    server->listenSocket=listenSocket;
    server->clients=clients;
    server->clientsLength=0;
    server->clientsLengthMax=4;
    server->bufferSize=bufferSize;

    SocketListenerTaskArgs threadArg = (SocketListenerTaskArgs)malloc(sizeof(struct SocketListenerTaskArgs_st));
    threadArg->libData=libData; 
    threadArg->server=server; 
    taskId = libData->ioLibraryFunctions->createAsynchronousTaskWithThread(socketListenerTask, threadArg);

    printf("[socketListen]\nlistening task id: %d\n\n", (int)taskId);
    MArgument_setInteger(Res, taskId); 
    return LIBRARY_NO_ERROR; 
}

static void socketListenerTask(mint taskId, void* vtarg)
{
    SocketListenerTaskArgs targ = (SocketListenerTaskArgs)vtarg;
    Server server = targ->server;
	WolframLibraryData libData = targ->libData;

    int iResult;
    SOCKET clientSocket = INVALID_SOCKET;
    char *buffer = (char*)malloc(server->bufferSize * sizeof(char));
    mint dims[1];
    MNumericArray data;
	DataStore ds;
    
	while(libData->ioLibraryFunctions->asynchronousTaskAliveQ(taskId))
	{
        SLEEP(1);
        clientSocket = accept(server->listenSocket, NULL, NULL); 
        if (ISVALIDSOCKET(clientSocket)) {
            printf("[socketListenerTask]\nnew client: %d\n\n", (int)clientSocket);
            server->clients[server->clientsLength] = clientSocket;
            server->clientsLength++;
            printf("[socketListenerTask]\nclients length: %d\n\n", (int)server->clientsLength);
        
            if (server->clientsLength == server->clientsLengthMax) {
                server->clientsLengthMax *= 2; 
                server->clients = realloc(server->clients, server->clientsLengthMax * sizeof(SOCKET)); 
            }
        }

        for (int i = 0; i < server->clientsLength; i++)
        {
            if (server->clients[i] != INVALID_SOCKET) {
                iResult = recv(server->clients[i], buffer, server->bufferSize, 0); 
                if (iResult > 0){
                    printf("[socketListenerTask]\nrecv %d bytes from %d\n\n", iResult, (int)server->clients[i]);
                    dims[0] = iResult;
                    libData->numericarrayLibraryFunctions->MNumericArray_new(MNumericArray_Type_UBit8, 1, dims, &data); 
                    memcpy(libData->numericarrayLibraryFunctions->MNumericArray_getData(data), buffer, iResult);
                    ds = libData->ioLibraryFunctions->createDataStore();
                    libData->ioLibraryFunctions->DataStore_addInteger(ds, server->listenSocket);
                    libData->ioLibraryFunctions->DataStore_addInteger(ds, server->clients[i]);
                    libData->ioLibraryFunctions->DataStore_addMNumericArray(ds, data);
                    libData->ioLibraryFunctions->raiseAsyncEvent(taskId, "Received", ds);
                } else if (iResult == 0) {
                    printf("[socketListenerTask]\nclient %d closed\n\n", (int)server->clients[i]);
                    server->clients[i] = INVALID_SOCKET;
                }
            }
        }
	}

    printf("[socketListenerTask]\nremoveAsynchronousTask: %d\n\n", (int)taskId);
    for (int i = 0; i < server->clientsLength; i++)
    {
        printf("[socketListenerTask]\nclose client: %d\n\n", (int)server->clients[i]);
        CLOSESOCKET(server->clients[i]);
    }

    free(targ); 
    free(server->clients);
    free(buffer);
}

#pragma endregion

#pragma region socketListenerTaskRemove[taskId_Integer]: taskId_Integer 

DLLEXPORT int socketListenerTaskRemove(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    mint taskId = MArgument_getInteger(Args[0]);
    printf("[socketListenerTaskRemove]\nremoved task id: %d\n\n", (int)taskId);
    MArgument_setInteger(Res, libData->ioLibraryFunctions->removeAsynchronousTask(taskId));
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketConnect[host_String, port_String]: socketId_Integer 

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
        printf("[socketConnect]\ngetaddrinfo error: %d\n\n", iResult);
        return LIBRARY_FUNCTION_ERROR; 
    }

    connectSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol); 
    if (connectSocket == INVALID_SOCKET){
        printf("[socketConnect]\nsocket error: %d\n\n", GETSOCKETERRNO());
        freeaddrinfo(address); 
        return LIBRARY_FUNCTION_ERROR; 
    }

    iResult = connect(connectSocket, address->ai_addr, (int)address->ai_addrlen);
    freeaddrinfo(address);
    if (iResult == SOCKET_ERROR) {
        printf("[socketConnect]\nconnect error: %d\n\n", GETSOCKETERRNO());
        closesocket(connectSocket); 
        connectSocket = INVALID_SOCKET;
        return LIBRARY_FUNCTION_ERROR;
    }

    ioctlsocket(connectSocket, FIONBIO, &iMode);

    MArgument_setInteger(Res, connectSocket); 
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketBinaryWrite[socketId_Integer, data: ByteArray[<>], dataLength_Integer, bufferLength_Integer]: socketId_Integer 

DLLEXPORT int socketBinaryWrite(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    MNumericArray mArr = MArgument_getMNumericArray(Args[1]); 

    int iResult;
    BYTE *data = (BYTE *)libData->numericarrayLibraryFunctions->MNumericArray_getData(mArr); 
    int dataLength = MArgument_getInteger(Args[2]); 
    int bufferSize = MArgument_getInteger(Args[3]); 
    
    iResult = socketWrite(clientId, data, dataLength, bufferSize); 
    if (iResult == SOCKET_ERROR) {
        printf("[socketWrite]\n\tsend failed with error: %d\n\n", (int)GETSOCKETERRNO());
        CLOSESOCKET(clientId);
        MArgument_setInteger(Res, GETSOCKETERRNO()); 
        return LIBRARY_FUNCTION_ERROR; 
    }
    
    printf("[socketWrite]\nwrite %d bytes\n\n", dataLength);
    MArgument_setInteger(Res, clientId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketWriteString[socketId_Integer, data_String, dataLength_Integer, bufferSize_Integer]: socketId_Integer 

DLLEXPORT int socketWriteString(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    SOCKET socketId = MArgument_getInteger(Args[0]); 
    char* data = MArgument_getUTF8String(Args[1]); 
    int dataLength = MArgument_getInteger(Args[2]); 
    int bufferSize = MArgument_getInteger(Args[3]); 
    
    iResult = socketWrite(socketId, data, dataLength, bufferSize); 
    if (iResult == SOCKET_ERROR) {
        printf("[socketWriteString]\nsend failed with error: %d\n\n", (int)GETSOCKETERRNO());
        CLOSESOCKET(socketId);
        MArgument_setInteger(Res, GETSOCKETERRNO()); 
        return LIBRARY_FUNCTION_ERROR; 
    }
  
    printf("[socketWriteString]\nwrite %d bytes\n\n", dataLength);
    MArgument_setInteger(Res, socketId);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadyQ[socketId_Integer]: readyQ: True | False 

DLLEXPORT int socketReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]); 
    
    int iResult; 
    BYTE *buffer = (BYTE *)malloc(sizeof(BYTE)); 
    
    iResult = recv(socketId, buffer, 1, MSG_PEEK);
    if (iResult == SOCKET_ERROR){
        MArgument_setBoolean(Res, False); 
    } else {
        MArgument_setBoolean(Res, True); 
    }

    free(buffer);
    return LIBRARY_NO_ERROR;
}

#pragma endregion

#pragma region socketReadMessage[socketId_Integer, bufferSize_Integer]: ByteArray[<>] 

DLLEXPORT int socketReadMessage(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET socketId = MArgument_getInteger(Args[0]);
    int bufferSize = MArgument_getInteger(Args[1]);
    
    BYTE *buffer = (BYTE*)malloc(bufferSize * sizeof(BYTE));
    int iResult;
    int length = 0;

    iResult = recv(socketId, buffer, bufferSize, 0);
    if (iResult > 0) {
        printf("[socketReadMessage]\nreceived %d bytes\n\n", iResult);
        MArgument_setMNumericArray(Res, createByteArray(libData, buffer, iResult));
    } else {
        return LIBRARY_FUNCTION_ERROR;
    }

    return LIBRARY_NO_ERROR; 
}

#pragma endregion

#pragma region socketPort[socketId_Integer]: port_Integer

DLLEXPORT int socketPort(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    SOCKET socketId = MArgument_getInteger(Args[0]); 
    struct  sockaddr_in sin;
    int port;
    int addrlen = sizeof(sin);

    getsockname(socketId, (struct sockaddr *)&sin, &addrlen);
    port = ntohs(sin.sin_port); 

    printf("[sockePort]\nsocketId: %d and port: %d\n\n", (int)socketId, port);
    MArgument_setInteger(Res, port);
    return LIBRARY_NO_ERROR; 
}

#pragma endregion
