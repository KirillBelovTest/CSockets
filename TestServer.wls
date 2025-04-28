PacletDirectoryLoad[ParentDirectory[DirectoryName[$InputFileName]]]; 


Get["KirillBelov`CSockets`TCP`"];


server = CSocketOpen[8080];


lsitener = SocketListen[server, func];


func[packet_] := Get["TestHandler.wl"][packet]; 


While[True, Pause[0.1]];