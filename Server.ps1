wolframscript -f .\Build.wls

while ($true) {
    wolframscript -f Server.wls

    "Exit code == $LASTEXITCODE"
    
    Start-Sleep -Seconds 1
}