#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];


Get["KirillBelov`CSockets`TCP`"];


socket = CSocketOpen[8080]; 


listener = SocketListen[socket, handler]; 


handler = If[#Event === "Recv", 
    WriteString[#["SourceSocket"], 
        "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello"
    ]
]&


While[True, Pause[1]];
