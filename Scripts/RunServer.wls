#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[Directory[]]

Get["KirillBelov`CSockets`"]; 

server = CSocketOpen[8000]


listener = SocketListen[server, Echo]


While[True, Pause[0.1]]