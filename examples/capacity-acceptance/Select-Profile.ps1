param(
    [Parameter(Mandatory=$true)][string]$SdRoot,
    [Parameter(Mandatory=$true)][Alias('Profile')][ValidateSet('Defaults','Explicit','Automatic','Restore')][string]$TestProfile
)
$ErrorActionPreference = 'Stop'
$sdBase = [IO.Path]::GetFullPath($SdRoot)
$sdPrefix = $sdBase.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar

function Safe-Path([string]$Relative) {
    $absolute = [IO.Path]::GetFullPath((Join-Path $sdBase $Relative))
    if (-not $absolute.StartsWith($sdPrefix,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'The selected path is outside the supplied SD root.'
    }
    $walk = $absolute
    while ($walk) {
        $item = Get-Item -LiteralPath $walk -Force -ErrorAction SilentlyContinue
        if ($null -ne $item) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw 'Linked SD paths are not supported by this test helper.'
            }
        }
        if ($walk.TrimEnd('\','/') -eq $sdBase.TrimEnd('\','/')) { break }
        $parent = [IO.Directory]::GetParent($walk)
        if ($null -eq $parent) { break }
        $walk = $parent.FullName
    }
    return $absolute
}
function Digest([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw 'Expected a regular file.' }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Save-New([string]$Path, [byte[]]$Bytes) {
    $stream = [IO.File]::Open($Path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write)
    try { $stream.Write($Bytes,0,$Bytes.Length); $stream.Flush() } finally { $stream.Dispose() }
}

$manifestPath = Join-Path $PSScriptRoot 'profile-manifest.json'
if ((Get-Item -LiteralPath $manifestPath).Length -gt 16384) { throw 'Oversized profile manifest.' }
$manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($manifest.format -ne 1) { throw 'Unsupported profile manifest.' }
$paths = @{}
$knownHashes = @()
foreach ($name in @('Defaults','Explicit','Automatic')) {
    $path = Join-Path $PSScriptRoot ('profiles/' + $name.ToLowerInvariant() + '.ini')
    $expected = $manifest.profiles.$name
    if ($expected -notmatch '^[a-f0-9]{64}$' -or (Digest $path) -ne $expected) {
        throw 'A packaged profile is missing or has changed.'
    }
    $paths[$name] = $path
    $knownHashes += $expected
}
$config = Safe-Path 'switch/gtav-scripthook/runtime.ini'
$backupDir = Safe-Path 'switch/gtav-scripthook/settings/capacity-acceptance'
$backup = Safe-Path 'switch/gtav-scripthook/settings/capacity-acceptance/runtime.ini.original'
$journal = Safe-Path 'switch/gtav-scripthook/settings/capacity-acceptance/profile-backup.json'
$temporary = Safe-Path 'switch/gtav-scripthook/runtime.ini.capacity-next'
$currentHash = Digest $config
if ($TestProfile -ne 'Restore') {
    $module = Safe-Path 'atmosphere/contents/0100b00b51230000/exefs/subsdk5'
    $plugin = Safe-Path 'switch/gtav-scripthook/plugins/capacity-acceptance.nro.manual'
    if ($manifest.runtime_sha256 -notmatch '^[a-f0-9]{64}$' -or (Digest $module) -ne $manifest.runtime_sha256) {
        throw 'The installed runtime does not match the Test1 candidate.'
    }
    if ($manifest.plugin_sha256 -notmatch '^[a-f0-9]{64}$' -or (Digest $plugin) -ne $manifest.plugin_sha256) {
        throw 'Install the matching manual capacity probe before selecting a profile.'
    }
}
if (Test-Path -LiteralPath $journal) {
    if ((Get-Item -LiteralPath $journal).Length -gt 4096) { throw 'Oversized profile backup journal.' }
    $saved = Get-Content -LiteralPath $journal -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($saved.format -ne 1 -or $saved.original_exists -isnot [bool]) { throw 'Invalid profile backup journal.' }
    if ($saved.original_exists) {
        if ($saved.original_sha256 -notmatch '^[a-f0-9]{64}$' -or (Digest $backup) -ne $saved.original_sha256) {
            throw 'The original configuration backup is missing or has changed.'
        }
    } elseif (Test-Path -LiteralPath $backup) { throw 'Unexpected original configuration backup.' }
} else {
    if ($TestProfile -eq 'Restore') { throw 'No capacity-test configuration backup exists.' }
    if (Test-Path -LiteralPath $backup) { throw 'An unjournaled backup already exists; preserve it for inspection.' }
    if ($currentHash -and (Get-Item -LiteralPath $config).Length -gt 1048576) { throw 'Oversized original configuration.' }
    [void][IO.Directory]::CreateDirectory($backupDir)
    $saved = [pscustomobject]@{ format=1; original_exists=($null -ne $currentHash); original_sha256=$currentHash }
    if ($saved.original_exists) { Save-New $backup ([IO.File]::ReadAllBytes($config)) }
    Save-New $journal ([Text.UTF8Encoding]::new($false).GetBytes(($saved | ConvertTo-Json)))
}
$originalHash = if ($saved.original_exists) { $saved.original_sha256 } else { $null }
if ($currentHash -ne $originalHash -and $knownHashes -notcontains $currentHash) {
    throw 'The current runtime.ini has unrelated edits; it and the original backup were preserved.'
}
# Preserve the previous phase before a later launch overwrites its reports.
$previous = 'original'
foreach ($name in @('Defaults','Explicit','Automatic')) {
    if ($currentHash -eq $manifest.profiles.$name) { $previous = $name.ToLowerInvariant() }
}
$capture = Safe-Path ('switch/gtav-scripthook/settings/capacity-acceptance/phase-results/' +
    $previous + '-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
[void][IO.Directory]::CreateDirectory($capture)
$captures = @{}
$sources = @{
    'loader-report.txt' = 'switch/gtav-scripthook/loader-report.txt'
    'status.txt' = 'switch/gtav-scripthook/settings/capacity-acceptance/status.txt'
    'cleanup.txt' = 'switch/gtav-scripthook/settings/capacity-acceptance/cleanup.txt'
    'runtime.ini' = 'switch/gtav-scripthook/runtime.ini'
}
foreach ($index in 1..8) {
    $name = 'run-' + $index + '.txt'
    $sources[$name] = 'switch/gtav-scripthook/settings/capacity-acceptance/' + $name
}
foreach ($name in $sources.Keys) {
    $source = Safe-Path $sources[$name]
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        if ((Get-Item -LiteralPath $source).Length -gt 1048576) { throw 'Oversized phase report; preserve it for inspection.' }
        $target = Join-Path $capture $name
        Save-New $target ([IO.File]::ReadAllBytes($source))
        $captures[$name] = Digest $target
    }
}
$captureInfo = @{ format=1; previous_profile=$previous; configuration_sha256=$currentHash; files=$captures }
Save-New (Join-Path $capture 'capture.json') ([Text.UTF8Encoding]::new($false).GetBytes(($captureInfo | ConvertTo-Json -Depth 4)))
if ($TestProfile -eq 'Restore' -and -not $saved.original_exists) {
    if (Test-Path -LiteralPath $config) { Remove-Item -LiteralPath $config }
    Write-Output 'Original absent runtime.ini state restored. Restart GTA.'
    return
}
if (Test-Path -LiteralPath $temporary) { throw 'A previous temporary configuration exists; preserve it for inspection.' }
$source = if ($TestProfile -eq 'Restore') { $backup } else { $paths[$TestProfile] }
Save-New $temporary ([IO.File]::ReadAllBytes($source))
# Both paths are fixed children of the checked SD root; only one file is moved.
Move-Item -LiteralPath $temporary -Destination $config -Force
if ((Digest $config) -ne (Digest $source)) { throw 'Configuration readback differs; the original backup remains preserved.' }
Write-Output ($TestProfile + ' profile selected. The original configuration backup is retained. Restart GTA.')
