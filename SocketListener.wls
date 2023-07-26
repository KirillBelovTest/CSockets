#!/usr/bin/env wolframscript
(* ::Package:: *)

Get["CCompilerDriver`"]; 
libSrc = File["socket_listener.c"]; 
If[Head[createServer] == LibraryFunction, LibraryFunctionUnload[createServer]]; 
libBin = CreateLibrary[libSrc, "socket_listener", 
	"Debug" -> True, 
	"TargetDirectory" -> Directory[], 
	"ShellOutputFunction" -> Print, 
	"ShellCommandFunction" -> Print
]; 
createServer = LibraryFunctionLoad[libBin, "create_server", {String}, Integer]; 
task = Internal`CreateAsynchronousTask[createServer, {"8000"}, Print[packet = {##}]&]

While[True, Pause[1]];
