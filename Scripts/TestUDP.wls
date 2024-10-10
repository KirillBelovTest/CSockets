#!/usr/bin/env wolframscript
(* ::Package:: *)

$HistoryLength = 0; 

<<KirillBelov`CSockets`UDP`

server = UDPListen[8001]

UDPCreateListener[server, Function[$msg = ByteArrayToString@#]]

client = UDPConnect["127.0.0.1", 8001]

TimeConstrained[While[!UDPReadyQ[client], Pause[0.01]], 1]

UDPSendString[client, "Hello"]

TimeConstrained[While[True, 
	If[StringQ[$msg], 
		Echo[$msg]; 
		$msg = Null; 
	]
], 600]
