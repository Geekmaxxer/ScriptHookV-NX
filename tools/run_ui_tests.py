"""Run bounded UI model tests with a host C++ compiler or devkitA64 and Unicorn."""
import argparse,json,shutil,struct,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def run(output,cxx='c++',devkit=None):
    if output.exists():raise FileExistsError('Choose a fresh output directory')
    output.mkdir(parents=True)
    source=ROOT/'tests/ui_model_tests.cpp'
    flags=['-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include')]
    if devkit:
        from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM
        from unicorn.arm64_const import UC_ARM64_REG_SP,UC_ARM64_REG_X30,UC_ARM64_REG_X0
        from build_plugin import tool
        compiler=tool(devkit/'devkitA64/bin','aarch64-none-elf-g++')
        binary=output/'ui-tests.elf'
        subprocess.run([str(compiler),*flags,'-O2','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-builtin','-DSHVNX_FREESTANDING_TEST','-nostdlib','-Wl,-e,shvnx_ui_model_tests',str(source),'-o',str(binary)],check=True)
        data=binary.read_bytes();entry,phoff=struct.unpack_from('<QQ',data,24);phsize,phcount=struct.unpack_from('<HH',data,54)
        segments=[]
        for i in range(phcount):
            kind,_,offset,at,_,size,memory,_=struct.unpack_from('<IIQQQQQQ',data,phoff+i*phsize)
            if kind==1:segments.append((offset,at,size,memory))
        start=min(at&~4095 for _,at,_,_ in segments);end=max((at+memory+4095)&~4095 for _,at,_,memory in segments)
        if end-start>16*1024*1024:raise ValueError('Test ELF has excessive virtual span')
        uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM);uc.mem_map(start,end-start)
        for offset,at,size,_ in segments:uc.mem_write(at,data[offset:offset+size])
        stack=0x7000000000;done=0x7100000000;uc.mem_map(stack,0x20000);uc.mem_map(done,4096)
        uc.reg_write(UC_ARM64_REG_SP,stack+0x1fff0);uc.reg_write(UC_ARM64_REG_X30,done)
        uc.emu_start(entry,done,count=1000000);result=uc.reg_read(UC_ARM64_REG_X0)
        if result&0xffffffff:raise AssertionError(str(result&0xffffffff)+' UI model checks failed')
        report=dict(passed=result>>32,failed=0,scope='Real ARM64 UI model; no game/native rendering calls',switch_runtime_verified=False)
    else:
        compiler=shutil.which(cxx)
        if not compiler:raise FileNotFoundError('Host C++ compiler not found: '+cxx)
        binary=output/('ui-tests.exe' if __import__('os').name=='nt' else 'ui-tests')
        subprocess.run([compiler,*flags,str(source),'-o',str(binary)],check=True);subprocess.run([str(binary.resolve())],check=True)
        report=dict(failed=0,scope='Host C++ UI model; no game/native rendering calls',switch_runtime_verified=False)
    (output/'ui-tests.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--cxx',default='c++');p.add_argument('--devkitpro',type=Path);a=p.parse_args()
    try:run(a.output.resolve(),a.cxx,a.devkitpro.resolve() if a.devkitpro else None)
    except (OSError,ValueError,AssertionError,ImportError,subprocess.CalledProcessError) as e:p.exit(1,str(e)+'\n')
