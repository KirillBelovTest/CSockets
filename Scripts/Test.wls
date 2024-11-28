PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]]; 

<<KirillBelov`CSockets`

server = CSocketOpen["0.0.0.0", 50001];

lsitener = SocketListen[server, func];


func[packet_] := Module[{time, response, n = 1},

        Print[time = Now];
        Print["\n"];
        Print[packet];
        Print["\n"];
        Print[packet["Data"]];
        Print["\n"];

        response =
"HTTP/1.1 200 OK\r\n\
Connection: keep-alive\r\n\
Content-Type: text/html\r\n\
Content-Length: 4\r\n\r\n\
test"; 

        BinaryWrite[packet["SourceSocket"], StringToByteArray[response]];

        Print["Timing: ", Round[(AbsoluteTime[] - AbsoluteTime[time]) * 1000, 0.001], " ms"];
];

While[True, Pause[0.1]];