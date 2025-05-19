(* :Package: *)

response[_String] := 
"HTTP/1.1 404 Not found\r\nContent-Length: 3\r\n\r\n404";


response["/test"] := 
"HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ntest"; 