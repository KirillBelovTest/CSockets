(* ::Package:: *)

(* ::Chapter:: *)
(*CSocketListener*)


(* ::Section:: *)
(*Begin package*)


BeginPackage["KirillBelov`CSockets`"]; 


(* ::Section:: *)
(*Names*)


CSocketObject::usage = 
"CSocketObject[socketId] socket representation."; 


CSocketOpen::usage = 
"CSocketOpen[port] returns new server socket."; 


CSocketConnect::usage = 
"CSocketConnect[host, port] connect to socket."; 


CSocketListener::usage = 
"CSocketListener[assoc] listener object."; 


(* ::Section:: *)
(*Private context*)


Begin["`Private`"]; 


(* ::Section:: *)
(*Implementation*)


CSocketObject[socketId_Integer]["SocketId"] := 
socketId; 


CSocketObject[socketId_Integer]["DestinationPort"] := 
socketPort[socketId]; 


CSocketObject[socketId_Integer]["DestinationHostname"] := 
socketHostname[socketId]; 


CSocketObject[socketId_Integer]["ConnectedClients"] := 
Map[CSocketObject] @ socketClients[socketId]; 


CSocketOpen[host_String: "localhost", port_Integer] := 
CSocketObject[socketOpen[host, ToString[port]]]; 


CSocketOpen[address_String] /; 
StringMatchQ[address, __ ~~ ":" ~~ NumberString] := 
CSocketObject[Apply[socketOpen, StringSplit[address, ":"]]]; 


CSocketConnect[host_String: "localhost", port_Integer] := 
CSocketObject[socketConnect[host, ToString[port]]]; 


CSocketConnect[address_String] /; 
StringMatchQ[address, __ ~~ ":" ~~ NumberString] := 
CSocketObject[Apply[socketConnect, StringSplit[address, ":"]]]; 


CSocketObject /: BinaryWrite[CSocketObject[socketId_Integer], data_ByteArray] := 
socketBinaryWrite[socketId, data, Length[data], $bufferSize]; 


CSocketObject /: BinaryWrite[CSocketObject[socketId_Integer], data_List] := 
socketBinaryWrite[socketId, ByteArray[data], Length[data], $bufferSize];


CSocketObject /: WriteString[CSocketObject[socketId_Integer], data_String] := 
socketWriteString[socketId, data, StringLength[data], $bufferSize]; 


CSocketObject /: SocketReadMessage[CSocketObject[socketId_Integer], bufferSize_Integer: $bufferSize] := 
socketReadMessage[socketId, bufferSize]; 


CSocketObject /: SocketReadyQ[CSocketObject[socketId_Integer]] := 
socketReadyQ[socketId]; 


CSocketObject /: Close[CSocketObject[socketId_Integer]] := 
socketClose[socketId]; 


