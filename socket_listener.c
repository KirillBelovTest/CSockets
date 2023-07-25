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
    WolframIOLibrary_Functions ioLibrary;
    SOCKET listentSocket; 
}* SocketTaskArgs; 

static void ListenSocketTask(mint asyncObjID, void* vtarg)
{
    SOCKET clients[64]; 
    int clientsLength = 0;

    int iResult; 
    int iMode = 1; 

    char buf[512]; 
    int buflen = 512; 

    SOCKET clientSocket = INVALID_SOCKET;
	SocketTaskArgs targ = (SocketTaskArgs)vtarg;
    SOCKET listenSocket = targ->listentSocket; 
	WolframIOLibrary_Functions ioLibrary = targ->ioLibrary;
    printf("ACCEPT_SOCKET_TASK\n");

	DataStore ds;
    free(targ); 
    
    iResult = ioctlsocket(listenSocket, FIONBIO, &iMode); 
    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %ld\n", iResult);
    }
	
	while(ioLibrary->asynchronousTaskAliveQ(asyncObjID))
	{
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            //printf("accept failed with error: %d\n", WSAGetLastError());
        } else {
            printf("NEW CLIENT: %d\n", clientSocket);
            ds = ioLibrary->createDataStore();
            ioLibrary->DataStore_addInteger(ds, clientSocket);
            clients[clientsLength++] = clientSocket; 
            ioLibrary->raiseAsyncEvent(asyncObjID, "ACCEPT_SOCKET", ds);
        }

        for (size_t i = 0; i < clientsLength; i++)
        {
            iResult = recv(clients[i], buf, buflen, 0); 
            if (iResult > 0){
                printf("RECEIVED %d BYTES\n", iResult);
                ds = ioLibrary->createDataStore();
                ioLibrary->DataStore_addString(ds, buf);
                ioLibrary->raiseAsyncEvent(asyncObjID, "RECEIVED_BYTES", ds);
            }
        }
	}
}

/**
 * Создает сервер
 * - Инициализируем 
 * - Загружаем библиотеку
 * - Создаем адрес
 * - Создем сокет для прослушивания
 * - Привязка сокета к адресу
 * - Запуск отдельного потока где происходит принятие соединений
 * - 
*/
DLLEXPORT int create_server(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) 
{

    /* Инициализируем */

    int iResult; 
    char* listenPortName = MArgument_getUTF8String(Args[0]); 
    SOCKET listenSocket = INVALID_SOCKET; 
    WolframIOLibrary_Functions ioLibrary = libData->ioLibraryFunctions; 
    printf("INIT\n");

    /* -------------------------------------------------------------------- */

    /* Загружаем библиотеку */
    
    WSADATA wsaData; 
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    /* -------------------------------------------------------------------- */

    /* Создаем адрес */
    
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

    /* -------------------------------------------------------------------- */

    /* Создание сокета для прослушивания */
    
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(address);
        WSACleanup();
        return 1;
    }

    /* -------------------------------------------------------------------- */

    /* Привязка сокета к адресу */

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(address);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(address);

    /* -------------------------------------------------------------------- */

    /* Перевод сокета в состояние прослушивания */

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("LISTEN SOCKET\n"); 

    /* -------------------------------------------------------------------- */

    /* Запуск отдельного потока где происходит принятие соединений и чтение данных */

    SocketTaskArgs threadArg = (SocketTaskArgs)malloc(sizeof(struct SocketTaskArgs_st));
    threadArg->ioLibrary=ioLibrary; 
    threadArg->listentSocket=listenSocket;
    mint asyncObjID;
    asyncObjID = ioLibrary->createAsynchronousTaskWithThread(ListenSocketTask, threadArg);

    /* -------------------------------------------------------------------- */

    /* Возвращение идентификатора асинхронной задачи в качестве результата */

    MArgument_setInteger(Res, asyncObjID); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int socker_write(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    mint clientId = MArgument_getInteger(Args[0]); 
    char *bytes = MArgument_getUTF8String(Args[1]); 
    mint bytesLen = MArgument_getUTF8String(Args[2]); 
    send(clientId, bytes, bytesLen, 0); 
    MArgument_setInteger(Res, 0); 
    return LIBRARY_NO_ERROR; 
}