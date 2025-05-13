#pragma once

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
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "WolframLibrary.h"

#pragma endregion

//server data
typedef struct Server_st *Server;

//serverCreate[listenSocketId, maxClients, bufferSize, timeout]: serverPtr
DLLEXPORT int serverCreate(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res);

//serverListen[serverPtr_Integer]: taskId_Integer
DLLEXPORT int serverListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res);

DLLEXPORT void serverListenerTask(mint taskId, void* vtarg);

DLLEXPORT void serverSelect(Server server);

DLLEXPORT void serverAccept(Server server);

DLLEXPORT void serverRecv(Server server);

DLLEXPORT void serverCheck(Server server);

DLLEXPORT void serverClose(Server server);

DLLEXPORT void serverRaiseEvent(Server server);

DLLEXPORT void serverDestroy(Server server);