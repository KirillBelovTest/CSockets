#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];


Get["KirillBelov`CSockets`TCP`"];


Get["KirillBelov`CSockets`Handler`"];


socket = CSocketOpen[8080]; 


handler = CSocketHandler[];


handler["DefaultHandler"] = Function[packet, 
    If[packet["Event"] === "Recv", 
        Get["ServerResponse.wl"];
        WriteString[packet["SourceSocket"], 
            "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nloaded"
        ]
    ]
];


listener = SocketListen[socket, handler, "SelectTimeout" -> 2.5]; 


While[True, Pause[1]];