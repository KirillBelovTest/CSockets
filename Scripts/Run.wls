#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
	PacletDirectoryLoad[Directory[]], 
(*Else*)
	PacletDirectoryLoad[NotebookDirectory[]]
]; 


Get["KirillBelov`CSockets`"]; 


server = CSocketOpen[8000]; 


handler[assoc_Association] := Module[{n, response}, 
	n = ToExpression["n" /. Echo[URLParse[StringExtract[Echo[assoc["Data"]], " " -> 2]]["Query"]]]; 
	Print["n = ", n];
	Print[FullForm@n];
	If[IntegerQ[n], 
		response = StringToByteArray[StringTemplate["HTTP/1.1 200 OK\r\nContent-Length: `1`\r\n\r\n`2`"][n, StringJoin[ConstantArray["_", n]]]]; 
		Echo[AbsoluteTiming[BinaryWrite[assoc["SourceSocket"], response];][[1]], "Write Time:"], 
	(*Else*)
		WriteString[assoc["SourceSocket"], "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ntest"]
	]
]; 


listener = SocketListen[server, handler]; 


If[$FrontEnd === Null, 
	While[True, Pause[0.1]]
]; 


client = CSocketConnect[8000]; 


request = StringToByteArray["GET /test?n=4000000 HTTP/1.1\r\n"]; 
BinaryWrite[client, request]; 


msg = {}; 
AbsoluteTiming[While[SocketReadyQ[client], AppendTo[msg, SocketReadMessage[client]]]] 
Join@@msg


SocketReadyQ[client]


SocketReadMessage[client]
