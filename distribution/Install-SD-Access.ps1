param([string]$SdRoot, [string]$OutputDirectory, [switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$relative = 'atmosphere\contents\0100b00b51230000\exefs\main.npdm'
$expected = 'db97307619f2dd536f06c38703721e6164470742a81522edfb273778937e650f'
function U32([byte[]]$Bytes, [long]$At) {
    if ($At -lt 0 -or $At + 4 -gt $Bytes.Length) { throw 'NPDM field out of bounds.' }
    return [BitConverter]::ToUInt32($Bytes, [int]$At)
}
function U64([byte[]]$Bytes, [long]$At) {
    if ($At -lt 0 -or $At + 8 -gt $Bytes.Length) { throw 'NPDM field out of bounds.' }
    return [BitConverter]::ToUInt64($Bytes, [int]$At)
}
function Inspect-Npdm([byte[]]$Bytes) {
    if ($Bytes.Length -lt 128 -or $Bytes.Length -gt 1048576 -or [Text.Encoding]::ASCII.GetString($Bytes,0,4) -ne 'META') { throw 'Invalid META NPDM.' }
    $aci=[long](U32 $Bytes 112); $aciSize=[long](U32 $Bytes 116)
    $acid=[long](U32 $Bytes 120); $acidSize=[long](U32 $Bytes 124)
    if ($aci -lt 128 -or $acid -lt 128 -or $aciSize -lt 64 -or $acidSize -lt 576 -or $aci+$aciSize -gt $Bytes.Length -or $acid+$acidSize -gt $Bytes.Length -or [Math]::Max($aci,$acid) -lt [Math]::Min($aci+$aciSize,$acid+$acidSize)) { throw 'Invalid NPDM sections.' }
    if ([Text.Encoding]::ASCII.GetString($Bytes,[int]$aci,4) -ne 'ACI0' -or [Text.Encoding]::ASCII.GetString($Bytes,[int]($acid+512),4) -ne 'ACID') { throw 'Invalid NPDM section magic.' }
    $title=U64 $Bytes ($aci+16)
    if ($title -ne [uint64]0x0100b00b51230000 -or (U64 $Bytes ($acid+528)) -gt $title -or (U64 $Bytes ($acid+536)) -lt $title) { throw 'NPDM is for another title.' }
    $offsets=@()
    foreach ($part in @(@($aci,$aciSize,64,32,28),@($acid,$acidSize,576,544,44))) {
        $base=$part[0]; $size=$part[1]; $header=$part[2]; $table=$base+$part[3]; $sections=@()
        for ($i=0; $i -lt 3; $i++) {
            $offset=[long](U32 $Bytes ($table+$i*8)); $length=[long](U32 $Bytes ($table+$i*8+4))
            if ($i -eq 0 -and $length -lt $part[4]) { throw 'Short filesystem permission section.' }
            if (-not $length) { continue }
            if ($offset -lt $header -or $offset+$length -gt $size) { throw 'Permission section out of bounds.' }
            foreach ($section in $sections) { if ([Math]::Max($offset,$section[0]) -lt [Math]::Min($offset+$length,$section[1])) { throw 'Overlapping permission sections.' } }
            $sections+=,@($offset,($offset+$length))
        }
        $fs=$base+(U32 $Bytes $table)
        if ((U32 $Bytes $fs) -ne 1) { throw 'Unsupported filesystem permission version.' }
        $offsets+=($fs+4)
    }
    return [pscustomobject]@{ Offsets=$offsets }
}
$bundled=Join-Path $PSScriptRoot $relative
if ((Get-FileHash -LiteralPath $bundled -Algorithm SHA256).Hash -ne $expected) { throw 'Bundled NPDM checksum failed.' }
[byte[]]$reference=[IO.File]::ReadAllBytes($bundled)
$referenceInfo=Inspect-Npdm $reference
foreach ($offset in $referenceInfo.Offsets) { if (-not ((U64 $reference $offset) -band [uint64]0x200000)) { throw 'Bundled SD permission is missing.' } }
if ($OutputDirectory) {
    # Build staging mode: no SD access and no modification of an existing file.
    $target=Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) $relative
    if (Test-Path -LiteralPath $target) {
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected) { throw 'Build output already has different NPDM metadata.' }
        Write-Host 'Existing staged SD-access NPDM verified.'; return
    }
    [void](New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force)
    Copy-Item -LiteralPath $bundled -Destination $target
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected) { throw 'Staged NPDM checksum failed.' }
    Write-Host 'Staged shared SD-access main.npdm.'
    return
}
if (-not $SdRoot) { $SdRoot=Read-Host 'SD card root (close GTA first; for example D:\)' }
$card=(Resolve-Path -LiteralPath $SdRoot).ProviderPath
if (-not (Test-Path -LiteralPath (Join-Path $card 'switch') -PathType Container)) { throw 'Select the SD root containing switch.' }
$target=Join-Path $card $relative
if ($CheckOnly -and -not (Test-Path -LiteralPath $target -PathType Leaf)) { throw 'No SD-access NPDM is installed in exefs.' }
if (Test-Path -LiteralPath $target -PathType Leaf) {
    $length=(Get-Item -LiteralPath $target).Length
    if ($length -lt 128 -or $length -gt 1048576) { throw 'Invalid NPDM file size.' }
}
[byte[]]$before=if (Test-Path -LiteralPath $target -PathType Leaf) { [IO.File]::ReadAllBytes($target) } else { $reference }
$info=Inspect-Npdm $before
[byte[]]$after=$before.Clone()
foreach ($offset in $info.Offsets) {
    [uint64]$flags=(U64 $before $offset) -bor [uint64]0x200000
    [BitConverter]::GetBytes($flags).CopyTo($after,[int]$offset)
}
$changed=$false
for ($i=0; $i -lt $before.Length; $i++) {
    if ($before[$i] -ne $after[$i]) {
        if (-not ($info.Offsets | Where-Object { $i -ge $_ -and $i -lt $_+8 })) { throw 'Non-permission byte changed.' }
        $changed=$true
    }
}
if ($CheckOnly) { if ($changed) { throw 'Existing NPDM needs explicit SD permission.' }; Write-Host 'SD-access permissions verified.'; return }
if ((Test-Path -LiteralPath $target) -and -not $changed) { Write-Host 'Existing NPDM already grants SD access; preserved.'; return }
if (Test-Path -LiteralPath $target) {
    $hash=(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
    $backup=Join-Path $card ('switch\gtav-scripthook\npdm-backups\main.'+$hash+'.npdm')
    [void](New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($backup)) -Force)
    if (-not (Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $target -Destination $backup }
    if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hash) { throw 'NPDM backup verification failed.' }
}
[void](New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force)
[IO.File]::WriteAllBytes($target,$after)
if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($target)) -ne [Convert]::ToBase64String($after)) { throw 'NPDM write verification failed.' }
Write-Host 'Installed SD-access main.npdm; all other metadata preserved.'
