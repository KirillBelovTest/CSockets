#undef UNICODE


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define SLEEP Sleep



//Windows already defines SOCKET
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())
#define WSACLEANUP (WSACleanup())


#include <stdlib.h>
#include <stdio.h>


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

typedef struct SERVER_st {
    SOCKET listenSocket; 
    SOCKET *clients;
    int clientsLength;
    int clientsMaxLength;
    mint buflen;
    BYTE* buf;     
} SERVER; 

SERVER* servers[100];
int nServers = 0;

WolframIOLibrary_Functions ioLibrary;
WolframNumericArrayLibrary_Functions numericLibrary;
mint asyncObjID;

SOCKET garbage;

SOCKET* fetchClient(SOCKET s) {
    for (int l=0; l<nServers; ++l) {
        for (int k=0; k < servers[l]->clientsLength; ++k) {
            if (s == servers[l]->clients[k]) {
                printf("found! ok\n");
                SOCKET *ptr = &(servers[l]->clients[k]);
                return ptr;
            }
        }
    }

    printf("Something is wrong. A requested socket was not found in the array... \n");
    return &garbage;
}

static void ListenSocketTask(mint asyncObjID, void* vtarg);

DLLEXPORT int run_uvloop(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) {
    printf("creating async task...\n");

    ioLibrary = libData->ioLibraryFunctions;
    numericLibrary = libData->numericarrayLibraryFunctions;
        
    asyncObjID = ioLibrary->createAsynchronousTaskWithThread(ListenSocketTask, NULL);

    MArgument_setInteger(Res, asyncObjID); 
    return LIBRARY_NO_ERROR;     
}

static void ListenSocketTask(mint asyncObjID, void* vtarg)
{

  mint dims[1]; 
  MNumericArray data;
  DataStore ds;

  SOCKET clientSocket;
  SERVER* server;


  while(ioLibrary->asynchronousTaskAliveQ(asyncObjID)) {
    
    SLEEP(1);

    for (size_t l = 0; l < nServers; ++l) {
        server = servers[l];
        clientSocket = accept(server->listenSocket, NULL, NULL);
        
        if (clientSocket != INVALID_SOCKET) {
            printf("NEW CLIENT: %d\n", clientSocket);
            server->clients[server->clientsLength] = clientSocket; 
            server->clientsLength = server->clientsLength + 1;

            if (server->clientsLength == server->clientsMaxLength){
                server->clientsMaxLength *= 2; 
                printf("Reallocate client buffer to %d\n", server->clientsMaxLength);
                server->clients = (SOCKET*)realloc(server->clients, server->clientsMaxLength * sizeof(SOCKET)); 
            }
        } 

        for (size_t i = 0; i < server->clientsLength; i++)
        {
            if (server->clients[i] == INVALID_SOCKET) continue;
            int iResult = recv(server->clients[i], server->buf, server->buflen, 0); 
            if (iResult > 0) {            
                printf("CURRENT NUMBER OF CLIENTS: %d\n", server->clientsLength);
                printf("MAX NUMBER OF CLIENTS: %d\n", server->clientsMaxLength);
                printf("RECEIVED %d BYTES\n", iResult);
                dims[0] = iResult; 
                numericLibrary->MNumericArray_new(MNumericArray_Type_UBit8, 1, dims, &data); 
                memcpy(numericLibrary->MNumericArray_getData(data), server->buf, iResult);
                
                ds = ioLibrary->createDataStore();
                ioLibrary->DataStore_addInteger(ds, l);
                ioLibrary->DataStore_addInteger(ds, server->clients[i]);
                ioLibrary->DataStore_addMNumericArray(ds, data);

                ioLibrary->raiseAsyncEvent(asyncObjID, "RECEIVED_BYTES", ds);
            }
        }
    }
  }
}

