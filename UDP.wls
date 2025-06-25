#!/usr/bin/env wolframscript
(* ::Package:: *)

If[Head[$FrontEnd] === FrontEndObject, 
	PacletDirectoryLoad[NotebookDirectory[]], 
	PacletDirectoryLoad[Directory[]]
]; 


Get["KirillBelov`CSockets`TCP`"]; 


$ContextPath = DeleteDuplicates @ 
Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]; 


addr = socketAddressCreate["localhost", "8001", 
	$socketConstants["AF_INET"], 
	$socketConstants["SOCK_DGRAM"], 
	$socketConstants["IPPROTO_UDP"]
]


sock = socketCreate[
	$socketConstants["AF_INET"], 
	$socketConstants["SOCK_DGRAM"], 
	$socketConstants["IPPROTO_UDP"]
]


socketBind[sock, addr]


clnt = socketCreate[
	$socketConstants["AF_INET"], 
	$socketConstants["SOCK_DGRAM"], 
	$socketConstants["IPPROTO_UDP"]
]


socketConnect[clnt, addr]


socketSend[clnt, ByteArray[{1}], 1]


?socketSelect


socketSelect[{sock}, 1, 1000]


buf = socketBufferCreate[10]


data = socketRecv[1836, buf, 10]


Normal[data]
