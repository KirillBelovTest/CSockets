#pragma region header

#undef UNICODE

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define ms 1000
    #define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
    #define GETSOCKETERRNO() (WSAGetLastError())
    #pragma comment (lib, "Ws2_32.lib")
#else
    #include <string.h>
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
    #define SLEEP usleep
    #define ms 1000
    inline void nopp() {}
    #define SOCKET int
    #define ISVALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define GETSOCKETERRNO() (errno)
    #define BYTE uint8_t
    #define BOOL int
#endif

#include <stdio.h>
#include <stdlib.h>

#include "WolframLibrary.h"
#include "WolframIOLibraryFunctions.h"
#include "WolframNumericArrayLibrary.h"


int socketReadyForWriteQ(SOCKET socketId){
    fd_set set;
    struct timeval socktimeout;
    socktimeout.tv_sec = 0;
    socktimeout.tv_usec = 1000;
    int rv;

    FD_ZERO(&set);
    FD_SET(socketId, &set);

    if (select(socketId+1, NULL, &set, NULL, &socktimeout) == 1) return True;
    return False;
}

int socketWrite(SOCKET socketId, BYTE *data, int dataLength, int bufferSize){
    int iResult;
    int writeLength;
    char *buffer;
    int err;
    SOCKET currentSoketIdBackup;
    int timeoutInit = 10;
    int timeout = timeoutInit;

    for (int i = 0; i < dataLength; i += bufferSize) {
        buffer = (char*)&data[i];
        writeLength = dataLength - i > bufferSize ? bufferSize : dataLength - i;

        if (socketReadyForWriteQ(socketId) == 1) {
            iResult = send(socketId, buffer, writeLength, 0);
            if (iResult == SOCKET_ERROR) {
                err = GETSOCKETERRNO();
                printf("[socketWrite]\n\terror write %d bytes to socket %d on step %d error number = %d\n\n", dataLength, (int)socketId, i, err);
                if (err == 10035 || err == 35) {
                    printf("[socketWrite]\n\twrite paused on %d ms\n\n", timeout);
                    SLEEP(timeout * ms);
                    i -= bufferSize;
                    timeout += timeoutInit;

                    if (timeout > 100 * timeoutInit) {
                        return SOCKET_ERROR;
                    }
                } else {
                    return SOCKET_ERROR;
                }
            } else {
                timeout = timeoutInit;
            }
        } else {
            SLEEP(timeout * ms);
            i -= bufferSize;
            timeout += timeoutInit;
            if (timeout > 100 * timeoutInit) {
                printf("[socketWrite]\n\ttimeout error!\n\n");
                return SOCKET_ERROR;
            }
        }
    }

    return dataLength;
}