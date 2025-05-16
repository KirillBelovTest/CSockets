#!/usr/bin/env wolframscript
(* ::Package:: *)

If[$FrontEnd === Null, 
    PacletDirectoryLoad[DirectoryName[$InputFileName]], 
    PacletDirectoryLoad[NotebookDirectory[]]
];
Get["KirillBelov`CSockets`TCP`"];
$ContextPath = DeleteDuplicates[Append[$ContextPath, "KirillBelov`CSockets`TCP`Private`"]];


img = ExportString[CountryData["World", "Shape"], "SVG"];


resp = StringToByteArray["HTTP/1.1 200 OK\r\nContent-Type: image/svg+xml\r\nContent-Length:" <> ToString[StringLength[img]] <> "\r\n\r\n" <> img]; 


server = socketOpen["localhost", "8080", 0, 1, 1, 256 * 1024, 256 * 1024]; 


serverPtr = serverCreate[server, 1024, 8192, 10000000]; 


task = Internal`CreateAsynchronousTask[
    serverListen, 
    {serverPtr}, 
    PreemptProtect[handler[##]]&
]; 


handler[task_, "Recv", {l_, s_, _}] := 
socketSend[s, resp, Length[resp]];


handler[task_, "Close", {l_, s_}] := 
socketClose[s]; 


While[True, Pause[1]];