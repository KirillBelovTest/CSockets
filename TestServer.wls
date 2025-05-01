PacletDirectoryLoad[FileNameJoin[{$HomeDirectory, "Projects", "Internal"}]]; 


PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]]; 


Get["KirillBelov`Internal`Console`"];


Get["KirillBelov`CSockets`TCP`"];


server = CSocketOpen[8080];


lsitener = SocketListen[server, func];


response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 5\r\n\r\nhello"; 


func[packet_] := 
(
    ConsoleEcho1[KeyDrop[packet, "DataBytes"], packet["Event"]]; 
    If[packet["Event"] === "Received", 
        WriteString[packet["SourceSocket"], response]; 
        ConsoleEcho1[response, "sent"];
    ];
) 


While[True, Pause[0.1]];