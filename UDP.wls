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
	$socketConstants["SOCK_STREAM"], 
	$socketConstants["IPPROTO_TCP"]
]


sock = socketCreate[
	$socketConstants["AF_INET"], 
	$socketConstants["SOCK_STREAM"], 
	$socketConstants["IPPROTO_TCP"]
]


socketBind[sock, addr]


socketListen[sock, 2^32]


socketGetOpt[sock, $socketConstants["SOL_SOCKET"], $socketOptNames["SO_ACCEPTCONN"]]


$socketConstants["SOL_SOCKET"]


socketGetOpt[sock, $socketConstants["SOL_SOCKET"], $socketOptNames["SO_RCVBUF"]]


$socketOptNames["SO_TYPE"]


$socketConstants["SOL_SOCKET"]


socketGetOpt[sock, 65535, 16^^1008]


socketGetOpt[sock, $socketConstants["SOL_SOCKET"], $socketOptNames["SO_TYPE"]]


socketGetOpt[sock, $socketConstants["SOL_SOCKET"], $socketConstants["SO_ACCEPTCONN"]]


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
