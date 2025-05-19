#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];


<<KirillBelov`LTP`


c1 = CSocketConnect[8080]


LTPSend[c1, "hw"]


jc = JavaNew["kirillbelov.ltp.LTPClient", 8080]


jc@send["hw"]


Get["KirillBelov`CSockets`TCP`"];


client = CSocketConnect[8080]; 


WriteString[client, "test"];
