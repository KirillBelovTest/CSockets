#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];
$ContextPath = DeleteDuplicates[Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]];


resp = StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\nhello"]; 


server = socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024]; 


serverPtr = serverCreate[server, 1024, 8192, 10000000]; 


task = Internal`CreateAsynchronousTask[
	serverListen, 
	{serverPtr}, 
	handler[##]&
]; 


handler[task_, "Recv", {l_, s_, _}] := 
With[{
	d = StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\nhello"]
}, 
	Echo[{l, s}, "Recv"];
	socketSend[s, d, Length[d]]
]


handler[task_, "Close", {l_, s_}] := 
socketClose[s]; 
