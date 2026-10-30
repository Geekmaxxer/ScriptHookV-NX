param(
    [Parameter(Mandatory=$true)][string]$SdRoot,
    [ValidateSet('Check','Recover')][string]$Action = 'Check',
    [switch]$Json
)
$ErrorActionPreference = 'Stop'
if ($Json) { [Console]::OutputEncoding = New-Object Text.UTF8Encoding $false }

# Observed build-9b485eb8 DLC path coverage. These are not game assets or a
# universal list of optional DLCs. Only missing paths may receive blank stubs.
$title = '0100b00b51230000'
$required = @('mpbiker','mpchristmas2','mpheist','mppatchesng',
              'patchday1ng','patchday2bng','patchday2ng','patchday3ng')
$optional = @(
    'mpassault/dlc.rpf','mpbattle/dlc.rpf','mpbattle/dlc1.rpf',
    'mpchristmas2017/dlc.rpf','mpchristmas2018/dlc.rpf','mpexecutive/dlc.rpf',
    'mpgunrunning/dlc.rpf','mpheist3/dlc.rpf','mpheist4/dlc.rpf',
    'mpheist4/dlc1.rpf','mpheist4/dlc2.rpf','mplowrider/dlc.rpf',
    'mplowrider2/dlc.rpf','mpluxe/dlc.rpf','mpluxe2/dlc.rpf',
    'mpsecurity/dlc.rpf','mpsecurity/dlc1.rpf'
)
$stubHash = '076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560'

