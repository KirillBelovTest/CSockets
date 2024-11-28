#!/usr/bin/env wolframscript

Get["CCompilerDriver`"]; 
Get["LibraryLink`"]; 


$libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


build[lib_String] := Block[{$directory, $libSrc, $libDir, $linkerOptions}, 
    $directory = DirectoryName[If[$InputFileName == "", 
        NotebookFileName[], 
        $InputFileName
    ], 2]; 

    $libSrc = File[FileNameJoin[{
        $directory, 
        "Source", 
        lib <> ".c"
    }]]; 

    $libDir = FileNameJoin[{
        $directory, 
        "LibraryResources", 
        $SystemID <> "-v" <> ToString[$libraryLinkVersion]
    }]; 


    If[!FileExistsQ[$libDir], CreateDirectory[]];

    CreateLibrary[$libSrc, lib, 
        "TargetDirectory" -> $libDir, 
        "Debug" -> False, 
        (*"TransferProtocolLibrary" -> "WSTP", *)
        "ShellCommandFunction" -> Print
    ]
]; 


build["csockets"]; 


(*build["udp"]; *)