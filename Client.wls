#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];


<<KirillBelov`LTP`


<<KirillBelov`CSockets`TCP`


c1 = CSocketConnect[8080]


data = ByteArray[RandomInteger[{0, 255}, 32 * 1024 * 1024]]


AbsoluteTiming[BinaryWrite[c1, data]]


res = SocketReadMessage[c1]


SocketReadyQ[c1]


ByteArrayToString@ByteArray[CompressedData["
1:eJztyjEKgCAAQFFbBddmO0ClDQ2tLUFBEV4gSWoIXbw/dY3gv/lVPu2+EJ9S
iMm5rbWN1Z0xep2VHFPMIeZ6CfHK96B7JZV80nGGk8vlcrlcLpfL5XK5XC6X
y+Vyuf+5L/fZC5g=
"]]


LTPSend[c1, "hw"]


jc = JavaNew["kirillbelov.ltp.LTPClient", 8080]


jc@send["hw"]


Get["KirillBelov`CSockets`TCP`"];


client = CSocketConnect[8080]; 


WriteString[client, "test"];