function Assert-RegularPath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    for ($entry = $full; $entry; $entry = [IO.Path]::GetDirectoryName($entry)) {
        if (Test-Path -LiteralPath $entry) {
            if ((Get-Item -LiteralPath $entry -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Game-layout paths cannot contain links or junctions: $entry"
            }
        }
    }
}
function Write-NewFile([string]$Path, [byte[]]$Bytes) {
    Assert-RegularPath $Path
    $file = [IO.File]::Open($Path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $file.Write($Bytes, 0, $Bytes.Length); $file.Flush($true) } finally { $file.Dispose() }
}
function Check-Layout([string]$Card) {
    Assert-RegularPath $Card
    if (-not (Test-Path -LiteralPath $Card -PathType Container)) { throw 'Select an existing SD root.' }
    $romfs = Join-Path $Card "atmosphere/contents/$title/romfs"
    Assert-RegularPath $romfs
    if (-not (Test-Path -LiteralPath $romfs -PathType Container)) {
        throw 'GTA loose RomFS was not found. Select the SD root, not the romfs or DLC directory. Full-data NSP layouts require their own package audit.'
    }
    $packs = Join-Path $romfs 'update/switch/dlcpacks'
    $findings = @(); $missing = @(); $placeholders = @(); $coreProblems = @(); $invalid = @()
    foreach ($pack in $required) {
        $relative = "update/switch/dlcpacks/$pack/dlc.rpf"; $path = Join-Path $romfs $relative
        Assert-RegularPath $path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -le 512) {
            $coreProblems += $relative
            $findings += "Retained base archive missing or too small: $relative. Restore the original archive; it cannot be replaced with a placeholder."
        }
    }
    foreach ($relative in $optional) {
        $path = Join-Path $packs $relative; Assert-RegularPath $path
        if (-not (Test-Path -LiteralPath $path)) {
            $missing += "update/switch/dlcpacks/$relative"
        } elseif (-not (Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -lt 512) {
            $invalid += "update/switch/dlcpacks/$relative"
            $findings += "Existing path is a directory or truncated file; preserve and inspect it: update/switch/dlcpacks/$relative"
        } elseif ((Get-Item -LiteralPath $path).Length -eq 512 -and (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $stubHash) {
            $placeholders += "update/switch/dlcpacks/$relative"
        }
    }
    if ($missing.Count) {
        $findings += 'Known optional DLC paths are absent. Removing loose overrides can change the Atmosphere RomFS merge and has caused ams.mitm startup aborts in this profile.'
        $findings += 'For build-9b485eb8, recover missing path coverage or restore the matching original archives. Path recovery does not restore playable DLC content or update dlclist.xml.'
    }
    $findings += 'This checks 25 known DLC paths. It does not verify the installed NSP/build, game archives, saves, SD health, or gameplay. A different panic needs separate diagnosis.'
    return [pscustomobject]@{
        Profile = 'build-9b485eb8-dlc-path-coverage'; TitleId = $title; SdRoot = $Card; Romfs = $romfs
        MissingOptional = @($missing); Placeholders = @($placeholders); CoreProblems = @($coreProblems)
        InvalidPaths = @($invalid); Findings = @($findings)
        RecoveryAllowed = ($missing.Count -gt 0 -and $coreProblems.Count -eq 0 -and $invalid.Count -eq 0)
        Record = $null
    }
}

try {
    $card = [IO.Path]::GetFullPath($SdRoot)
    $report = Check-Layout $card
    if ($Action -eq 'Recover' -and $report.MissingOptional.Count) {
        if (-not $report.RecoveryAllowed) { throw 'Recovery stopped: restore the missing base archives or inspect existing invalid paths first. No DLC files were written.' }
        $id = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
        $history = Join-Path $card "switch/gtav-scripthook/game-layout-records/$id.json"
        Assert-RegularPath $history
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($history))
        $record = [pscustomobject]@{
            Format = 1; Profile = $report.Profile; TitleId = $title; CreatedUtc = [DateTime]::UtcNow.ToString('o')
            State = 'planned'; Files = @($report.MissingOptional); BytesPerFile = 512; Sha256 = $stubHash
        }
        Write-NewFile $history ([Text.Encoding]::UTF8.GetBytes(($record | ConvertTo-Json -Depth 4)))
        foreach ($relative in $record.Files) {
            $target = Join-Path $report.Romfs $relative
            Assert-RegularPath $target
            if (Test-Path -LiteralPath $target) { throw "A destination appeared after checking; it was preserved: $target. Keep the recovery record for review." }
            [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target))
            $temporary = $target + '.layout-' + $id + '.tmp'
            try {
                Write-NewFile $temporary (New-Object byte[] 512)
                if ((Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash -ne $stubHash) { throw 'Placeholder write verification failed.' }
                # File.Move without replacement also protects a destination that
                # appears after preflight. An interrupted run is safe to repeat.
                Assert-RegularPath $target
                [IO.File]::Move($temporary, $target)
            } finally {
                if (Test-Path -LiteralPath $temporary -PathType Leaf) { [IO.File]::Delete($temporary) }
            }
        }
        $report = Check-Layout $card
        if ($report.MissingOptional.Count) { throw "Recovery incomplete. Inspect record: $history" }
        $record.State = 'completed'
        [IO.File]::WriteAllText($history, ($record | ConvertTo-Json -Depth 4), (New-Object Text.UTF8Encoding $false))
        $report.Record = $history
        $report.Findings = @('Missing path coverage restored. Keep the placeholders until replaced by matching real DLC files. Safely eject and test one launch.') + $report.Findings
    }
    if ($Json) { $report | ConvertTo-Json -Depth 5 } else {
        Write-Output "GTA game layout - $($report.Profile)"
        Write-Output "Missing optional paths: $($report.MissingOptional.Count); blank placeholders: $($report.Placeholders.Count); base problems: $($report.CoreProblems.Count); invalid paths: $($report.InvalidPaths.Count)"
        foreach ($path in $report.MissingOptional) { Write-Output "Missing: $path" }
        foreach ($finding in $report.Findings) { Write-Output $finding }
        if ($report.Record) { Write-Output "Recovery record: $($report.Record)" }
        if ($report.RecoveryAllowed) { Write-Output 'Read GAME-STARTUP.md before using -Action Recover. Check mode makes no changes.' }
    }
    if ($report.CoreProblems.Count -or $report.InvalidPaths.Count) { exit 1 }
    if ($report.MissingOptional.Count) { exit 2 }
    exit 0
} catch { Write-Error $_; exit 1 }
