#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
	PacletDirectoryLoad[Directory[]], 
(*Else*)
	PacletDirectoryLoad[NotebookDirectory[]]
]; 


Get["KirillBelov`CSockets`"]; 


server = CSocketOpen[8000]; 


handler[assoc_Association] := Module[{response}, 
	Echo[assoc]; 
	WriteString[assoc["SourceSocket"], "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ntest"]
]; 


listener = SocketListen[server, handler]; 


If[$FrontEnd === Null, 
	While[True, Pause[0.1]]
]; 
