#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];
$ContextPath = DeleteDuplicates[Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]];
server = socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024]


client = socketConnect["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024]


socketAccept@server


socketRecv[1428, 1024]


Normal[ByteArray[{1, 2, 3}]]


serverPtr = serverCreate[server, 1024, 8192, 5000000]
task = Internal`CreateAsynchronousTask[
	serverListen, 
	{serverPtr}, 
	handler[Echo[{##}]]&
]


handler[{task_, "Recv", {l_, s_}}] := 
With[{
	d = StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\nhello"]
}, 
	socketSend[s, d, Length[d]]
]


client = socketConnect["localhost", "8080", 1, 1, 1, 256 * 1024, 256 * 1024]


WriteString[client, "hello"]


Close[client]



sockets = {server}; 
response = StringToByteArray["HTTP/1.1 200 OK\r\nContent-Length:5\r\n\r\nhello"]; 


Dynamic[Refresh[sockets]]


Button["select", selected = socketSelect[sockets, Length[sockets], 1000000]]


Button["accept", If[MemberQ[selected, server], 
	AppendTo[sockets, client = socketAccept[server]];
	sockets = socketCheck[sockets, Length[sockets]]
]]


Button["send", 
	Map[socketSend[#, response, Length[response]]&, DeleteCases[selected, server]]
]


socketRecv[1912, 4096]


socketCheck[sockets, Length[sockets]]


(# -> socketRecv[#, 4096])& /@ socketCheck[sockets, Length[sockets]]


ByteArrayToString @ ByteArray[CompressedData["
1:eJzFU8lu2zAQdQ+9GPA/TH1KXFGS62yVERRumtpFFhRekDNFjSUiFKlQlGPk
6ztS7NRpCgToIYUAYfQ423t8+hCbafyu1Wr13rda4/M5BDCZz38Gfb/faU9M
6SJQRnCV1eFJeBJ22mdGaxROGh3BLWLBuJIrJJyLDBmdOmtUBDlfM57iKVWU
KJjIWMUj6J5l1uSyyrvD1Wm3PzjoetC9Ni4asa+W66SBPzXo2JhUITQF+JS+
043lJpYKI/iyO4MVirulsTkNu5E6MfclFS2K1PIE2Q9NiZVFNsW7CktXRkBE
FyVaNkpRE90r8yCV4sGhH8LepgFcz6Ef+uEQCDg6GML66GAfRkWh8AbjC+mC
w8GxPziCvYvJ/OrSAyVvEcYobs3+hkBAy/th/cCML7mVm5JOeyQEFjTY4doF
mcuVx6mxFLyWOFjXyMf1n2iuhnenof/ZkzmJHPCVXG7Ce4yLLVro1OsFvSb1
5FmDUqYaE4ZrkXGdIqkbD5q04057RlJ+R0dqzqQjebXRuItemaRGaWbK3bOT
Wke6jv4u9g1rEyVGVDnpu6XLzrUwidRpBOmDLDxIcEn3hh7E1oOH0iVPmZe0
YEV0IrAVmy48W22oo2aL2YYb6u32nXZj4yWtJ4z26fXPjn7NU//TNW/xT22d
+arHHsNylTZGffzq/XbeS0uVPEdmrEyl/ouzDBPGli9N1DTutKe4RFsbLXOu
iILg+XUGb2GxXyxYnsI=
"]]


f[1] := Message[f::x1]
f[2] := Message[f::x2]
f[] := {Message[f::x1], Message[f::x2]}


Check[Check[f[1], Print @ 1, f::x1], Print[2], f::x2]
Check[Check[f[2], Print @ 1, f::x1], Print@2, f::x2]
Check[Check[f[], Print @ 1, f::x1], Print @ 2, f::x2]


Fold[Unevaluated[Check[#1, #2[[1]], #2[[2]]]]&, Unevaluated[f[]], {f::x1 :> Print[1], f::x2 :> Print[2]}]


SetAttributes[CheckReturn, HoldFirst]


CheckReturn[expr_, msgs: (RuleDelayed[_MessageName, _])..] := 



NestList[Hold[expr, #[[1]], #[[2]]]&, {1, 2}, {{3, 4}, {5, 6}}]


FoldList[f,x,{a,b,c,d}]


msgs = {f::x :> 1, f::y :> 2}


Fold[Unevaluated[Check[x, #[[1]], #[[1]] /. #]]&, 1+1, msgs]
