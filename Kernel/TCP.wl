(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


CSocketObject::usage = 
"CSocketObject[socketId] socket representation."; 


CServerObject::usage = 
"CServerObject[serverPtr] server representation."; 

CSocketOpen::usage = 
"CSocketOpen[port] returns new server socket opened on localhost.
CSocketOpen[host, port] returns new server socket opened on specific host."; 


CSocketConnect::usage = 
"CSocketConnect[port] connect to socket on localhost.
CSocketConnect[host, port] connect to socket."; 


CSocketListener::usage = 
"CSocketListener[assoc] listener object."; 


Begin["`Private`"]; 


Options[CSocketOpen] = {
    "BufferSize" :> $bufferSize, 
    "ModeNoDelay" :> $modeNoDelay,
    "Mode" :> $mode, 
    "SendBufferSize" :> $sendBufferSize,
    "RecvBufferSize" :> $recvBufferSize
};


CSocketOpen[host_String: "localhost", port_Integer, OptionsPattern[]] := 
CServerObject[socketOpen[
    host, 
    ToString[port], 
    OptionValue["BufferSize"], 
    OptionValue["SendBufferSize"],
    OptionValue["RecvBufferSize"], 
    OptionValue["ModeNoDelay"],
    OptionValue["Mode"]    
]]; 


CServerObject[serverPtr_Integer]["ListenSocket" | "Socket"] :=
CSocketObject[serverGetListenSocket[serverPtr]]; 


CServerObject[serverPtr_Integer]["DestinationHostname" | "Host"] :=
With[{socketId = serverGetListenSocket[serverPtr]}, 
    socketGetHostname[socketId]
];


CServerObject[serverPtr_Integer]["DestinationPort" | "Port"] :=
socketGetPort[serverGetListenSocket[serverPtr]]; 


CServerObject[serverPtr_Integer]["ConnectedClients" | "Clients"] :=
Map[CSocketObject, serverGetClients[serverPtr]]; 


CSocketConnect[host_String: "localhost", port_Integer] := 
CSocketObject[socketConnect[host, ToString[port]]]; 


CSocketObject /: BinaryWrite[CSocketObject[socketId_Integer], data_ByteArray, opts: OptionsPattern[{"BufferSize" :> $bufferSize}]] := 
With[{bufferSize = OptionValue[CSocketOpen, Flatten[{opts}], "BufferSize"]},
    socketBinaryWrite[socketId, data, Length[data], bufferSize]
]; 


CSocketObject /: WriteString[CSocketObject[socketId_Integer], data_String, opts: OptionsPattern[{"BufferSize" :> $bufferSize}]] := 
With[{
    bufferSize = OptionValue[CSocketOpen, Flatten[{opts}], "BufferSize"], 
    len = StringLength[data]
},
    socketWriteString[socketId, data, len, bufferSize]
]; 


CSocketObject /: SocketReadMessage[CSocketObject[socketId_Integer]] := 
socketReadMessage[socketId, $bufferSize]; 


CSocketObject /: SocketReadMessage[CSocketObject[socketId_Integer], bufferSize_Integer] := 
socketReadMessage[socketId, bufferSize]; 


CSocketObject /: SocketReadyQ[CSocketObject[socketId_Integer]] := 
socketReadyQ[socketId]; 


CSocketObject /: Close[CSocketObject[socketId_Integer]] := 
socketClose[socketId]; 


CServerObject /: SocketListen[server: CServerObject[serverPtr_Integer], handler_] := 
Module[{task}, 
    task = Internal`CreateAsynchronousTask[socketListen, {serverPtr}, handler[toPacket[##]]&]; 
    CSocketListener[task, server, handler]
]; 


CSocketListener /: DeleteObject[CSocketListener[taskId_, ___]] := 
RemoveAsynchronousTask[taskId]; 


$directory = DirectoryName[$InputFileName, 2]; 


$socketIcon = 
Import[FileNameJoin[{$directory, "Images", "SocketIcon.png"}]]; 


$ListenreIcon = 
Import[FileNameJoin[{$directory, "Images", "ListenerIcon.png"}]]; 


$libraryLinkVersion := $libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


Once[$libraryLinkVersion]; 


$libFile = FileNameJoin[{
    $directory, 
    "LibraryResources", 
    $SystemID <> "-v" <> ToString[$libraryLinkVersion], 
    "tcp." <> Internal`DynamicLibraryExtension[]
}]; 


