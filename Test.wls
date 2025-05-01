#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]]; 
Get["KirillBelov`CSockets`TCP`"];
server = CSocketOpen[8080];
listener = SocketListen[server, func];
log = {}; 
response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/html\r\n\r\nhello";



func[packet_] := (
    If[packet["Event"] === "Received", 
        WriteString[packet["SourceSocket"], response]
    ];
); 


server["Clients"]
