#!/usr/bin/env wolframscript
(* ::Package:: *)

$directory = If[$FrontEnd === Null, 
    Directory[], 
    NotebookDirectory[]
];


PacletDirectoryLoad[$directory];


Get["KirillBelov`CSockets`TCP`"];


addr = CSocketAddressCreate["localhost", 8080];


sock = CSocketCreate[addr];


CSocketBind[sock, addr];


DeleteObject[addr];


Close[sock];
