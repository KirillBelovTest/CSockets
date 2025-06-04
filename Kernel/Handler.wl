(* :Package: *)

BeginPackage["KirillBelov`CSockets`Handler`"]; 


CSocketHandler::usage = 
"CSocketHandler[] mutable handler object."; 


Begin["`Private`"]; 


Options[CSocketHandler] = {
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


With[{store = Language`NewExpressionStore["CSocketHandler"]}, 

    CSocketHandler[OptionsPattern[]] := 
    With[{handler = CSocketHandler[Null]}, 
        Map[store["put"[handler, #, OptionValue[#]]]&] @ Keys[Options[CSocketHandler]]; 

        handler
    ]; 

    (handler_CSocketHandler)[key_] := store["get"[handler, key]]; 

    CSocketHandler /: Set[(handler_CSocketHandler)[key_], value_] := (store["put"[handler, key, value]]; value); 

    CSocketHandler /: Set[(handler_CSocketHandler)[prop_, key_], value_] := (store["put"[handler, prop, Append[handler[prop], key -> value]]]; value); 
]; 


Unprotect[Set]; 


(*TODO: replace to MutationHandler*)
Set[(handler_?(Head[#] === CSocketHandler&))[keys__], value_] := 
With[{$handler$ = handler}, $handler$[keys] = value]; 


Protect[Set]; 


Format[handler_CSocketHandler, InputForm] := 
SequenceForm[CSocketHandler] @ Map[Function[# -> handler[#]]] @ Keys @ Options[CSocketHandler]; 


CSocketHandler /: MakeBoxes[handler: CSocketHandler[Null], form: (StandardForm | TraditionalForm)] := 
Module[{above, below}, 
    {above, below} = TakeDrop[Map[# -> handler[#]&] @ Keys @ Options[CSocketHandler], 2]; 

    BoxForm`ArrangeSummaryBox[CSocketHandler, handler, Null, above, below, form, "Interpretable" -> Automatic]
]; 


(handler_CSocketHandler)[packet_Association] := 
Module[{extendedPacket, result, extraPacket, extraPacketDataLength}, 
    Which[
        packet["Event"] === "Received", 

        extendedPacket = getExtendedPacket[handler, packet]; (*Association[]*)

        If[extendedPacket["Completed"], 
            With[{message = getMessage[handler, extendedPacket]}, 
                extendedPacket["DataByteArray"] := message; (*ByteArray[]*)
                extendedPacket["Data"] := ByteArrayToString[message];
                extendedPacket["DataBytes"] := Normal[message];
                With[{content = handler["Deserializer"][message]}, 
                    extendedPacket["Message"] := content; 
                ]; 
            ]; 

            result = handler["Serializer"] @ invokeHandler[handler, extendedPacket]; (*ByteArray[] | _String | Null*)

            sendResponse[handler, packet, result];

            If[extendedPacket["StoredLength"] > extendedPacket["ExpectedLength"], 
                extraPacket = packet; 
                extraPacketDataLength = extendedPacket["StoredLength"] - extendedPacket["ExpectedLength"]; 
                extraPacket["DataByteArray"] = packet["DataByteArray"][[-extraPacketDataLength ;; ]]; 
                clearBuffer[handler, packet]; 
                handler[extraPacket], 
            (*Else*)
                clearBuffer[handler, extendedPacket]
            ]; 
            
            Return[result], 
        (*Else*)
            savePacketToBuffer[handler, extendedPacket]
        ]; , 

        packet["Event"] === "Accepted", 
            handler["AcceptHandler"][packet], 

        packet["Event"] === "Closed", 
            handler["CloseHandler"][packet]
    ]; 
]; 


getExtendedPacket[handler_, packet_] := 
With[{uuid = packet["SourceSocket"][[1]]}, 
    Module[{
        dataLength, 
        last, 
        expectedLength, 
        storedLength, 
        completed, 
        buffer
    }, 
        dataLength = Length[packet["DataByteArray"]]; 
        buffer = handler["Buffer"]["Lookup", uuid]; (*DataStructure[DynamicArray]*)

        If[!MissingQ[buffer] && buffer["Length"] > 0, 
            last = buffer["Part", -1]; (*Association[]*) 
            expectedLength = last["ExpectedLength"]; 
            storedLength = last["StoredLength"];, 

        (*Else*)
            expectedLength = conditionApply[handler["Accumulator"], handler["DefaultAccumulator"]][packet]; 
            storedLength = 0; 
        ]; 

        completed = storedLength + dataLength >= expectedLength; 

        (*Return: Association[]*)
        Join[packet, <|
            "Completed" -> completed, 
            "ExpectedLength" -> expectedLength, 
            "StoredLength" -> storedLength + dataLength, 
            "DataLength" -> dataLength
        |>]
    ]
]; 


getMessage[handler_, extendedPacket_] := 
With[{
    buffer = handler["Buffer"]["Lookup", extendedPacket["SourceSocket"][[1]]], 
    expectedLength = extendedPacket["ExpectedLength"]
}, 
    If[!MissingQ[buffer] && buffer["Length"] > 0, 

        (*Return: _ByteArray*)
        Part[#, 1 ;; expectedLength]& @ 
        Apply[Join] @ 
        Append[extendedPacket["DataByteArray"]] @ 
        buffer["Elements"][[All, "DataByteArray"]], 

    (*Else*)

        (*Return: _ByteArray*)
        extendedPacket["DataByteArray"][[1 ;; expectedLength]]
    ]
]; 


invokeHandler[handler_, packet_] := 
Module[{messageHandler, defaultMessageHandler}, 
    messageHandler = handler["Handler"]; 
    defaultMessageHandler = handler["DefaultHandler"]; 

    (*Return: ByteArray[] | _String | Null*)
    conditionApply[messageHandler, defaultMessageHandler][packet]
]; 


CSocketHandler::cntsnd = 
"Can't send result to the client\n `1`"; 


sendResponse[handler_, packet_, result: _ByteArray | _String | Null] := 
With[{client = packet["SourceSocket"]}, 
    Switch[result, 
        _String, 
            WriteString[client, result], 
        
        _ByteArray, 
            BinaryWrite[client, result], 

        Null, 
            Null
    ]
]; 


sendResponse[_, _, result_] := 
Message[CSocketHandler::cntsnd, result]; 


savePacketToBuffer[handler_, extendedPacket_] := 
With[{
    buffer = handler["Buffer"]["Lookup", extendedPacket["SourceSocket"][[1]]], 
    uuid = extendedPacket["SourceSocket"][[1]]
}, 
    If[!MissingQ[buffer], 
        buffer["Append", extendedPacket], 
        handler["Buffer"]["Insert", uuid -> CreateDataStructure["DynamicArray", {extendedPacket}]]
    ]
]; 


clearBuffer[handler_, packet_] := 
With[{buffer = handler["Buffer"]["Lookup", packet["SourceSocket"][[1]]]}, 
    buffer["DropAll"]; 
]; 


conditionApply[conditionAndFunctions_: <||>, defalut_: Function[Null], ___] := 
Function[
    With[{selected = SelectFirst[conditionAndFunctions, Function[f, First[f][##]], {defalut}]}, 
        selected[[-1]][##]
    ]
]; 


End[(*`Private`*)]; 


EndPackage[(*KirillBelov`CSockets`Handler`*)]; 