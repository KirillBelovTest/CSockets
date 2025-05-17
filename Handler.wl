(* :Package: *)


response[_String] := 
"HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\n200";


response["/test"] := 
"HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nTest"; 


response[path_String?getFileQ] := 
getFile[path]; 


getFileQ[path_] := 
FileExistsQ[FileNameJoin[URLParse[StringTrim[path, "/"]]["Path"]]]; 


getFile[path_] := 
Module[{
    file = FileNameJoin[URLParse[StringTrim[path, "/"]]["Path"]]
}, 
    If[DirectoryQ[file], file = FileNameJoin[{file, "index.html"}]]; 

    mimeType = getMIMEType[file]; 

    StringTemplate[
        "HTTP/1.1 200 OK\r\n" <> 
        "Content-Length: `1`\r\n" <> 
        "Content-Type: `2`\r\n" <> 
        "Connection: keep-alive\r\n\r\n`3`"
    ][mimeType, StringLength[#], #]& @ ReadString[file]
]; 


getMIMEType[file_] := 
ToUpperCase[FileExtension[file]] /. {
    "HTML" -> "text/html", 
    "TXT" -> "text/plain", 
    "PNG" -> "image/png", 
    "SVG" -> "image/svg+xml", 
    "ICO" -> "image/png"
}