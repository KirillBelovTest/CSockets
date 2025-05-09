#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
	PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
	PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];
server = KirillBelov`CSockets`TCP`Private`socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024];


Echo[server, "SERVER:"];


sockets = {server}; 


selected = KirillBelov`CSockets`TCP`Private`socketSelect[sockets, Length[sockets], 1000000];


selected


client = KirillBelov`CSockets`TCP`Private`socketAccept[server];


client


data = KirillBelov`CSockets`TCP`Private`socketRecv[client, 8 * 1024];
