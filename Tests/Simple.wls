PacletInstall["JerryI/LPM"]
<< JerryI`LPM`

PacletRepositories[{
  Github -> "https://github.com/KirillBelovTest/Objects",
  Github -> "https://github.com/KirillBelovTest/Internal",
  Github -> "https://github.com/KirillBelovTest/TCPServer",
  Github -> "https://github.com/KirillBelovTest/HTTPHandler",
  Github -> "https://github.com/KirillBelovTest/WebSocketHandler",
  Github -> "https://github.com/JerryI/wl-wsp",
  Github -> "https://github.com/JerryI/wl-misc"
}]


SetDirectory[$InputFileName // DirectoryName]
Get[FileNameJoin[{ParentDirectory[Directory[]], "Kernel", "CSockets.wl"}]]

<<KirillBelov`Objects`
<<KirillBelov`Internal`
<<KirillBelov`TCPServer`

<<KirillBelov`HTTPHandler`
<<KirillBelov`HTTPHandler`Extensions`

<<JerryI`WSP`

$PublicDirectory = {FileNameJoin[{Directory[], "Simple"}]}


Print["Staring HTTP server..."];

tcp = TCPServer[];
tcp["CompleteHandler", "HTTP"] = HTTPPacketQ -> HTTPPacketLength;
tcp["MessageHandler", "HTTP"] = HTTPPacketQ -> http;

http = HTTPHandler[];

http["MessageHandler", "Index"] = AssocMatchQ[<|"Path" -> "/"|>] -> (HypertextProcess[#, "index.wsp", "Base" -> $PublicDirectory] &)
http["MessageHandler", "WSP"] = GetFileRequestQ[{"wsp"}] -> (HypertextProcess[#, "Base" -> $PublicDirectory] &)
http["MessageHandler", "Files"] = GetFileRequestQ[{"png", "html", "js"}] -> (ImportFile[#, "Base" -> $PublicDirectory] &)

(* ::End::*)
SocketListen[CSocketOpen[8010], tcp@# &];

SetOptions[WSPEngine, "Cache" -> False];

"open http://127.0.0.1:8010/" // Print;
While[True, Pause[1]];

