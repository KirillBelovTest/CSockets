#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "WolframLibrary.h"

DLLEXPORT mint WolframLibrary_getVersion() {
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    return 0;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    return;
}

DLLEXPORT int test(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res){
    MArgument_setInteger(Res, 1); 
    fd_set *fd = malloc(sizeof(fd_set));
    libData->Message("hello", "hi");
    
    return LIBRARY_NO_ERROR; 
}
