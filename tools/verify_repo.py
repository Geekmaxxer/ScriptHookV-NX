"""Run public repository checks with optional desktop, UI-model and NRO builds."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
from doctor import diagnose
from generate_bindings import profile_bindings

ROOT=Path(__file__).resolve().parents[1]
def verify(output,desktop=False,cxx=None,devkit=None):
    if output.exists():raise FileExistsError('Choose a fresh verification directory')
    output.mkdir(parents=True);checks=[]
    environment=dict(os.environ)
    if devkit:environment['DEVKITPRO']=str(devkit)
    def run(label,args):
        with (output/(label+'.log')).open('w',encoding='utf-8') as log:
            subprocess.run([str(a) for a in args],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,env=environment,check=True)
        checks.append(label);print('PASS',label,flush=True)
    diagnosis=diagnose(ROOT,devkit,check_toolchain=bool(devkit),core=True)
    (output/'doctor.json').write_text(json.dumps(diagnosis,indent=2)+'\n')
    # diagnose() returns the individual check list plus the aggregate result.
    if diagnosis['failed']:raise ValueError('SDK/profile checks failed; see doctor.json')
    profile_bindings(output/'target-bindings')
    checks.append('native-index-and-target-profile')
    run('portable-tools',[sys.executable,ROOT/'tools/test_portable.py'])
    if desktop:
        run('desktop-tests',['dotnet','run','--project',ROOT/'desktop/ModPorter.Tests/ModPorter.Tests.csproj','-c','Release','--',ROOT,*([] if devkit else ['--no-toolchain'])])
        if sys.platform=='win32':run('desktop-ui-build',['dotnet','build',ROOT/'desktop/ModPorter.App/ModPorter.App.csproj','-c','Release','-v','quiet'])
    if cxx:run('ui-model',[sys.executable,ROOT/'tools/run_ui_tests.py','--output',output/'ui-model','--cxx',cxx])
    if devkit:run('nro-examples',[sys.executable,ROOT/'tools/check_examples.py','--output',output/'examples','--devkitpro',devkit])
    result=dict(checks=checks,passed=True,scope='Local source, profile, desktop and requested compiler checks',
        native_header_sha256=hashlib.sha256((ROOT/'include/natives.h').read_bytes()).hexdigest(),
        skipped=[label for label,enabled in [('desktop',desktop),('host-ui-model',bool(cxx)),('nro-builds',bool(devkit))] if not enabled]+(['desktop-compiler-groups'] if desktop and not devkit else []),
        switch_runtime_verified=False)
    (output/'verification.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--desktop',action='store_true');p.add_argument('--cxx');p.add_argument('--devkitpro',type=Path);a=p.parse_args()
    try:verify(a.output.resolve(),a.desktop,a.cxx,a.devkitpro.resolve() if a.devkitpro else None)
    except (OSError,ValueError,KeyError,subprocess.CalledProcessError) as e:p.exit(1,str(e)+'\n')
