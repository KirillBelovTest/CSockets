(* :Package: *)

response["/test"] := 
"HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nTest"; 


htmlPageQ[path_] := 
StringMatchQ[path, "/" ~~ __ ~~ ".html"];


response[path_?htmlPageQ] :=
StringTemplate[
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: `2`\r\n\r\n`1`"
][#, Length[#]]& @ 
ReadString[FileNameJoin[FieNameSplit[path]]];
