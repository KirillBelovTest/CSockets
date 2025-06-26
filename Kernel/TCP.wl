(* ::Package:: *)

(* TCP Server
    address = socketAddressCreate[host, port]
    socket = socketCreate[address]
    socketBind[socket, address]
    socketBlockingMode[socket, 1|0]
    DeleteObject[address]
    socketSetOpt[socket, "IPPROTO_TCP", "TCP_NODELAY", 1|0]
    socketSetOpt[socket, "SOL_SOCKET", "SO_KEEPALIVE", 1|0]
    socketSetOpt[socket, "SOL_SOCKET", "SO_RCVBUF", bufferSize]
    socketSetOpt[socket, "SOL_SOCKET", "SO_SNDBUF", bufferSize]
    socketListen[socket]
*)


BeginPackage["KirillBelov`CSockets`TCP`", {
    "CCompilerDriver`", 
    "LibraryLink`"
}]; 


CSocketObject::usage = 
"CSocketObject[socketId] socket object representation.";


CSocketOpen::usage = 
"CSocketOpen[host, port, protocol] returns a new server socket opened on specific host basd on protocol.";


CSocketConnect::usage = 
"CSocketConnect[host, port, protocol] returns a new client socket connected to specific host.";


CSocketListener::usage =
"CSocketListener[assoc] returns a new listener object.";


Begin["`Private`"];


