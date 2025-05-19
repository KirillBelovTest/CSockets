#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];





Get["KirillBelov`CSockets`TCP`"];


client = CSocketConnect[8080]; 


WriteString[client, "test"];
