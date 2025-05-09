(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


TCPSocketObject::usage = 
"TCPSocketObject[socketId] socket object representation.";


TCPSocketOpen::usage = 
"TCPSocketOpen[port] returns a new server socket opened on localhost.
TCPSocketOpen[host, port] returns a new server socket opened on specific host.";


TCPSocketClose::usage =
"TCPSocketClose[socket] closes the socket.";


TCPSocketConnect::usage = 
"TCPSocketConnect[port] returns a new client socket connected to localhost.
TCPSocketConnect[host, port] returns a new client socket connected to specific host.";


TCPSocketsSelect::usage = 
"TCPSocketsSelect[{sockets}] returns list of ready sockets.";


TCPSocketsAccept::usage = 
"TCPSocketsAccept[sockets] returns new client.";


TCPSocketsRecv::usage = 
"TCPSocketsRecv[socket] returns received data.";


TCPSocketsSend::usage = 
"TCPSocketsSend[socket, data] send data.";


Begin["`Private`"];


Options[TCPSocketOpen] = {
    "NonBlocking" :> False, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


TCPSocketOpen[host_String: "localhost", port_Integer, OptionsPattern[]] := 
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
    TCPSocketObject[socketId]
];


TCPSocketClose[TCPSocketObject[socketId_Integer]] := 
With[{result = socketClose[socketId]}, 
    (*Returns success or not*)
    result === 0
];


Options[TCPSocketConnect] = {
    "NonBlocking" :> False, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


TCPSocketConnect[host_String: "localhost", port_Integer, OptionsPattern[]] :=
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
    TCPSocketObject[socketId]
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


(*socketOpen["host", "port", 
    nonBlocking:        0 | 1: 0,
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


(*socketAccept[listenSocketId] -> clientSocketId*)
socketAccept = 
LibraryFunctionLoad[$libFile, "socketAccept", {Integer}, Integer]; 


(*socketRecv[clientSocketId, bufferSize] -> *)
socketRecv = 
LibraryFunctionLoad[$libFile, "socketRecv", {Integer, Integer}, "ByteArray"];


(*socketSend[socketId, data, length] -> length*)
socketSend = LibraryFunctionLoad[$libFile, "socketSend", {Integer, {"ByteArray", "Shared"}, Integer}, Integer]; 


End[]; 


EndPackage[]; 