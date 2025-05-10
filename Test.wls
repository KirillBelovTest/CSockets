#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];



Get["KirillBelov`CSockets`TCP`"];


$ContextPath = DeleteDuplicates[Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]];


server = socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024];


sockets = {server};


While[True, 
    selected = socketSelect[sockets, Length[sockets], 5000000];

    Table[
        If[socket === server, 
            AppendTo[sockets, socketAccept[server]], 
            Check[
                data = socketRecv[socket, 4096];
                If[ByteArrayQ[data], 
                    Print @ ByteArrayToString @ data
                ], 

                sockets = DeleteCases[sockets, socket]; 
                socketClose[socket], 
                {LibraryFunction::sockclose, LibraryFunction::nonsock}
            ]; 
            socketSend[socket, 
                StringToByteArray[#], 
                StringLength[#]
            ]&["HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello"]
        ], 
        {socket, selected}
    ];
];