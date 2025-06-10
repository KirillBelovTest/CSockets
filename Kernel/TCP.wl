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


CSocketSelect::usage = 
"CSocketSelect[{sockets}] returns list of ready sockets.";


CSocketAccept::usage = 
"CSocketAccept[socket] returns new client.";


CSocketRecv::usage = 
"CSocketRecv[socket] returns received data.";


CSocketSend::usage = 
"CSocketSend[socket, data] send data.";


CSocketListener::usage =
"CSocketListener[assoc] returns a new listener object.";


Begin["`Private`"];


Options[CSocketOpen] = {
    "NonBlocking" :> False, 
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


CSocketObject /: Close[socket_CSocketObject] := 
CSocketClose[socket];


CSocketSelect[sockets: {__CSocketObject}, timeout: _?NumericQ: 1] := 
With[{socketIds = sockets[[All, 1]], tv = Round[timeout * 1000000]}, 
    Map[CSocketObject] @ 
    socketSelect[socketIds, Length[socketIds], tv]
];


CSocketObject /: SocketReadyQ[socket_CSocketObject, timeout: _?NumericQ: 0] := 
CSocketsSelect[{socket}, timeout] === {socket};


CSocketsRecv[CSocketObject[socketId_Integer], bufferSize_Integer: 8192] := 
socketRecv[socketId, bufferSize]; 


CSocketObject /: SocketReadMessage[socket_CSocketObject, bufferSize_Integer: 8192] := 
CSocketsRecv[socket, bufferSize];


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


CSocketObject /: WriteString[CSocketObject[socketId_Integer], data_String, len_: 1024 * 1024] :=
Do[
    socketSendString[socketId, #, StringLength[#]]& @ StringTake[data, {i, UpTo[i + len - 1]}], 
    {i, 1, StringLength[data], len}
];


CSocketObject /: BinaryWrite[CSocketObject[socketId_Integer], data_ByteArray, len_: 1024 * 1024] :=
Do[
    socketSend[socketId, #, Length[#]]& @ data[[i ;; UpTo[i + len - 1]]], 
    {i, 1, Length[data], len}
];


CSocketObject /: SocketReadyQ[socket_CSocketObject, timeout: _?NumericQ: 0] :=
CSocketSelect[{socket}, timeout] === {socket};


CSocketObject /: SocketListen[CSocketObject[socketId_Integer], handler_, OptionsPattern[{
    "ClientsCapacity" -> 1024,
    "BufferSize" -> 65536,
    "SelectTimeout" -> 1, 
    "Encoding" -> "UTF-8"
}]] := 
With[{
    serverPtr = serverCreate[
        socketId, 
        OptionValue["ClientsCapacity"], 
        OptionValue["BufferSize"], 
        Round[OptionValue["SelectTimeout"] * 1000000]
    ], 
    encoding = OptionValue["Encoding"]
}, 
    With[{
        task = Internal`CreateAsynchronousTask[
            serverListen, 
            {serverPtr}, 
            handler[toEvent[encoding][##]]&
        ]
    }, 
        (*Returns listener*)
        CSocketListener[<|
            "Task" -> task, 
            "Server" -> serverPtr, 
            "Socket" -> CSocketObject[socketId], 
            "Handler" -> handler,
            "ClientsCapacity" -> OptionValue["ClientsCapacity"], 
            "BufferSize" -> OptionValue["BufferSize"], 
            "SelectTimeout" -> OptionValue["SelectTimeout"]
        |>]
    ]
];


CSocketListener /: Close[CSocketListener[assoc_?AssociationQ]] := (
    Close[assoc["Socket"]];
    RemoveAsynchronousTask[assoc["Task"]];
);


toEvent[encoding_][task_, event_, {serv_, sock_, data_}] := 
With[{byteArray = ByteArray[data], time = Now}, <|
    "Event" :> event, 
    "TimeStamp" :> time, 
    "SourceSocket" :> CSocketObject[sock],
    "Socket" :> CSocketObject[serv],
    "Data" :> ByteArrayToString[byteArray, encoding],
    "DataBytes" :> Normal[byteArray],
    "DataByteArray" :> byteArray, 
    "MultiPartComplete" :> True
|>];


toEvent[encoding_][task_, event_, {serv_, sock_}] := 
With[{time = Now}, <|
    "Event" :> event, 
    "TimeStamp" :> time, 
    "SourceSocket" :> CSocketObject[sock],
    "Socket" :> CSocketObject[serv]
|>];


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
    "csockets." <> Internal`DynamicLibraryExtension[]
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


(*socketCheck[sockets, length]: aliveSockets*)
socketCheck = 
LibraryFunctionLoad[$libFile, "socketCheck", {{Integer, 1}, Integer}, {Integer, 1}]; 


(*socketAccept[listenSocketId]: clientSocketId*)
socketAccept = 
LibraryFunctionLoad[$libFile, "socketAccept", {Integer}, Integer]; 


(*socketRecv[clientSocketId, bufferSize]: byteArray*)
socketRecv = 
LibraryFunctionLoad[$libFile, "socketRecv", {Integer, Integer}, "ByteArray"];


(*socketSend[socketId, data, length]: length*)
socketSend = 
LibraryFunctionLoad[$libFile, "socketSend", {Integer, {"ByteArray", "Shared"}, Integer}, Integer]; 


(*socketSend[socketId, data, length]: length*)
socketSendString = 
LibraryFunctionLoad[$libFile, "socketSendString", {Integer, String, Integer}, Integer]; 


(*serverCreate[listenSocket, clientsCapacity, bufferSize, selectTimeout]: serverPtr*)
serverCreate = 
LibraryFunctionLoad[$libFile, "serverCreate", {Integer, Integer, Integer, Integer}, Integer]; 


(*serverListen[serverPtr]: taskId*)
serverListen = 
LibraryFunctionLoad[$libFile, "serverListen", {Integer}, Integer]; 


End[]; 


EndPackage[]; 