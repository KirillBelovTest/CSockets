(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


CSocketObject::usage = 
"CSocketObject[socketId] socket object representation.";


CSocketOpen::usage = 
"CSocketOpen[port] returns a new server socket opened on localhost.
CSocketOpen[host, port] returns a new server socket opened on specific host.";


CSocketClose::usage =
"CSocketClose[socket] closes the socket.";


CSocketConnect::usage = 
"CSocketConnect[port] returns a new client socket connected to localhost.
CSocketConnect[host, port] returns a new client socket connected to specific host.";


CServerObject::usage = 
"CServerObject[serverPtr] server object representation.";


CServerCreate::usage = 
"CServerCreate[listenSocket] returns server object.";


Begin["`Private`"];


Options[CSocketOpen] = {
    "NonBlocking" :> True, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


CSocketOpen[host_String: "localhost", port_Integer, OptionsPattern[]] := 
With[{
    socketId = socketOpen[
        host, 
        ToString[port], 
        If[OptionValue["NonBlocking"], 1, 0], 
        If[OptionValue["KeepAlive"], 1, 0],
        If[OptionValue["NoDelay"], 1, 0], 
        OptionValue["SendBufferSize"],
        OptionValue["RecvBufferSize"]
    ]
}, 
    CSocketObject[socketId]
];


CSocketClose[CSocketObject[socketId_Integer]] := 
With[{result = socketClose[socketId]}, 
    (*Returns success or not*)
    result === 0
];


Options[CSocketConnect] = {
    "NonBlocking" :> False, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


CSocketConnect[host_String: "localhost", port_Integer, OptionsPattern[]] :=
With[{
    socketId = socketConnect[
        host, 
        ToString[port], 
        If[OptionValue["NonBlocking"], 1, 0],
        If[OptionValue["KeepAlive"], 1, 0],
        If[OptionValue["NoDelay"], 1, 0],
        OptionValue["SendBufferSize"],
        OptionValue["RecvBufferSize"]
    ]
}, 
    CSocketObject[socketId]
];


Options[CServerCreate] = {
    "MaxClients" -> 1024, 
    "BufferSize" -> 64 * 1024
};


CServerCreate[CSocketObject[listenSocketId_Integer], OptionsPattern[]] := 
With[{
    maxClients = OptionValue["MaxClients"], 
    bufferSize = OptionValue["BufferSize"]
}, 
    With[{
        serverPtr = serverCreate[listenSocketId, maxClients, bufferSize]
    }, 
        (*Return*)
        CServerObject[serverPtr]
    ]
];


CServerObject[serverPtr_Integer]["Socket" | "socket" | "ListenSocket" | "listenSocket"] := 
serverGetListenSocket[serverPtr]; 


$directory = 
DirectoryName[$InputFileName, 2]; 


$socketIcon = 
Import[FileNameJoin[{$directory, "Images", "SocketIcon.png"}]]; 


$ListenreIcon = 
Import[FileNameJoin[{$directory, "Images", "ListenerIcon.png"}]]; 


$libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


$libFile = FileNameJoin[{
    $directory, 
    "LibraryResources", 
    $SystemID <> "-v" <> ToString[$libraryLinkVersion], 
    "tcp." <> Internal`DynamicLibraryExtension[]
}]; 


If[!FileExistsQ[$libFile], 
    Get[FileNameJoin[{$directory, "Scripts", "Build.wls"}]]
]; 


toPacket[task_, event_, {serverId_, clientId_, data_}] := 
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


toPacket[task_, event_, {serverId_, clientId_}] := 
<|
    "Event" -> event, 
    "TimeStamp" -> Now, 
    "Socket" :> CSocketObject[serverId], 
    "SourceSocket" :> CSocketObject[clientId]
|>; 


(*socketOpen["host", "port", 
    nonBlocking:        0 | 1: 1,
    keepAlive_Integer:  0 | 1: 1, 
    noDelay_Integer:    0 | 1: 1, 
    sendBufferSize:     _Integer?Positive: 256*1024, 
    recvBufferSize:     _Integer?Positive: 256*1024, 
]-> listenSocketId*)
socketOpen = 
LibraryFunctionLoad[$libFile, "socketOpen", {String, String, Integer, Integer, Integer, Integer, Integer}, Integer]; 


(*socketClose[socketId] -> socketId*)
socketClose = 
LibraryFunctionLoad[$libFile, "socketClose", {Integer}, Integer]; 


(*socketConnect["host", "port", 
    nonBlocking:        0 | 1: 0,
    keepAlive_Integer:  0 | 1: 1, 
    noDelay_Integer:    0 | 1: 1, 
    sendBufferSize:     _Integer?Positive: 256*1024, 
    recvBufferSize:     _Integer?Positive: 256*1024, 
]-> connectSocketId*)
socketConnect = 
LibraryFunctionLoad[$libFile, "socketConnect", {String, String, Integer, Integer, Integer, Integer, Integer}, Integer]; 


(*socketSelect[{listenSocket, client1, ..}, length, timeout]: {client1, ..}*)
socketSelect = 
LibraryFunctionLoad[$libFile, "socketSelect", {{Integer, 1}, Integer, Integer}, {Integer, 1}]; 


(*socketGetSocketId[socketId, clientsLengthMax, bufferSize] -> serverPtr*)
serverCreate = 
LibraryFunctionLoad[$libFile, "serverCreate", {Integer, Integer, Integer}, Integer]; 


(*serverAddClient[serverPtr, client]*)
serverAddClient = 
LibraryFunctionLoad[$$libFile, "serverAddClient", {Integer, Integer}, "Void"];


(*socketListen[listenSocketId] -> clientId*)
socketAccept = 
LibraryFunctionLoad[$libFile, "socketAccept", {Integer}, {Integer}]; 


(*socketListen[serverPtr] -> taskId*)
serverCreateListenerTask = 
LibraryFunctionLoad[$libFile, "serverCreateListenerTask", {Integer}, {Integer}]; 


(*serverGetListenSocket[serverPtr] -> listenSocket*)
serverGetListenSocket = LibraryFunctionLoad[$libFile, "serverGetListenSocket", {Integer}, Integer]; 


(*socketGetHostname[socketId] -> hostname*)
socketGetHostname = LibraryFunctionLoad[$libFile, "socketGetHostname", {Integer}, String]; 


(*socketGetPort[serverPtr] -> port*)
socketGetPort = LibraryFunctionLoad[$libFile, "socketGetPort", {Integer}, Integer]; 


(*serverGetClients[serverPtr] -> port*)
serverGetClients = LibraryFunctionLoad[$libFile, "serverGetClients", {Integer}, {Integer, 1}]; 


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