#!/usr/bin/env wolframscript
(* ::Package:: *)

$directory = If[$FrontEnd === Null, 
	Directory[], 
	NotebookDirectory[]
];


$lib = FileNameJoin[{
	$directory, 
	"LibraryResources\\Windows-x86-64-v8\\csockets.dll"
}];


addrCreate = LibraryFunctionLoad[$lib, 
	"socketAddressCreate", {String, String}, Integer]; 


addrRemove = LibraryFunctionLoad[$lib, 
	"socketAddressRemove", {Integer}, Integer]; 


addr = addrCreate["localhost", "8080"]; 


addrRemove[addr];
