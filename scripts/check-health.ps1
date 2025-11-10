param(
    [string]$Url = "http://localhost:8000/health",
    [int]$Retries = 20,
    [int]$DelaySeconds = 3
)

Write-Host "Checking backend health at $Url" -ForegroundColor Cyan
for ($i = 1; $i -le $Retries; $i++) {
    try {
        $resp = Invoke-WebRequest -UseBasicParsing -TimeoutSec 3 -Uri $Url
        if ($resp.StatusCode -eq 200) {
            Write-Host "Healthy (200)" -ForegroundColor Green
            exit 0
        }
    } catch {
        # ignore
    }
    Write-Host "Waiting ($i/$Retries)..." -ForegroundColor Yellow
    Start-Sleep -Seconds $DelaySeconds
}
Write-Error "Backend did not become healthy in time."
exit 1
