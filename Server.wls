#!/usr/bin/env wolframscript

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];


Get["KirillBelov`CSockets`TCP`"];


socket = CSocketOpen[8080]; 


listener = SocketListen[socket, handler, "SelectTimeout" -> 5]; 


response["/"] := (
    Get["ServerResponse.wl"]; 
    "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nloaded"
)


handler = If[#Event === "Recv", 
    Module[{path = StringExtract[#["Data"], " " -> 2]}, 
        WriteString[#["SourceSocket"], 
            response[path]
        ]
    ]
]&


While[True, Pause[1]];