$bufferSize = 4 * 1024; 


$modeNoDelay = 0;


$mode = 1;


$sendBufferSize = 64 * 1024; 


$recvBufferSize = 64 * 1024; 


If[!FileExistsQ[$libFile], 
    Get[FileNameJoin[{$directory, "Scripts", "Build.wls"}]]
]; 


toPacket[task_, event: "Accepted" | "Closed", {serverId_, clientId_, data_Integer}] := 
<|
    "Event" -> event, 
    "TimeStamp" -> Now, 
    "Socket" :> CSocketObject[serverId], 
    "SourceSocket" :> CSocketObject[clientId], 
    "DataByteArray" :> ByteArray[{}], 
    "Data" :> "", 
    "DataBytes" :> {}, 
    "MultipartComplete" -> True
|>; 


toPacket[task_, event: "Received", {serverId_, clientId_, data_}] := 
With[{byteArray = ByteArray[data]}, 
    <|
        "Event" -> event, 
        "TimeStamp" -> Now, 
        "Socket" :> CSocketObject[serverId], 
        "SourceSocket" :> CSocketObject[clientId], 
        "DataByteArray" :> byteArray, 
        "Data" :> ByteArrayToString[byteArray], 
        "DataBytes" :> Normal[byteArray], 
        "MultipartComplete" -> True
    |>
]; 


(*socketOpen["host", "port", 
    bufferSize_Integer,
    sndBufSize_Integer, 
    rcvBufSize_Integer, 
    modeNoDelay: 0 | 1: 0, 
    mode: 0 | 1: 1, 
] -> serverPtr*)
socketOpen = LibraryFunctionLoad[$libFile, "socketOpen", {String, String, Integer, Integer, Integer, Integer, Integer}, Integer]; 


(*socketClose[socketId] -> socketId*)
socketClose = LibraryFunctionLoad[$libFile, "socketClose", {Integer}, Integer]; 


(*socketListen[serverPtr] -> taskId*)
socketListen = LibraryFunctionLoad[$libFile, "socketListen", {Integer}, {Integer}]; 


(*serverGetListenSocket[serverPtr] -> listenSocket*)
serverGetListenSocket = LibraryFunctionLoad[$libFile, "serverGetListenSocket", {Integer}, Integer]; 


(*socketGetHostname[socketId] -> hostname*)
socketGetHostname = LibraryFunctionLoad[$libFile, "socketGetHostname", {Integer}, String]; 


(*socketGetPort[serverPtr] -> port*)
socketGetPort = LibraryFunctionLoad[$libFile, "socketGetPort", {Integer}, Integer]; 


(*serverGetClients[serverPtr] -> port*)
serverGetClients = LibraryFunctionLoad[$libFile, "serverGetClients", {Integer}, {Integer, 1}]; 


(*socketConnect["host", "port"] -> socketId*)
socketConnect = LibraryFunctionLoad[$libFile, "socketConnect", {String, String}, Integer]; 


(*socketBinaryWrite[socketId, data, length, bufferSize] -> length*)
socketBinaryWrite = LibraryFunctionLoad[$libFile, "socketBinaryWrite", {Integer, {"ByteArray", "Shared"}, Integer, Integer}, Integer]; 


(*socketWriteString[socketId, data, length, bufferSize] -> length*)
socketWriteString = LibraryFunctionLoad[$libFile, "socketWriteString", {Integer, String, Integer, Integer}, Integer]; 


(*socketReadyQ[socketId] -> readyState*)
socketReadyQ = LibraryFunctionLoad[$libFile, "socketReadyQ", {Integer}, True | False]; 


(*socketReadMessage[socketId, bufferSize] -> message*)
socketReadMessage = LibraryFunctionLoad[$libFile, "socketReadMessage", {Integer, Integer}, "ByteArray"]; 


End[]; 


EndPackage[]; 