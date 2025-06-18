#!/usr/bin/env wolframscript

Get["CCompilerDriver`"];
Get["LibraryLink`"];


$libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"];


build[lib_String, prot_: Automatic] := Block[{$directory, $libSrc, $libDir, $linkerOptions},
    $directory = DirectoryName[If[$InputFileName == "",
        NotebookFileName[],
        $InputFileName
    ], 1];

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

    CreateLibrary @@ {$libSrc, lib,
        "TargetDirectory" -> $libDir,
        "Debug" -> True,
        If[prot === Automatic, Nothing, "TransferProtocolLibrary" -> "WSTP"],
        "ShellCommandFunction" -> Print,
        "ShellOutputFunction" -> Print,
        "SystemCompileOptions" -> "/Ox /GL"
    }
];


build["csockets"];