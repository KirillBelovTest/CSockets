wolframscript -f .\Build.wls

while ($true) {
    wolframscript -f App.wls

    "Exit code == $LASTEXITCODE"
    
    Start-Sleep -Seconds 1
}