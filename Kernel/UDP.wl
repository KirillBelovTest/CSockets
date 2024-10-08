(* ::Package:: *)


BeginPackage["KirillBelov`CSockets`UDP`", {
    "CCompilerDriver`", 
	"LibraryLink`"
}]; 


(*Public*)


UDPSocketObject::usage = 
"UDPSocketObject[sockfd] socket representation."; 


UDPListen::usage = 
"UDPListen[port] starts listening of the specific port."; 


UDPConnect::usage = 
"UDPConnect[host, port] connect to the specifit addess."; 


UDPReadyQ::usage = 
"UDPReadyQ[socket] check that socket is ready for write or read."; 


UDPRead::usage = 
"UDPRead[socket] read byte array."; 


UDPReadString::usage = 
"UDPReadString[socket] read byte array and convert to thestring."; 


UDPSend::usage = 
"UDPSend[socket, bytArray] send byte array."; 


UDPSendString::usage = 
"UDPSendString[socket, string] convert string to byte array and send."; 


UDPClose::usage = 
"UDPClose[socket] close socket."


Begin["`Private`"]; 


(*Implementation*)


UDPListen[host_String, port_Integer] /; port > 1023 && StringMatchQ[host, NumberString ~~ "." ~~ NumberString ~~ "." ~~ NumberString ~~ "." ~~ NumberString] := 
With[{id = udpSocketListen[port]}, UDPSocketObject[id, "Read"]]; 


UDPListen[port_Integer] := 
UDPListen["127.0.0.1", oprt]; 


UDPConnect[host_String, port_Integer?Positive] /; StringMatchQ[host, NumberString ~~ "." ~~ NumberString ~~ "." ~~ NumberString ~~ "." ~~ NumberString] := 
With[{id = udpSocketConnect[host, port]}, UDPSocketObject[id, "Write"]]; 


UDPConnect[port_Integer] := 
UDPConnect["127.0.0.1", port]; 


Options[UDPReadyQ] = {
	"Timeout" :> $timeout
}; 


UDPReadyQ[UDPSocketObject[id_Integer, "Read"], OptionsPattern[]] := 
With[{result = udpSocketReadReadyQ[id, Round[OptionValue["Timeout"] * 1000000]]}, result] === 1; 


UDPReadyQ[UDPSocketObject[id_Integer, "Write"], OptionsPattern[]] := 
With[{result = udpSocketWriteReadyQ[id, Round[OptionValue["Timeout"] * 1000000]]}, result] === 1; 


UDPSend[UDPSocketObject[id_Integer, "Write"], byteArray_ByteArray] := 
(udpSocketSend[id, byteArray, Length[byteArray]];); 


UDPSendString[UDPSocketObject[id_Integer, "Write"], string_String] := 
(udpSocketSend[id, StringToByteArray[string], StringLength[string]];); 


Options[UDPRead] = {
	"BufferSize" :> $bufferSize
}; 


UDPRead[UDPSocketObject[id_Integer, "Read"], OptionsPattern[]] := 
With[{result = udpSocketRead[id, OptionValue["BufferSize"]]}, result]; 


Options[UDPReadString] = {
	"BufferSize" :> $bufferSize
}; 


UDPReadString[UDPSocketObject[id_Integer, "Read"], OptionsPattern[]] := 
With[{result = udpSocketRead[id, OptionValue["BufferSize"]]}, ByteArrayToString[result]]; 


UDPClose[UDPSocketObject[id_,_]] := 
With[{res = udpSocketClose[id]}, res;]; 


(*Internal*)


$directory = DirectoryName[$InputFileName, 2]; 


$libraryLinkVersion := $libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


Once[$libraryLinkVersion]; 


$libFile = FileNameJoin[{
	$directory, 
	"LibraryResources", 
	$SystemID <> "-v" <> ToString[$libraryLinkVersion], 
	"udp." <> Internal`DynamicLibraryExtension[]
}]; 


$bufferSize = 8 * 1024; 


$timeout = 0.001; 


udpSocketListen = LibraryFunctionLoad[$libFile, "udpSocketListen", {String, Integer}, Integer]; 


udpSocketRead = LibraryFunctionLoad[$libFile, "udpSocketRead", {Integer, Integer}, "ByteArray"]; 


udpSocketConnect = LibraryFunctionLoad[$libFile, "udpSocketConnect", {String, Integer}, Integer]; 


udpSocketSend = LibraryFunctionLoad[$libFile, "udpSocketSend", {Integer, {LibraryDataType[ByteArray], "Shared"}, Integer}, Integer]; 


udpSocketClose = LibraryFunctionLoad[$libFile, "udpSocketClose", {Integer}, Integer]; 


udpSocketReadReadyQ = LibraryFunctionLoad[$libFile, "udpSocketReadReadyQ", {Integer, Integer}, Integer]; 


udpSocketWriteReadyQ = LibraryFunctionLoad[$libFile, "udpSocketWriteReadyQ", {Integer, Integer}, Integer]; 


End[(*`Private`*)]; 


EndPackage[(*KirillBelov`CSockets`UDP`*)]; 