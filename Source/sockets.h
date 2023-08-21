#undef UNICODE

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

DLLEXPORT int SocketOpen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res);

DLLEXPORT int SocketConnect(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketWrite(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketRead(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketClose(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res);

DLLEXPORT int SocketReadyQ(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketListen(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketListenerTask(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

DLLEXPORT int SocketListenerTaskRemove(WolframLibraryData libData, mint Argc, MArgument *Args, MArgument Res); 

#ifdef _WIN32
#include "sockets_windows.c"
#else
#include "sockets_unix.c"
#endif
