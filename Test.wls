#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
	PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
	PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];


server = KirillBelov`CSockets`TCP`Private`socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024];


sockets = {server}; 


selected = KirillBelov`CSockets`TCP`Private`socketSelect[sockets, Length[sockets], 1000000]


client = KirillBelov`CSockets`TCP`Private`socketAccept[server]


data = KirillBelov`CSockets`TCP`Private`socketRecv[client, 8 * 1024]


ByteArrayToString[ByteArray[data]]


KirillBelov`CSockets`TCP`Private`socketSend[client, 
	StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/html\r\n\r\nhello"], 
	StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/html\r\n\r\nhello"] // Length
]
