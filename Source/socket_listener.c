#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"

DLLEXPORT mint WolframLibrary_getVersion( ) {
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    return 0;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    return;
}

typedef struct SocketTaskArgs_st {
    WolframNumericArrayLibrary_Functions numericLibrary;
    WolframIOLibrary_Functions ioLibrary;
    SOCKET listentSocket; 
}* SocketTaskArgs; 

static void ListenSocketTask(mint asyncObjID, void* vtarg)
{
    SOCKET *clients = (SOCKET*)malloc(2 * sizeof(SOCKET)); 
    int clientsLength = 0;
    int clientsMaxLength = 2; 

    int iResult; 
    int iMode = 1; 

    BYTE buf[8192]; 
    size_t buflen = 8192; 
    mint dims[1]; 
    MNumericArray data;

    SOCKET clientSocket = INVALID_SOCKET;
	SocketTaskArgs targ = (SocketTaskArgs)vtarg;
    SOCKET listenSocket = targ->listentSocket; 
	WolframIOLibrary_Functions ioLibrary = targ->ioLibrary;
    WolframNumericArrayLibrary_Functions numericLibrary = targ->numericLibrary;
    printf("ACCEPT_SOCKET_TASK\n");

	DataStore ds;
    free(targ); 
    
    iResult = ioctlsocket(listenSocket, FIONBIO, &iMode); 
    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %ld\n", iResult);
    }
	
	while(ioLibrary->asynchronousTaskAliveQ(asyncObjID))
	{
        Sleep(1);
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
                numericLibrary->MNumericArray_new(MNumericArray_Type_UBit8, 1, dims, &data); 
                memcpy(numericLibrary->MNumericArray_getData(data), buf, iResult);
                
                ds = ioLibrary->createDataStore();
                ioLibrary->DataStore_addInteger(ds, listenSocket);
                ioLibrary->DataStore_addInteger(ds, clients[i]);
                ioLibrary->DataStore_addMNumericArray(ds, data);

                ioLibrary->raiseAsyncEvent(asyncObjID, "RECEIVED_BYTES", ds);
            }
        }
	}
}

DLLEXPORT int create_server(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) 
{
    int iResult; 
    char* listenPortName = MArgument_getUTF8String(Args[0]); 
    SOCKET listenSocket = INVALID_SOCKET; 
    WolframIOLibrary_Functions ioLibrary = libData->ioLibraryFunctions; 
    WolframNumericArrayLibrary_Functions numericLibrary = libData->numericarrayLibraryFunctions;
    printf("INIT\n");
    
    WSADATA wsaData; 
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    
    struct addrinfo *address = NULL; 
    struct addrinfo addressHints; 

    ZeroMemory(&addressHints, sizeof(addressHints));
    addressHints.ai_family = AF_INET;
    addressHints.ai_socktype = SOCK_STREAM;
    addressHints.ai_protocol = IPPROTO_TCP;
    addressHints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, listenPortName, &addressHints, &address); 
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(address);
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(address);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(address);

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("LISTEN SOCKET\n"); 

    SocketTaskArgs threadArg = (SocketTaskArgs)malloc(sizeof(struct SocketTaskArgs_st));
    threadArg->ioLibrary=ioLibrary; 
    threadArg->listentSocket=listenSocket;
    threadArg->numericLibrary=numericLibrary;
    mint asyncObjID;
    asyncObjID = ioLibrary->createAsynchronousTaskWithThread(ListenSocketTask, threadArg);

    MArgument_setInteger(Res, asyncObjID); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int socket_write(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    WolframNumericArrayLibrary_Functions numericLibrary = libData->numericarrayLibraryFunctions; 
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    BYTE *bytes = numericLibrary->MNumericArray_getData(MArgument_getMNumericArray(Args[1]));      
    int bytesLen = MArgument_getInteger(Args[2]); 

    iResult = send(clientId, bytes, bytesLen, 0); 
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d\n", WSAGetLastError());
        closesocket(clientId);
        MArgument_setInteger(Res, WSAGetLastError()); 
        return LIBRARY_FUNCTION_ERROR; 
    }
    
    printf("WRITE %d BYTES\n", bytesLen);
    MArgument_setInteger(Res, 0); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int socket_write_string(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    WolframNumericArrayLibrary_Functions numericLibrary = libData->numericarrayLibraryFunctions; 
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    char *text = numericLibrary->MNumericArray_getData(MArgument_getUTF8String(Args[1]));      
    int textLen = MArgument_getInteger(Args[2]); 

    iResult = send(clientId, text, textLen, 0); 
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d\n", WSAGetLastError());
        closesocket(clientId);
        MArgument_setInteger(Res, WSAGetLastError()); 
        return LIBRARY_FUNCTION_ERROR; 
    }
    
    printf("WRITE %d BYTES\n", textLen);
    MArgument_setInteger(Res, 0); 
    return LIBRARY_NO_ERROR; 
}
