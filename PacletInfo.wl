(* ::Package:: *)

PacletObject[
  <|
    "Name" -> "KirillBelov/CSocketListener",
    "Description" -> "Socket Listener powered by pure C & UV (a fork)",
    "Creator" -> "Kirill Belov",
    "License" -> "MIT",

    "Version" -> "2.0.0",
    "WolframVersion" -> "12+",
    "PrimaryContext" -> "KirillBelov`CSocketListener`",
    "Extensions" -> {
      {
        "Kernel",
        "Root" -> "Kernel",
        "Context" -> {"KirillBelov`CSocketListener`"},
        "Symbols" -> {
          "KirillBelov`CSocketListener`CSocket",
          "KirillBelov`CSocketListener`CSocketListen",
          "KirillBelov`CSocketListener`CSocketListener"
        }
      },
      {"Documentation", "Language" -> "English"},
      {"LibraryLink", "Root" -> "LibraryResources"},
      {
        "Asset",
        "Assets" -> {
          {"License", "./LICENSE"},
          {"ReadMe", "./README.md"},
          {"Source", "./Source"},
          {"Scripts", "./Scripts"}
        }
      }
    }
  |>
]
