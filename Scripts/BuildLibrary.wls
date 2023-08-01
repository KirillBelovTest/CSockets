#!/usr/bin/env wolframscript
(* ::Package:: *)

LINUXLIVUVPATH = "/home/kirill/Downloads/libuv-1.46.0/build/libuv.so"
MACOSXLIBUVPATH = "/usr/local/lib/libuv.a"

Get["CCompilerDriver`"]; 

(*$CCompiler={"Compiler"->GenericCCompiler,
 "CompilerInstallation"->"C:\\mingw64\\bin",
 "CompilerName"->"gcc.exe",
 "ShellOutputFunction"->Print}*)

OSLibs = If[$OperatingSystem === "Windows", {"ws2_32"}, {}];
OSPath = If[$OperatingSystem === "Windows", "Windows", "Unix"];

OSLinker = If[$OperatingSystem === "Windows", {}, If[$OperatingSystem === "MacOSX", {MACOSXLIBUVPATH<>" -pthread"}, {LINUXLIVUVPATH<>" -pthread"}]];

Block[{$directory, $libSrc, $libDir}, 
	$directory = DirectoryName[If[$InputFileName == "", 
		NotebookFileName[], 
		$InputFileName
	], 2]; 


	$libSrc = File[FileNameJoin[{
		$directory, 
		"Source", 
		OSPath,
		"socket_listener.c"
	}]]; 


	$libDir = FileNameJoin[{
		$directory, 
		"LibraryResources", 
		$SystemID
	}]; 


	If[!FileExistsQ[$libDir], CreateDirectory[]];


	Get["CCompilerDriver`"]; 

	CreateLibrary[$libSrc, "socket_listener" , "TargetDirectory" -> $libDir, "Debug"->True, "LinkerOptions"->OSLinker,	"Libraries"->OSLibs]
]; 
