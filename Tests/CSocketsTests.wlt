BeginTestSection["CSocketsTests"]


Get["KirillBelov`CSockets`"]; 


If[ValueQ[client],  Close[client]]; 


If[ValueQ[listener],  DeleteObject[listener]]; 


If[ValueQ[server],  Close[server]]; 


server = CSocketOpen[8000]; 


listener = SocketListen[
	server, 
	If[#Event === "Received", 
		request = #Data; 
		BinaryWrite[#SourceSocket,  #DataByteArray]
	]&
]; 


client = CSocketConnect[8000]; 


VerificationTest[(* 1 *)
	WriteString[client,  "hello"]; request,
	"hello"	,
	TestID -> "EchoServerTest"
]


EndTestSection[]
