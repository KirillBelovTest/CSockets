Once[Get["KirillBelov`Internal`Console`"]]

Once[
    img = ExportString[
        Plot[{Sin[x], Cos[x]}, {x, -5, 5}], 
        "SVG"
    ]
];

Function[
    If[#Event === "Received", 
        Print[#SourceSocket]; 
        Print[ByteArrayToString[#DataByteArray]]; 

(*        WriteString[#SourceSocket, 
        
"HTTP/1.1 200 OK\r\n\
Content-Type: image/svg+xml\r\n\
Content-Length: " <> ToString[StringLength[img]] <> "\r\n\
\r\n\
" <> img

        ]*)
    ]
]