DLLEXPORT int create_server(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res) 
{
    int iResult; 
    char* listenAddrName = MArgument_getUTF8String(Args[0]); 
    char* listenPortName = MArgument_getUTF8String(Args[1]); 

    SOCKET listenSocket = INVALID_SOCKET; 

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

    iResult = getaddrinfo(listenAddrName, listenPortName, &addressHints, &address); 
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACLEANUP;
        return 1;
    }
    
    listenSocket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (!ISVALIDSOCKET(listenSocket)) {
        printf("socket failed with error: %d\n", GETSOCKETERRNO());
        freeaddrinfo(address);
        WSACLEANUP;
        return 1;
    }

    iResult = bind(listenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", GETSOCKETERRNO());
        freeaddrinfo(address);
        CLOSESOCKET(listenSocket);
        WSACLEANUP;
        return 1;
    }

    freeaddrinfo(address);

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", GETSOCKETERRNO());
        CLOSESOCKET(listenSocket);
        WSACLEANUP;
        return 1;
    }

    printf("LISTEN SOCKET\n"); 

    servers[nServers] = (SERVER*)malloc(sizeof(SERVER));

    servers[nServers]->clients = (SOCKET*)malloc(2 * sizeof(SOCKET));
    servers[nServers]->buf = malloc(8192 * sizeof(BYTE));
    servers[nServers]->buflen = 8192;
    servers[nServers]->listenSocket = listenSocket;
    servers[nServers]->clientsLength = 0;
    servers[nServers]->clientsMaxLength = 2;

    u_long iMode = 1; 

    iResult = ioctlsocket(listenSocket, FIONBIO, &iMode); 

    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %d\n", iResult);
    }

    MArgument_setInteger(Res, nServers); 

    nServers++;
    return LIBRARY_NO_ERROR; 
}

size_t send_full_msg(int sock_fd, char *write_buf, size_t write_buf_length, size_t chunk_s) {

        fd_set set;
        struct timeval socktimeout;
        int rv;
        FD_ZERO(&set); /* clear the set */
        FD_SET(sock_fd, &set); /* add our file descriptor to the set */

        socktimeout.tv_sec = 6;
        socktimeout.tv_usec = 0;

        size_t chunk_size = chunk_s;
        if (write_buf_length < chunk_size) chunk_size = write_buf_length;



        size_t pos_in_buf = 0; //starts at 0 and is incremented to write to the right location
        ssize_t size_sent = 0; //the size of the values obtained from a recv

        int num_neg_count=0;
     
        //log.msg("Entering loop non block on write...");
        int total_failed = 0;


        while (pos_in_buf < write_buf_length)
        {
   
            rv = select(sock_fd+1, NULL, &set, NULL, &socktimeout);

        
            if (rv==0) {
                printf("No data available...sleeping\n");
                SLEEP(100);
                
                num_neg_count++;
    
                if(num_neg_count > 3) { //three timeouts in a row
                    printf("Timeout!\n");
                    return pos_in_buf == 0 ? -1 : pos_in_buf;
                } else {
                    continue;
                }
            }
            else if(rv==-1){
                //do nothing if this hits the timeout it will break out
                //log.msg("Select error...num neg count is: " + to_string(num_neg_count));
                
                printf("Error during writting to a pipe! \n");
                return -1;
            }
            else{
                //there is data to be handled
                //log.msg("Select is saying socket is available for sending...");
                //remaining buf size is the total buf length minus the position (plus 1?)
                size_t remaining_buf_size = write_buf_length - pos_in_buf;                                     //avoids a segmentation fault

                size_t bytes_to_write = remaining_buf_size > chunk_size ? chunk_size : remaining_buf_size; //works to prevent a segmentation fault
                printf("Writting %d bytes to a pipe ...\n", bytes_to_write);
                size_sent = send(sock_fd, write_buf+pos_in_buf, bytes_to_write, 0);

                if (size_sent < 0)
                {
                    printf("Failed! sleeping 100 ms\n");
                    total_failed++;
                    num_neg_count++; //if there are 3 consecutive failed writes we will quit
                    SLEEP(100);
                    //this_thread::sleep_for(chrono::microseconds(100)); //needs to wait to try and get more data
                    if (num_neg_count>3) //timeout or 3 consecutive failed writes
                    {
                        //log.msg("Timeout exceeded");
                        printf("3 consecutive failed writes!!!\n");
                        return -1;
                    }

                    continue;
                } else {
                    num_neg_count = 0; //reset the failed writes
                    printf("writting to a pipe went good\n");
                    pos_in_buf += size_sent;
                }

                //log.msg("Data received! Length: " + to_string(size_recv));

            }

            //cout << "Duration: " << duration.count() << endl;
            //cout << "Timeout: " << timeout.count() << endl;

            if (num_neg_count>3) //timeout or 3 consecutive failed writes
            {
                //log.msg("Timeout exceeded");
                printf("3 consecutive failed writes!!!\n");
                return -1;
            }


            
        }

        //log.msg("Total data length sent was: " + to_string(pos_in_buf));
        if(pos_in_buf == 0) {
            printf("error! no data received\n");
            return -1; //error, no data received
        }

        printf("writting was done correctly :)\n");
            

        return pos_in_buf; //the full size of the message received
    }


