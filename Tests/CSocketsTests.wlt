BeginTestSection["CSocketsTests"]


Get["KirillBelov`CSockets`"]; 


If[ValueQ[client],  Close[client]]; 


If[ValueQ[listener],  DeleteObject[listener]]; 


If[ValueQ[server],  Close[server]]; 


server = CSocketOpen[8000]; 


listener = SocketListen[
	server, 
	If[#Event === "Received", 
		request = #; 
		BinaryWrite[#SourceSocket,  #DataByteArray]
	]&
]; 


client = CSocketConnect[8000]; 


VerificationTest[(* 1 *)
	WriteString[client,  "hello"]; Pause[0.01]; request["Data"],
	"hello",
	TestID -> "EchoServerTest"
]


VerificationTest[(* 2 *)
	WriteString[client,  "hello"]; Pause[0.01]; While[!SocketReadyQ[client], Pause[0.01]]; ByteArrayToString[SocketReadMessage[client]],
	"hello",
	TestID -> "EchoServerTest"
]


EndTestSection[]
