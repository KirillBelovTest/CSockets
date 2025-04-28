wolframscript -f .\Scripts\Build.wls

while ($true) {
    wolframscript -f TestServer.wls

    "Exit code == $LASTEXITCODE"
    
    Start-Sleep -Seconds 1
}