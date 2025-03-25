(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


TCPClientSocket::usage = 
"TCPClientSocket[socketId] socket representation."; 


TCPServerSocket::usage = 
"TCPServerSocket[serverPtr] server representation."; 

TCPSocketOpen::usage = 
"CSocketOpen[port] returns new server socket opened on localhost.
CSocketOpen[host, port] returns new server socket opened on specific host."; 


TCPSocketConnect::usage = 
"CSocketConnect[port] connect to socket on localhost.
CSocketConnect[host, port] connect to socket."; 


TCPSocketListener::usage = 
"TCPSocketListener[assoc] listener object."; 


Begin["`Private`"]; 


Options[TCPSocketOpen] = {
    "BufferSize" :> $bufferSize
};


TCPSocketOpen[host_String: "localhost", port_Integer, OptionsPattern[]] := 
TCPServerSocket[socketOpen[host, ToString[port], OptionValue["BufferSize"]]]; 


Options[TCPSocketConnect] = {
    "BufferSize" :> $bufferSize
};


TCPSocketConnect[host_String: "localhost", port_Integer, OptionsPattern[]] := 
TCPClientSocket[socketConnect[host, ToString[port]], OptionValue["BufferSize"]]; 


TCPClientSocket /: BinaryWrite[TCPClientSocket[socketId_Integer], data_ByteArray, opts: OptionsPattern[{"BufferSize" :> $bufferSize}]] := 
socketBinaryWrite[socketId, data, Length[data], OptionValue[Flatten[{opts}], "BufferSize"]]; 


TCPClientSocket /: WriteString[TCPClientSocket[socketId_Integer], data_String, opts: OptionsPattern[{"BufferSize" :> $bufferSize}]] := 
socketWriteString[socketId, data, StringLength[data], OptionValue[Flatten[{opts}], "BufferSize"]]; 


TCPClientSocket /: SocketReadMessage[TCPClientSocket[socketId_Integer]] := 
socketReadMessage[socketId, $bufferSize]; 


TCPClientSocket /: SocketReadMessage[TCPClientSocket[socketId_Integer], bufferSize_Integer] := 
socketReadMessage[socketId, bufferSize]; 


TCPClientSocket /: SocketReadyQ[TCPClientSocket[socketId_Integer]] := 
socketReadyQ[socketId]; 


TCPClientSocket /: Close[TCPClientSocket[socketId_Integer]] := 
socketClose[socketId]; 


TCPServerSocket /: SocketListen[server: TCPServerSocket[serverPtr_Integer], handler_, OptionsPattern[{SocketListen, "BufferSize" :> $bufferSize}]] := 
Module[{task}, 
    task = Internal`CreateAsynchronousTask[socketListen, {serverPtr}, handler[toPacket[##]]&]; 
    TCPListener[task, server, handler]
]; 


TCPSocketListener /: DeleteObject[TCPSocketListener[taskId_, ___]] := 
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


$bufferSize = 8 * 8192; 


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
socketListen = LibraryFunctionLoad[$libFile, "socketListen", {Integer}, Integer]; 


(*socketConnect["host", "port", bufferSize] -> socketId*)
socketConnect = LibraryFunctionLoad[$libFile, "socketConnect", {String, String, Integer}, Integer]; 


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