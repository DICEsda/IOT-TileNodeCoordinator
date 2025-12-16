# Render PlantUML diagrams to PNG using Docker
param(
  [string]$Input = "docs/diagrams",
  [string]$Output = "docs/Pics"
)

if (-not (Test-Path $Output)) {
  New-Item -ItemType Directory -Path $Output | Out-Null
}

$pwdPath = (Get-Location).Path

$diagrams = Get-ChildItem -Path $Input -Filter *.puml -File -Recurse
if ($diagrams.Count -eq 0) {
  Write-Host "No .puml files found under $Input" -ForegroundColor Yellow
  exit 0
}

try {
  docker version | Out-Null
} catch {
  Write-Error "Docker is not available. Please start Docker Desktop and retry."
  exit 1
}

foreach ($diag in $diagrams) {
  Write-Host "Rendering $($diag.FullName) -> $Output" -ForegroundColor Cyan
  $mount = "$pwdPath:/workspace"
  docker run --rm -v $mount -w /workspace plantuml/plantuml -tpng $diag.FullName -o $Output
}
