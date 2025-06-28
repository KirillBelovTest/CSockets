(* ::Package:: *)

BeginPackage["KirillBelov`CSockets`"]; 


Internal`CreateAsynchronousTask::usage = 
"CreateAsynchronousTask[libFunc, {args}, handlerFunc] creates async task.";


EndPackage[]; 


Get["KirillBelov`CSockets`TCP`"]; 


Get["KirillBelov`CSockets`UDP`"]; 


Get["KirillBelov`CSockets`Handler`"]; 