DLLEXPORT int socket_write(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    WolframNumericArrayLibrary_Functions numericLibrary = libData->numericarrayLibraryFunctions; 
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    mint trueLength = libData->numericarrayLibraryFunctions->MNumericArray_getFlattenedLength(MArgument_getMNumericArray(Args[1]));

    BYTE *bytes = numericLibrary->MNumericArray_getData(MArgument_getMNumericArray(Args[1]));      
    mint bytesLen = MArgument_getInteger(Args[2]); 

    printf("sending stuff....\n");
    printf("real length: %lld, claimed: %lld\n", trueLength, bytesLen);



    //iResult = send(clientId, bytes, bytesLen, MSG_NOSIGNAL); 
    iResult =send_full_msg(clientId, bytes, bytesLen, 8000);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", GETSOCKETERRNO());
        *fetchClient(clientId) = INVALID_SOCKET;
        CLOSESOCKET(clientId);
        printf("socket was closed\n");
        MArgument_setInteger(Res, -1); 
        return LIBRARY_NO_ERROR;
    }


    
    
    printf("WRITE %lld BYTES\n", bytesLen);

    MArgument_setInteger(Res, 0); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int socket_write_string(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    int iResult; 
    WolframNumericArrayLibrary_Functions numericLibrary = libData->numericarrayLibraryFunctions; 
    SOCKET clientId = MArgument_getInteger(Args[0]); 
    char *text = MArgument_getUTF8String(Args[1]);      
    int textLen = MArgument_getInteger(Args[2]); 

    //iResult = send(clientId, text, textLen, 0); 
    iResult = send_full_msg(clientId, text, textLen, 8000);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d\n", GETSOCKETERRNO());
        *fetchClient(clientId) = INVALID_SOCKET;
        CLOSESOCKET(clientId);
        MArgument_setInteger(Res, -1);  
        return LIBRARY_NO_ERROR;
    }

    printf("WRITE %d BYTES\n", textLen);
    MArgument_setInteger(Res, 0); 
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int close_socket(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    SOCKET s = MArgument_getInteger(Args[0]);
    SOCKET* current = fetchClient(s);
    if(*current == INVALID_SOCKET) {
        printf("already closed!\n");
        MArgument_setInteger(Res, 0);
    } else {
        MArgument_setInteger(Res, CLOSESOCKET(s));
        *current = INVALID_SOCKET;
    }
    
    return LIBRARY_NO_ERROR; 
}

DLLEXPORT int stop_server(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    mint taskId = MArgument_getInteger(Args[0]); 
    MArgument_setInteger(Res, libData->ioLibraryFunctions->removeAsynchronousTask(taskId)); 
    return LIBRARY_NO_ERROR; 
}