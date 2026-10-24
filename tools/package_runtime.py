"""Package only a runtime module, attribution and a short player guide."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
from nso import Image
from generate_bindings import DEFAULT_PROFILE
from sd_permissions import stage as stage_sd_access

ROOT = Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def package(module, output, acceptance=None, profile=DEFAULT_PROFILE):
    if output.exists(): raise FileExistsError('Use a fresh output directory')
    image = Image(module)
    if not all(s['hash_required'] for s in image.layout): raise ValueError('Runtime must include all three segment hashes')
    if any(r['type'] != 1027 for r in image.relocations) or any(s['name'] and not s['section'] for s in image.symbols):
        raise ValueError('Unsupported runtime imports/relocations')
    target = json.loads((profile/'profile.json').read_text())
    accepted = False
    if acceptance is not None:
        proof = json.loads(acceptance.read_text())
        if proof.get('module_sha256') != sha(module): raise ValueError('Acceptance identifies another runtime')
        accepted = True
    stage = output/'RUNTIME'
    relative = 'atmosphere/contents/'+target['title_id']+'/exefs/subsdk5'
    destination = stage/relative; destination.parent.mkdir(parents=True); shutil.copy2(module, destination)
    sd_access = stage_sd_access(stage)
    for name in ('LICENSE', 'NOTICE', 'THIRD-PARTY-NOTICES.txt'): shutil.copy2(ROOT/name, stage/name)
    text = '''# ScriptHookV-NX - Runtime Public Preview

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX

Close GTA. Back up any existing exefs/subsdk5 and main.npdm, then copy **atmosphere** to the
SD root. Launch Story Mode. Keep your existing mod bindings and saved settings.
Put compatible mods in switch/gtav-scripthook/plugins/ (.nro=Auto,
.nro.manual=Manual, .nro.disabled=Disabled). Restart GTA after replacing code.

Target title: {title}. Game build: {build}.
{status}
The included main.npdm supplies shared SD access. For a customized NPDM, run
Install-SD-Access.cmd to preserve it, then copy subsdk5 separately. See SD-ACCESS.md.
This download contains the runtime, SD-access metadata/tools and required attribution.
No menu, NRO mods, game archive or saved settings are included. An NSC menu is
optional; Auto plugins work independently. The standalone manager source is in
the SDK/repository. Check resident_limit in loader-report.txt for this runtime's
module capacity; restart GTA to release resident slots.
Restore your backed-up module to roll back. Disable dependent scripts before
removing the runtime. See the repository's install guide for details.
'''.format(title=target['title_id'], build=target['build_id'], status=
           'This exact runtime has console acceptance; new plugins need their own tests.' if accepted else
           'This runtime has no matched console acceptance and needs Switch testing.')
    (stage/'READ-ME-FIRST.md').write_text(text, encoding='utf-8')
    files = {p.relative_to(stage).as_posix(): sha(p) for p in stage.rglob('*') if p.is_file()}
    archive = output/'ScriptHookV-NX-Runtime-Public-Preview.zip'
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as z:
        for name in sorted(files): z.write(stage/name, name)
    with zipfile.ZipFile(archive) as z:
        if set(z.namelist()) != set(files): raise ValueError('ZIP inventory mismatch')
        for name, digest in files.items():
            if hashlib.sha256(z.read(name)).hexdigest() != digest: raise ValueError('ZIP readback mismatch')
    result = dict(module_sha256=sha(module), console_acceptance_matched=accepted,
                  target_build=target['build_id'], game_files_included=True, executable_or_game_assets_included=False, sd_access=sd_access,
                  plugins_or_bindings_or_settings_included=False, files=files,
                  zip_sha256=sha(archive), zip_bytes=archive.stat().st_size, zip_readback_verified=True)
    (output/'package-verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ('module_sha256', 'console_acceptance_matched', 'zip_bytes')}, indent=2))
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--module', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True); p.add_argument('--acceptance', type=Path)
    p.add_argument('--target-profile', type=Path, default=DEFAULT_PROFILE); a = p.parse_args()
    try: package(a.module.resolve(), a.output.resolve(), a.acceptance, a.target_profile)
    except (OSError, ValueError) as error: p.exit(1, str(error)+'\n')
