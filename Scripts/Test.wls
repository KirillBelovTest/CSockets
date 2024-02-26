#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[Directory[]]

<<KirillBelov`CSockets`

server = CSocketOpen[8000]

Print[server["DestinationHostname"]]
