param(
    [string]$Minimum
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

$project = Select-String -Path "$root\CMakeLists.txt" -Pattern 'project\(GnomeparsingCompanion VERSION ([0-9.]+)'
$version = $project.Matches[0].Groups[1].Value
$release = "$root\build\release"

if (-not $Minimum -and (Test-Path "$release\version.json")) {
    $Minimum = (Get-Content "$release\version.json" -Raw | ConvertFrom-Json).minimum
}
if (-not $Minimum) { $Minimum = $version }

& cmake --build "$root\build\windows" --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

New-Item -ItemType Directory -Force $release | Out-Null
Copy-Item "$root\build\windows\Release\GnomeparsingCompanion.exe" $release -Force

$hash = (Get-FileHash "$release\GnomeparsingCompanion.exe" -Algorithm SHA256).Hash.ToLower()
$json = "{`n  `"latest`": `"$version`",`n  `"minimum`": `"$Minimum`",`n  `"sha256`": `"$hash`"`n}`n"
[IO.File]::WriteAllText("$release\version.json", $json, (New-Object Text.UTF8Encoding $false))

Write-Output "Upload the exe first, then version.json, from $release"
