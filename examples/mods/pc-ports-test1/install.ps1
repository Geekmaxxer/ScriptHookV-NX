param([Parameter(Position=0)][string]$SdRoot)
$ErrorActionPreference = 'Stop'

# Run from an extracted test package. Update existing startup filenames in place
# so an older Auto/Disabled copy cannot silently win over a new Manual copy.
if (-not $SdRoot) { $SdRoot = Read-Host 'SD card root (for example D:\)' }
$card = (Resolve-Path -LiteralPath $SdRoot).ProviderPath
if (-not (Test-Path -LiteralPath (Join-Path $card 'switch') -PathType Container)) {
    throw 'Choose the SD card root containing the switch folder.'
}
$destination = Join-Path $card 'switch\gtav-scripthook\plugins'
$manifest = Join-Path $PSScriptRoot 'ports.json'
if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
    throw 'Run this installer from the fully extracted test ZIP.'
}
$ports = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if (-not $ports) { throw 'The package contains no plugins.' }
# Validate the entire package before writing anything.
foreach ($port in $ports) {
    if ($port.id -notmatch '^[a-z0-9-]{1,31}$' -or $port.sha256 -notmatch '^[a-fA-F0-9]{64}$') {
        throw 'Invalid package manifest.'
    }
    $source = Join-Path $PSScriptRoot ('switch\gtav-scripthook\plugins\' + $port.id + '.nro.manual')
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $port.sha256) {
        throw ('Package checksum failed: ' + $port.id)
    }
}
[void](New-Item -ItemType Directory -Path $destination -Force)
& (Join-Path $PSScriptRoot 'Install-SD-Access.ps1') -SdRoot $card
foreach ($port in $ports) {
    $source = Join-Path $PSScriptRoot ('switch\gtav-scripthook\plugins\' + $port.id + '.nro.manual')
    $existing = @('.nro', '.nro.manual', '.nro.disabled' | ForEach-Object {
        $candidate = Join-Path $destination ($port.id + $_)
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { $candidate }
    })
    if (-not $existing.Count) { $existing = @(Join-Path $destination ($port.id + '.nro.manual')) }
    foreach ($target in $existing) {
        Copy-Item -LiteralPath $source -Destination $target -Force
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $port.sha256) {
            throw ('Installed checksum failed: ' + $target)
        }
        Write-Host ('Updated ' + [IO.Path]::GetFileName($target))
    }
}
Write-Host 'Done. Startup modes, settings, other mods and the runtime are preserved.'
