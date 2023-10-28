#!/usr/bin/env wolframscript
(* ::Package:: *)

PacletDirectoryLoad[Directory[]]


Get["KirillBelov`CSockets`"]; 


server = CSocketOpen[8000]


timelist = {};


handler[___] := AppendTo[timelist, AbsoluteTime[]]


listener = SocketListen[server, handler[#]&]


client = CSocketConnect[8000]


timeWrite = AbsoluteTiming[BinaryWrite[client, ByteArray[RandomInteger[{100, 200},  1024 * 1024]]]]; 


Pause[1]


Print[Mean[timeWrite[[All, 1]]]]
Print[timelist[[-1]] - timelist[[1]]]
