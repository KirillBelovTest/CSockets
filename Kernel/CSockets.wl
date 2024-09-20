(* ::Package:: *)

(* ::Chapter:: *)
(*CSocketListener*)


(* ::Section:: *)
(*Begin package*)


BeginPackage["KirillBelov`CSockets`", {
	"CCompilerDriver`", 
	"LibraryLink`"
}]; 


(* ::Section:: *)
(*Names*)


CSocketObject::usage = 
"CSocketObject[socketId] socket representation."; 


CSocketOpen::usage = 
"CSocketOpen[port] returns new server socket opened on localhost.
CSocketOpen[host, port] returns new server socket opened on specific host."; 


CSocketConnect::usage = 
"CSocketConnect[port] connect to socket on localhost.
CSocketConnect[host, port] connect to socket."; 


CSocketListener::usage = 
"CSocketListener[assoc] listener object."; 


CSocketHandler::usage = 
"CSocketHandler[opts] handler."; 


(* ::Section:: *)
(*Private context*)


Begin["`Private`"]; 


(* ::Section:: *)
(*Implementation*)


Options[CSocketHandler] = {
	"Logger" :> Function[#], 
	"Buffer" :> CreateDataStructure["HashTable"], 
	"Serializer" :> Function[#], 
	"Deserializer" :> Function[#], 
	"Accumulator" :> CreateDataStructure["HashTable"], 
	"DefaultAccumulator" :> Function[Length[#DataByteArray]], 
	"Handler" :> CreateDataStructure["HashTable"], 
	"DefaultHandler" :> Function[Null], 
	"AcceptHandler" :> Function[Null], 
	"CloseHandler" :> Function[Null]
}; 


CSocketHandler[OptionsPattern[]] := 
With[{store = CreateDataStructure["HashTable"]}, 
	Map[store["Insert", # -> assocToStruct[OptionValue[#]]]&] @ Keys[Options[CSocketHandler]]; 
	System`Private`SetValid[CSocketHandler[store]]
]; 


assocToStruct[arg_] := arg; 


assocToStruct[assoc_Association] := 
CreateDataStructure["HashTable", assoc]; 


CSocketHandler[store_][key_String] := 
store["Lookup", key]; 


CSocketHandler /: 
Set[CSocketHandler[store_][key_String], value_] := 
(store["Insert", key -> value]; value); 


Unprotect[Set]; 


Set[(handler_?(System`Private`ValidQ))[prop_String], value_] := 
With[{$handler = handler}, $handler[prop] = value]; 


Set[(handler_?(System`Private`ValidQ))[prop_String, key_], value_] := 
(handler[prop]["Insert", key -> value]; value); 


Protect[Set]; 


(handler_CSocketHandler?System`Private`ValidQ)[packet_Association] := 
Module[{logger, extendedPacket, result, extraPacket, extraPacketDataLength}, 
	Which[
		packet["Event"] === "Received", 

		extendedPacket = getExtendedPacket[handler, packet]; (*Association[]*)

		If[extendedPacket["Completed"], 
			With[{message = getMessage[handler, extendedPacket]}, 
				extendedPacket["DataByteArray"] := message; (*ByteArray[]*)
				extendedPacket["Data"] := ByteArrayToString[message];
				extendedPacket["DataBytes"] := Normal[message];
				With[{content = handler["Deserializer"][message]}, 
					extendedPacket["Message"] := content; 
				]; 
			]; 

			result = handler["Serializer"] @ invokeHandler[handler, extendedPacket]; (*ByteArray[] | _String | Null*)

			sendResponse[handler, packet, result]; 

			If[extendedPacket["StoredLength"] > extendedPacket["ExpectedLength"], 
				extraPacket = packet; 
				extraPacketDataLength = extendedPacket["StoredLength"] - extendedPacket["ExpectedLength"]; 
				extraPacket["DataByteArray"] = packet["DataByteArray"][[-extraPacketDataLength ;; ]]; 
				clearBuffer[handler, packet]; 
				handler[extraPacket], 
			(*Else*)
				clearBuffer[handler, extendedPacket]
			]; 
			
			Return[result], 
		(*Else*)
			savePacketToBuffer[handler, extendedPacket]
		]; , 

		packet["Event"] === "Accepted", 
			handler["AcceptHandler"][packet], 

		packet["Event"] === "Closed", 
			handler["CloseHandler"][packet]
	]; 
]; 


getExtendedPacket[handler_, packet_] := 
With[{uuid = packet["SourceSocket"][[1]]}, 
	Module[{
		dataLength, 
		last, 
		expectedLength, 
		storedLength, 
		completed, 
		completeHandler, 
		defaultCompleteHandler, 
		extendedPacket, 
		buffer
	}, 
		dataLength = Length[packet["DataByteArray"]]; 
		buffer = handler["Buffer"]["Lookup", uuid]; (*DataStructure[DynamicArray]*)

		If[!MissingQ[buffer] && buffer["Length"] > 0, 
			last = buffer["Part", -1]; (*Association[]*) 
			expectedLength = last["ExpectedLength"]; 
			storedLength = last["StoredLength"];, 

		(*Else*)
			expectedLength = conditionApply[handler["Accumulator"], handler["DefaultAccumulator"]][packet]; 
			storedLength = 0; 
		]; 

		completed = storedLength + dataLength >= expectedLength; 

		(*Return: Association[]*)
		Join[packet, <|
			"Completed" -> completed, 
			"ExpectedLength" -> expectedLength, 
			"StoredLength" -> storedLength + dataLength, 
			"DataLength" -> dataLength
		|>]
	]
]; 


getMessage[handler_, extendedPacket_] := 
With[{
	buffer = handler["Buffer"]["Lookup", extendedPacket["SourceSocket"][[1]]], 
	expectedLength = extendedPacket["ExpectedLength"]
}, 
	If[!MissingQ[buffer] && buffer["Length"] > 0, 

		(*Return: _ByteArray*)
		Part[#, 1 ;; expectedLength]& @ 
		Apply[Join] @ 
		Append[extendedPacket["DataByteArray"]] @ 
		buffer["Elements"][[All, "DataByteArray"]], 

	(*Else*)

		(*Return: _ByteArray*)
		extendedPacket["DataByteArray"][[1 ;; expectedLength]]
	]
]; 


invokeHandler[handler_, packet_] := 
Module[{messageHandler, defaultMessageHandler}, 
	messageHandler = handler["Handler"]; 
	defaultMessageHandler = handler["DefaultHandler"]; 

	(*Return: ByteArray[] | _String | Null*)
	conditionApply[messageHandler, defaultMessageHandler][packet]
]; 


CSocketHandler::cntsnd = 
"Can't send result to the client\n `1`"; 


Format[handler_CSocketHandler, InputForm] := 
SequenceForm[CSocketHandler][Normal[Normal[handler[[1]]]]]; 


sendResponse[handler_, packet_, result: _ByteArray | _String | Null] := 
With[{client = packet["SourceSocket"]}, 
	Switch[result, 
		_String, 
			WriteString[client, result], 
		
		_ByteArray, 
			BinaryWrite[client, result], 

		Null, 
			Null
	]
]; 


sendResponse[_, _, result_] := 
Message[CSocketHandler::cntsnd, result]; 


savePacketToBuffer[handler_, extendedPacket_] := 
With[{
	buffer = handler["Buffer"]["Lookup", extendedPacket["SourceSocket"][[1]]], 
	uuid = extendedPacket["SourceSocket"][[1]]
}, 
	If[!MissingQ[buffer], 
		buffer["Append", extendedPacket], 
		handler["Buffer"]["Insert", uuid -> CreateDataStructure["DynamicArray", {extendedPacket}]]
	]
]; 


clearBuffer[handler_, packet_] := 
With[{buffer = handler["Buffer"]["Lookup", packet["SourceSocket"][[1]]]}, 
	buffer["DropAll"]; 
]; 


conditionApply[conditionAndFunctions_: <||>, defalut_: Function[Null], ___] := 
Function[Last[SelectFirst[conditionAndFunctions, Function[cf, First[cf][##]], {defalut}]][##]]; 


conditionApply[hashTable_?(DataStructureQ[#, "HashTable"]&), rest___] := 
conditionApply[Normal[hashTable], rest]; 


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


$libraryLinkVersion := $libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


Once[$libraryLinkVersion]; 


$libFile = FileNameJoin[{
	$directory, 
	"LibraryResources", 
	$SystemID <> "-v" <> ToString[$libraryLinkVersion], 
	"csockets." <> Internal`DynamicLibraryExtension[]
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
