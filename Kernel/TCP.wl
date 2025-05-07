(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


CSocketObject::usage = 
"CSocketObject[socketId] socket representation object.";


CSocketOpen::usage = 
"CSocketOpen[port] returns new server socket opened on localhost.
CSocketOpen[host, port] returns new server socket opened on specific host."; 


CSocketClose::usage =
"CSocketClose[socket] closes the socket.";


CSocketConnect::usage = 
"CSocketConnect[port] returns new server socket opened on localhost.
CSocketConnect[host, port] returns new server socket opened on specific host."; 


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


(*socketGetSocketId[socketId, clientsLengthMax, bufferSize] -> serverPtr*)
serverCreate = 
LibraryFunctionLoad[$libFile, "serverCreate", {Integer, Integer, Integer}, Integer]; 


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