CSocketObject /: SocketListen[socket: CSocketObject[socketId_Integer], handler_, OptionsPattern[{SocketListen, "BufferSize" -> $bufferSize}]] := 
Module[{task}, 
	task = Internal`CreateAsynchronousTask[socketListen, {socketId, OptionValue["BufferSize"]}, handler[toPacket[##]]&]; 
	CSocketListener[<|
		"Socket" -> socket, 
		"Host" -> socket["DestinationHostname"], 
		"Port" -> socket["DestinationPort"], 
		"Handler" -> handler, 
		"TaskId" -> task[[2]], 
		"Task" -> task
	|>]
]; 


$cSocketObjectIcon = 
Import[FileNameJoin[{ParentDirectory[DirectoryName[$InputFileName]], "Images", "CSocketObjectIcon.png"}]]; 


CSocketObject /: MakeBoxes[socket: CSocketObject[socketId_Integer], form: (StandardForm | TraditionalForm)] := 
Module[{above, below}, 
	above = {
		{BoxForm`SummaryItem[{"SocketId: ", socketId}], SpanFromLeft}, 
		{BoxForm`SummaryItem[{"DestinationPort: ", socket["DestinationPort"]}], SpanFromLeft}, 
		{BoxForm`SummaryItem[{"DestinationHostname: ", socket["DestinationHostname"]}], SpanFromLeft}
	}; 
	below = {}; 
	
	BoxForm`ArrangeSummaryBox[CSocketObject, socket, $cSocketObjectIcon, above, below, form, "Interpretable" -> Automatic]
]; 


CSocketListener /: DeleteObject[CSocketListener[assoc_Association]] := 
socketListenerTaskRemove[assoc["TaskId"]]; 


CSocketListener[assoc_Association][key_String] := 
assoc[key]; 


$cSocketListenreIcon = 
Import[FileNameJoin[{ParentDirectory[DirectoryName[$InputFileName]], "Images", "CSocketListenerIcon.png"}]]; 


CSocketListener /: MakeBoxes[listener: CSocketListener[assoc_Association], form: (StandardForm | TraditionalForm)] := 
Module[{above, below}, 
	above = {
		{BoxForm`SummaryItem[{"TaskId: ", assoc["TaskId"]}], SpanFromLeft}, 
		{BoxForm`SummaryItem[{"Host: ", assoc["Host"]}], SpanFromLeft}, 
		{BoxForm`SummaryItem[{"Post: ", assoc["Port"]}], SpanFromLeft}
	}; 
	below = {
		{BoxForm`SummaryItem[{"Handler: ", assoc["Handler"]}], SpanFromLeft}
	}; 
	
	BoxForm`ArrangeSummaryBox[CSocketListener, listener, $cSocketListenreIcon, above, below, form, "Interpretable" -> Automatic]
]; 


(* ::Section:: *)
(*Internal*)


$directory = DirectoryName[$InputFileName, 2]; 


$libFile = FileNameJoin[{
	$directory, 
	"LibraryResources", 
	$SystemID, 
	"csockets." <> Internal`DynamicLibraryExtension[]
}]; 


$bufferSize = 8192; 


If[!FileExistsQ[$libFile], 
	Get[FileNameJoin[{$directory, "Scripts", "BuildLibrary.wls"}]]
]; 


toPacket[task_, event_, {serverId_, clientId_, data_}] := 
With[{byteArray = ByteArray[data]}, 
	<|
		"Socket" :> CSocketObject[serverId], 
		"SourceSocket" :> CSocketObject[clientId], 
		"DataByteArray" :> byteArray, 
		"Data" :> ByteArrayToString[byteArray]
	|>
]; 


socketOpen = LibraryFunctionLoad[$libFile, "socketOpen", {String, String}, Integer]; 


socketClose = LibraryFunctionLoad[$libFile, "socketClose", {Integer}, Integer]; 


socketListen = LibraryFunctionLoad[$libFile, "socketListen", {Integer, Integer}, Integer]; 


socketListenerTaskRemove = LibraryFunctionLoad[$libFile, "socketListenerTaskRemove", {Integer}, Integer]; 


socketConnect = LibraryFunctionLoad[$libFile, "socketConnect", {String, String}, Integer]; 


socketBinaryWrite = LibraryFunctionLoad[$libFile, "socketBinaryWrite", {Integer, {LibraryDataType[ByteArray], "Shared"}, Integer, Integer}, Integer]; 


socketWriteString = LibraryFunctionLoad[$libFile, "socketWriteString", {Integer, String, Integer, Integer}, Integer]; 


socketReadyQ = LibraryFunctionLoad[$libFile, "socketReadyQ", {Integer}, True | False]; 


socketReadMessage = LibraryFunctionLoad[$libFile, "socketReadMessage", {Integer, Integer}, "ByteArray"]; 


socketPort = LibraryFunctionLoad[$libFile, "socketPort", {Integer}, Integer]; 


socketHostname = LibraryFunctionLoad[$libFile, "socketHostname", {Integer}, String]; 


socketClients = LibraryFunctionLoad[$libFile, "socketClients", {Integer}, {Integer, 1}]; 


(* ::Section:: *)
(*End private context*)


End[]; 


(* ::Section:: *)
(*End package*)


EndPackage[]; 
