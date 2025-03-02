#!/usr/bin/env wolframscript
(* ::Package:: *)

Get["CCompilerDriver`"]; 
Get["LibraryLink`"]; 


$libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


build[lib_String, prot_: Automatic] := Block[{$directory, $libSrc, $libDir, $linkerOptions}, 
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
        If[prot === Automatic, Nothing, "TransferProtocolLibrary" -> "WSTP"],
        "ShellCommandFunction" -> Print, 
        "SystemCompileOptions" -> "/Ox /GL"
    ]
]; 


build["tcp"]; 


build["udp"]; 
