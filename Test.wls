#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
	PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
	PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];
$ContextPath = DeleteDuplicates[Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]];
server = socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024];



