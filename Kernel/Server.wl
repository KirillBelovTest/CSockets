(* :Package: *)

BeginPackage["KirillBelov`CSockets`Server`"]; 


CSocketCreateServer::usage = 
"CSocketCreateServer[port, func] creates CSocketServer."; 


Begin["`Private`"]; 


Options[CSocketCreateServer] = {
    "Host" :> "localhost", 
    "Port" :> RandomInteger[20000, 60000], 
    "SocketOpen" :> KirillBelov`CSockets`TCP`CSocketOpen, 
    "Logger" :> Function[#], 
    "Buffer" :> CreateDataStructure["HashTable"], 
    "Serializer" :> Function[#], 
    "Deserializer" :> Function[#], 
    "Accumulator" :> <||>, 
    "DefaultAccumulator" :> Function[Length[#DataByteArray]], 
    "Handler" :> <||>, 
    "DefaultHandler" :> Function[Null], 
    "AcceptHandler" :> Function[Null], 
    "CloseHandler" :> Function[Null]
}; 


CSocketCreateServer[opts: OptionsPattern[]] := 
With[{
    $port = OptionValue["Port"], 
    $host = OptionValue["Host"], 
    $handlerOpts = FilterRules[Flatten[{opts}], Options[CSocketHandler]]
}, 
    With[{
        $socket = OptionValue["SocketOpen"][$host, $port], 
        $handler = CSocketHandler[$handlerOpts]
    }, 
        SocketListen[$socket, $handler]
    ]
]; 


CSocketCreateServer[port_Integer, func_, opts: OptionsPattern[]] := 
CSocketCreateServer["Port" -> port, "DefaultHandler" -> func, opts]; 


CSocketCreateServer[func_, opts: OptionsPattern[]] := 
CSocketCreateServer["DefaultHandler" -> func, opts]; 


CSocketCreateServer[assoc_?AssociationQ, opts: OptionsPattern[]] := 
CSocketCreateServer["Handler" -> assoc, opts]; 


End[]; 


EndPackage[]; 