Options[CSocketOpen] = {
    "NonBlocking" :> False, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


CSocketOpen[host_String: "localhost", port_Integer, OptionsPattern[]] := 
With[{
    socketId = socketOpen[
        host, 
        ToString[port], 
        If[OptionValue["NonBlocking"], 1, 0], 
        If[OptionValue["KeepAlive"], 1, 0],
        If[OptionValue["NoDelay"], 1, 0], 
        OptionValue["SendBufferSize"],
        OptionValue["RecvBufferSize"]
    ]
}, 
    CSocketObject[socketId]
];


CSocketOpen[host_String: "localhost", port_Integer, protocol: "TCP" | "UDP": "TCP", OptionsPattern[]] := 
Module[{
    family = $socketConstants["AF_INET"],
    socktype = If[protocol === "TCP", $socketConstants["SOCK_STREAM"], $socketConstants["SOCK_DGRAM"]],
    protocolNum = If[protocol === "TCP", $socketConstants["IPPROTO_TCP"], $socketConstants["IPPROTO_UDP"]]
},
    With[{
        addr = socketAddressCreate[host, port, family, socktype, protocolNum], 
        sock = socketCreate[family, socktype, protocolNum]
    }, 
        socketBind[sock, addr];

        If[protocol === "TCP", 
            socketListen[sock, $socketConstants["SOMAXCONN"]]
        ]; 

        CSocketObject[sock]
    ]
];


CSocketObject /: Close[CSocketObject[socketId_Integer]] := 
With[{result = socketClose[socketId]}, 
    (*Returns success or not*)
    result === 0
];


CSocketSelect[sockets: {__CSocketObject}, timeout: _?NumericQ: 1] := 
With[{socketIds = sockets[[All, 1]], tv = Round[timeout * 1000000]}, 
    Map[CSocketObject] @ 
    socketSelect[socketIds, Length[socketIds], tv]
];


CSocketObject /: SocketReadyQ[socket_CSocketObject, timeout: _?NumericQ: 0] := 
CSocketsSelect[{socket}, timeout] === {socket};


CSocketsRecv[CSocketObject[socketId_Integer], bufferSize_Integer: 8192] := 
socketRecv[socketId, bufferSize]; 


CSocketObject /: SocketReadMessage[socket_CSocketObject, bufferSize_Integer: 8192] := 
CSocketsRecv[socket, bufferSize];


Options[CSocketConnect] = {
    "NonBlocking" :> False, 
    "KeepAlive" :> True, 
    "NoDelay" :> True,
    "SendBufferSize" :> 256 * 1024,
    "RecvBufferSize" :> 256 * 1024
};


CSocketConnect[host_String: "localhost", port_Integer, OptionsPattern[]] :=
With[{
    socketId = socketConnect[
        host, 
        ToString[port], 
        If[OptionValue["NonBlocking"], 1, 0],
        If[OptionValue["KeepAlive"], 1, 0],
        If[OptionValue["NoDelay"], 1, 0],
        OptionValue["SendBufferSize"],
        OptionValue["RecvBufferSize"]
    ]
}, 
    CSocketObject[socketId]
];


CSocketObject /: WriteString[CSocketObject[socketId_Integer], data_String, len_: 1024 * 1024] :=
Do[
    socketSendString[socketId, #, StringLength[#]]& @ StringTake[data, {i, UpTo[i + len - 1]}], 
    {i, 1, StringLength[data], len}
];


CSocketObject /: BinaryWrite[CSocketObject[socketId_Integer], data_ByteArray, len_: 1024 * 1024] :=
Do[
    socketSend[socketId, #, Length[#]]& @ data[[i ;; UpTo[i + len - 1]]], 
    {i, 1, Length[data], len}
];


CSocketObject /: SocketReadyQ[socket_CSocketObject, timeout: _?NumericQ: 0] :=
CSocketSelect[{socket}, timeout] === {socket};


CSocketObject /: SocketListen[CSocketObject[socketId_Integer], handler_, OptionsPattern[{
    "ClientsCapacity" -> 1024,
    "BufferSize" -> 65536,
    "SelectTimeout" -> 1, 
    "Encoding" -> "UTF-8"
}]] := 
With[{
    serverPtr = serverCreate[
        socketId, 
        OptionValue["ClientsCapacity"], 
        OptionValue["BufferSize"], 
        Round[OptionValue["SelectTimeout"] * 1000000]
    ], 
    encoding = OptionValue["Encoding"]
}, 
    With[{
        task = Internal`CreateAsynchronousTask[
            serverListen, 
            {serverPtr}, 
            handler[toEvent[encoding][##]]&
        ]
    }, 
        (*Returns listener*)
        CSocketListener[<|
            "Task" -> task, 
            "Server" -> serverPtr, 
            "Socket" -> CSocketObject[socketId], 
            "Handler" -> handler,
            "ClientsCapacity" -> OptionValue["ClientsCapacity"], 
            "BufferSize" -> OptionValue["BufferSize"], 
            "SelectTimeout" -> OptionValue["SelectTimeout"]
        |>]
    ]
];


CSocketListener /: Close[CSocketListener[assoc_?AssociationQ]] := 
RemoveAsynchronousTask[assoc["Task"]];


toEvent[encoding_][task_, event_, {serv_, sock_, data_}] := 
With[{byteArray = ByteArray[data], time = Now}, <|
    "Event" :> event, 
    "TimeStamp" :> time, 
    "SourceSocket" :> CSocketObject[sock],
    "Socket" :> CSocketObject[serv],
    "Data" :> ByteArrayToString[byteArray, encoding],
    "DataBytes" :> Normal[byteArray],
    "DataByteArray" :> byteArray, 
    "MultiPartComplete" :> True
|>];


toEvent[encoding_][task_, event_, {serv_, sock_}] := 
With[{time = Now}, <|
    "Event" :> event, 
    "TimeStamp" :> time, 
    "SourceSocket" :> CSocketObject[sock],
    "Socket" :> CSocketObject[serv]
|>];


$directory = 
DirectoryName[$InputFileName, 2]; 


$socketIcon = 
Import[FileNameJoin[{$directory, "Images", "SocketIcon.png"}]]; 


$ListenreIcon = 
Import[FileNameJoin[{$directory, "Images", "ListenerIcon.png"}]]; 


$libraryLinkVersion = 
LibraryVersionInformation[FindLibrary["demo"]]["WolframLibraryVersion"]; 


$libFile = FileNameJoin[{
    $directory, 
    "LibraryResources", 
    $SystemID <> "-v" <> ToString[$libraryLinkVersion], 
    "csockets." <> Internal`DynamicLibraryExtension[]
}]; 


If[!FileExistsQ[$libFile], 
    Get[FileNameJoin[{$directory, "Build.wls"}]]
]; 


CSocketObject /: MakeBoxes[socket: CSocketObject[socketId_Integer], form: StandardForm | OutputForm] := 
BoxForm`ArrangeSummaryBox[
    CSocketObject, 
    socket, 
    None, 
    {{socketId}}, 
    {}, 
    form, 
    "Interpretable" -> Automatic
];


(*socketAddressCreate["host", "port", family, socktype, protocol] -> addressPtr*)
socketAddressCreate = 
LibraryFunctionLoad[$libFile, "socketAddressCreate", {String, String, Integer, Integer, Integer}, Integer];


(*socketAddressRemove[addressPtr] -> successStatus*)
socketAddressRemove = 
LibraryFunctionLoad[$libFile, "socketAddressRemove", {Integer}, Integer];


(*socketBufferCreate[bufferSize] -> bufferPtr*)
socketBufferCreate = 
LibraryFunctionLoad[$libFile, "socketBufferCreate", {Integer}, Integer];


(*socketBufferRemove[bufferPtr] -> successStatus*)
socketBufferRemove = 
LibraryFunctionLoad[$libFile, "socketBufferRemove", {Integer}, Integer];


(*socketCreate[family, type, protocol] -> socketId*)
socketCreate = 
LibraryFunctionLoad[$libFile, "socketCreate", {Integer, Integer, Integer}, Integer];


(*socketClose[socketId] -> successStatus*)
socketClose = 
LibraryFunctionLoad[$libFile, "socketClose", {Integer}, Integer]; 


(*socketBind[socketId, addressPtr] -> successStatus*)
socketBind = 
LibraryFunctionLoad[$libFile, "socketBind", {Integer, Integer}, Integer];


(*socketSetOpt[socketId, level, optname, optvalue] -> successStatus*)
socketSetOpt =
LibraryFunctionLoad[$libFile, "socketSetOpt", {Integer, Integer, Integer, Integer}, Integer];


(*socketGetOpt[socketId, level, optname] -> optvalue*)
socketGetOpt =
LibraryFunctionLoad[$libFile, "socketGetOpt", {Integer, Integer, Integer}, Integer];


(*socketBlockingMode[socketId, level, optname] -> successStatus*)
socketBlockingMode =
LibraryFunctionLoad[$libFile, "socketBlockingMode", {Integer, Integer}, Integer];


(*socketConnect[socketId, addressPtr] -> successStatus*)
socketConnect =
LibraryFunctionLoad[$libFile, "socketConnect", {Integer, Integer}, Integer];


(*socketListen[socketId] -> successStatus*)
socketListen =
LibraryFunctionLoad[$libFile, "socketListen", {Integer, Integer}, Integer];


(*socketSelect[{listenSocket, client1, ..}, length, timeout] -> {client1, ..}*)
socketSelect = 
LibraryFunctionLoad[$libFile, "socketSelect", {{Integer, 1}, Integer, Integer}, {Integer, 1}];


(*socketListCreate[interruptSocketId, length] -> socketListPtr*)
socketListCreate =
LibraryFunctionLoad[$libFile, "socketListCreate", {Integer, Integer}, Integer];


(*socketListSet[socketListPtr, sockets, length] -> successStatus*)
socketListSet =
LibraryFunctionLoad[$libFile, "socketListSet", {Integer, {Integer, 1}, Integer}, Integer];


(*socketSelectTaskCreate[socketListPtr] -> taskId*)
socketSelectTaskCreate =
LibraryFunctionLoad[$libFile, "socketSelectTaskCreate", {Integer}, Integer];


(*socketCheck[sockets, length]: aliveSockets*)
socketCheck =
LibraryFunctionLoad[$libFile, "socketCheck", {{Integer, 1}, Integer}, {Integer, 1}]; 


(*socketAccept[listenSocketId]: clientSocketId*)
socketAccept =
LibraryFunctionLoad[$libFile, "socketAccept", {Integer}, Integer]; 


(*socketRecv[clientSocketId, bufferSize]: byteArray*)
socketRecv =
LibraryFunctionLoad[$libFile, "socketRecv", {Integer, Integer, Integer}, "ByteArray"];


(*socketSend[socketId, data, length]: length*)
socketSend =
LibraryFunctionLoad[$libFile, "socketSend", {Integer, {"ByteArray", "Shared"}, Integer}, Integer]; 


(*socketSend[socketId, data, length]: length*)
socketSendString =
LibraryFunctionLoad[$libFile, "socketSendString", {Integer, String, Integer}, Integer];


(*serverCreate[listenSocket, clientsCapacity, bufferSize, selectTimeout]: serverPtr*)
serverCreate =
LibraryFunctionLoad[$libFile, "serverCreate", {Integer, Integer, Integer, Integer}, Integer];


(*serverListen[serverPtr]: taskId*)
serverListen =
LibraryFunctionLoad[$libFile, "serverListen", {Integer}, Integer];


(* Cross-platform socket constants association *)
(* Format: "CONST_NAME" -> value  (16^^ for hex, decimal for raw numbers) *)
(* Verified against POSIX/Windows headers *)

$socketConstants = <|
    (* Protocol levels *)
    "IPPROTO_IP"   :> 0,               (* IPv4 protocol - standard value *)
    "IPPROTO_TCP"  :> 6,               (* TCP protocol - common value *)
    "IPPROTO_UDP"  :> 17,              (* UDP protocol - standard value *)
    "IPPROTO_IPV6" :> 16^^0029,        (* IPv6 protocol - POSIX hex value *)
    "SOL_SOCKET"   :> 16^^FFFF,        (* Socket-level options - standard hex *)

    (* Socket options (SOL_SOCKET level) *)
    "SO_KEEPALIVE" :> 16^^0008,        (* Enable keep-alive packets *)
    "SO_RCVBUF"    :> 16^^1002,        (* Receive buffer size *)
    "SO_SNDBUF"    :> 16^^1001,        (* Send buffer size *)
    "SO_REUSEADDR" :> 16^^0002,        (* Allow address reuse - POSIX hex *)
    "SO_EXCLUSIVEADDRUSE" :> -5,       (* Windows-specific address protection *)
    "SO_LINGER"    :> 16^^0080,        (* Linger on close *)
    "SO_BROADCAST" :> 16^^0020,        (* Permit broadcast *)
    "SO_ERROR"     :> 16^^1007,        (* Get error status *)
    "SOMAXCONN"    :> 16^^7FFFFFFF,    (* Max available sockets for listen *)
    "SO_TYPE"      :> 16^^1008,        (* Get socket type *)
    "SO_ACCEPTCONN" :> 16^^0002,       (* Check if socket is listening *)

    (* TCP-specific options *)
    "TCP_NODELAY"  :> 16^^0001,        (* Disable Nagle algorithm *)
    "TCP_KEEPIDLE" :> 16^^0004,        (* Start keepalives after idle period *)
    "TCP_KEEPINTVL":> 16^^0005,        (* Interval between keepalives *)
    "TCP_KEEPCNT"  :> 16^^0006,        (* Number of keepalives before drop *)

    (* IP-level options *)
    "IP_TTL"       :> 16^^0004,        (* Time-To-Live for packets *)
    "IP_TOS"       :> 16^^0001,        (* Type Of Service *)
    "IP_MTU_DISCOVER" :> 16^^000A,     (* Path MTU discovery *)

    (* IPv6-specific options *)
    "IPV6_V6ONLY"  :> 16^^001A,        (* Restrict to IPv6 only *)

    (* Address families *)
    "AF_INET"      :> 16^^0002,        (* IPv4 address family *)
    "AF_INET6"     :> 16^^000A,        (* IPv6 address family *)
    "SOCK_STREAM"  :> 16^^0001,        (* Stream socket (TCP) *)
    "SOCK_DGRAM"   :> 16^^0002         (* Datagram socket (UDP) *)
|>;


(* Platform-specific overrides *)
(* Uncomment if targeting specific OS: *)
If[$OperatingSystem === "Windows",
    (* Windows uses different values for some constants *)
    $socketConstants["IPPROTO_IPV6"] := 41;
    $socketConstants["SO_REUSEADDR"] := 4;
    $socketConstants["IPV6_V6ONLY"] := 27;
    $socketConstants["SO_TYPE"] := 3;
];


(* ================================================ *)
(*  Protocol levels for getsockopt / setsockopt     *)
(*  ($socketOptLevels - only "level", not optname)  *)
(* ================================================ *)

$socketOptLevels = <|
    "SOL_SOCKET"    :> 1,           (* Socket-level options - POSIX default *)
    "IPPROTO_IP"    :> 0,           (* IPv4 protocol level *)
    "IPPROTO_TCP"   :> 6,           (* TCP protocol level *)
    "IPPROTO_UDP"   :> 17,          (* UDP protocol level *)
    "IPPROTO_IPV6"  :> 16^^0029     (* IPv6 protocol level - POSIX hex *)
|>;


(* Platform-specific overrides *)
If[$OperatingSystem === "Windows",
    (* Windows uses different values for some levels *)
    $socketOptLevels["SOL_SOCKET"]   := 16^^FFFF;  (* Winsock socket level *)
    $socketOptLevels["IPPROTO_IPV6"] := 41;        (* Decimal 0x29 *)
];


(* ================================================= *)
(*  Socket / protocol option names (optname only)     *)
(*  Use together with $socketOptLevels for level arg  *)
(* ================================================= *)

$socketOptNames = <|
    (* -------- SOL_SOCKET level -------- *)
    "SO_KEEPALIVE"        :> 16^^0008,     (* Enable keep-alive packets            *)
    "SO_RCVBUF"           :> 16^^1002,     (* Receive buffer size (bytes)          *)
    "SO_SNDBUF"           :> 16^^1001,     (* Send buffer size (bytes)             *)
    "SO_REUSEADDR"        :> 16^^0002,     (* Allow local address reuse            *)
    "SO_EXCLUSIVEADDRUSE" :> -5,           (* Windows-specific: exclusive bind     *)
    "SO_LINGER"           :> 16^^0080,     (* Linger on close                      *)
    "SO_BROADCAST"        :> 16^^0020,     (* Permit datagram broadcasts           *)
    "SO_ERROR"            :> 16^^1007,     (* Get pending error status             *)
    "SOMAXCONN"           :> 16^^7FFFFFFF, (* Maximum backlog for listen()       *)
    "SO_TYPE"             :> 3,            (* Get socket type (STREAM/DGRAM/…)     *)
    "SO_ACCEPTCONN"       :> 16^^0002,     (* Non-zero if socket is in LISTEN      *)

    (* -------- IPPROTO_TCP level -------- *)
    "TCP_NODELAY"         :> 16^^0001,     (* Disable Nagle algorithm              *)
    "TCP_KEEPIDLE"        :> 16^^0004,     (* Idle time before keep-alives (s)     *)
    "TCP_KEEPINTVL"       :> 16^^0005,     (* Interval between keep-alives (s)     *)
    "TCP_KEEPCNT"         :> 16^^0006,     (* Keep-alive probe count before drop   *)

    (* -------- IPPROTO_IP level -------- *)
    "IP_TTL"              :> 16^^0004,     (* Default IPv4 TTL                     *)
    "IP_TOS"              :> 16^^0001,     (* IPv4 Type-of-Service / DSCP          *)
    "IP_MTU_DISCOVER"     :> 16^^000A,     (* Path-MTU discovery setting           *)

    (* -------- IPPROTO_IPV6 level -------- *)
    "IPV6_V6ONLY"         :> 16^^001A      (* Restrict socket to IPv6 only         *)
|>;

(* ---------- Platform-specific overrides ---------- *)
If[$OperatingSystem === "Windows",
    (* Winsock uses different numeric values for some options *)
    $socketOptNames["SO_REUSEADDR"] := 4;       (* 0x0004 on Windows *)
    $socketOptNames["SO_TYPE"]      := 16^1108; (* 0x1008 on Windows *)
];


$socketOptLevelMap = <|
    (* -------- SOL_SOCKET level -------- *)
    "SO_KEEPALIVE"         -> "SOL_SOCKET",
    "SO_RCVBUF"            -> "SOL_SOCKET",
    "SO_SNDBUF"            -> "SOL_SOCKET",
    "SO_REUSEADDR"         -> "SOL_SOCKET",
    "SO_EXCLUSIVEADDRUSE"  -> "SOL_SOCKET",   (* Windows-only *)
    "SO_LINGER"            -> "SOL_SOCKET",
    "SO_BROADCAST"         -> "SOL_SOCKET",
    "SO_ERROR"             -> "SOL_SOCKET",
    "SOMAXCONN"            -> "SOL_SOCKET",
    "SO_TYPE"              -> "SOL_SOCKET",
    "SO_ACCEPTCONN"        -> "SOL_SOCKET",

    (* -------- IPPROTO_TCP level -------- *)
    "TCP_NODELAY"          -> "IPPROTO_TCP",
    "TCP_KEEPIDLE"         -> "IPPROTO_TCP",
    "TCP_KEEPINTVL"        -> "IPPROTO_TCP",
    "TCP_KEEPCNT"          -> "IPPROTO_TCP",

    (* -------- IPPROTO_IP (IPv4) level -------- *)
    "IP_TTL"               -> "IPPROTO_IP",
    "IP_TOS"               -> "IPPROTO_IP",
    "IP_MTU_DISCOVER"      -> "IPPROTO_IP",

    (* -------- IPPROTO_IPV6 level -------- *)
    "IPV6_V6ONLY"          -> "IPPROTO_IPV6"
|>;


End[];


EndPackage[];