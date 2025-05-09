#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]]; 
Get["KirillBelov`CSockets`TCP`"];


server = TCPSocketOpen[8080]


selected = 
KirillBelov`CSockets`TCP`Private`socketSelect[{1788}, 1, 1000000]
