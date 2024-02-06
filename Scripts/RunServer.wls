#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[Directory[]]


Get["KirillBelov`CSockets`"]; 


server = CSocketOpen[8000]; 


(*handler[assoc_Association] := Module[{length, body, path, file}, 
	path = StringTrim[StringSplit[assoc["Data"], "\r\n"][[1]], {"GET ", " HTTP/1.1"}]; 
	file = FileNameJoin[Flatten[{Directory[], FileNameSplit[path]}]]; 
	body = Import[file, "String"]; 
	length = StringLength[body]; 
	WriteString[assoc["SourceSocket"], StringTemplate["HTTP/1.1 200 OK\r\nContent-Length: `1`\r\n\r\n`2`"][length, body]]; 
];*)


handler[assoc_Association] := 
{}


listener = SocketListen[server, handler[#]&]; 


While[True, Pause[0.1]]; 
