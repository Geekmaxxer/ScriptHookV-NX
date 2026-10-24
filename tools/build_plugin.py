"""Build a freestanding ScriptHookV-NX NRO plugin from compatible C++ sources."""
import argparse,hashlib,json,shutil,subprocess,struct,os
from pathlib import Path
from sd_permissions import bundle, copy_distribution, stage as stage_sd_access

HEADER_SUFFIXES={'.h','.hh','.hpp','.hxx','.inl','.inc','.ipp','.tpp'}

def include_snapshot(roots):
    """Collect bounded, explicitly selected headers before creating an output."""
    if len(roots)>16:raise ValueError('Select at most 16 --include directories')
    result=[];total=0;entries=0
    for index,value in enumerate(roots):
        root=Path(value).resolve()
        if not root.is_dir():raise FileNotFoundError(f'Include directory does not exist: {root}')
        selected=[]
        def walk_error(error):raise error
        for directory,subdirs,names in os.walk(root,onerror=walk_error,followlinks=False):
            subdirs.sort();names.sort();entries+=len(subdirs)+len(names)
            if entries>16384:raise ValueError('Include directories contain too many entries; select smaller header directories')
            for name in subdirs:
                if (Path(directory)/name).is_symlink():raise ValueError(f'Include directory contains a symbolic link: {Path(directory)/name}')
            for name in names:
                path=Path(directory)/name
                if path.suffix.lower() not in HEADER_SUFFIXES:continue
                if path.is_symlink():raise ValueError(f'Include header is a symbolic link: {path}')
                if path.stat().st_size>2*1024*1024:raise ValueError(f'Include header exceeds 2 MiB: {path}')
                data=path.read_bytes();total+=len(data)
                if len(result)+len(selected)>=1024 or total>16*1024*1024:
                    raise ValueError('Selected include headers exceed 1,024 files or 16 MiB; select smaller header directories')
                selected.append((Path('headers')/f'include-{index}'/path.relative_to(root),data))
        if not selected:raise ValueError(f'Include directory contains no supported C/C++ headers: {root}')
        result.extend(selected)
    return result

def tool(devkit, relative):
    """devkitPro uses extensionless tools on POSIX and .exe tools on Windows."""
    path=devkit/relative
    for candidate in (path.with_name(path.name+'.exe'),path) if os.name=='nt' else (path,path.with_name(path.name+'.exe')):
        if candidate.is_file():return candidate
    raise FileNotFoundError(f'Missing {relative} in {devkit}; install devkitA64 and switch-tools or pass --devkitpro')

def toolchain(project, explicit=None):
    if explicit is not None:
        devkit=Path(explicit).resolve()
        return tool(devkit,'devkitA64/bin/aarch64-none-elf-g++'),tool(devkit,'tools/bin/elf2nro')
    candidates=[]
    if os.environ.get('DEVKITPRO'):candidates.append(Path(os.environ['DEVKITPRO']))
    candidates += [project.parent/'devkitPro',Path('C:/devkitPro'),Path('/opt/devkitpro')]
    for candidate in candidates:
        try:return tool(candidate,'devkitA64/bin/aarch64-none-elf-g++'),tool(candidate,'tools/bin/elf2nro')
        except FileNotFoundError:pass
    raise FileNotFoundError('Install devkitA64 and switch-tools, then set DEVKITPRO or pass --devkitpro')

def build(sources,output,devkit=None,includes=None):
    project=Path(__file__).resolve().parents[1]
    output=Path(output).resolve()
    if any(output.resolve().is_relative_to(project/folder) for folder in ('include','sdk')):
        raise ValueError('Output must be outside SDK include/sdk trees being snapshotted')
    sources=[Path(source).resolve() for source in sources]
    if not sources:raise ValueError('Provide at least one C++ source file')
    for source in sources:
        if not source.is_file():raise FileNotFoundError(f'Source file does not exist: {source}')
    includes=[] if includes is None else list(includes)
    headers=include_snapshot(includes)
    bundle(project)
    compiler,elf2nro=toolchain(project,devkit)
    output.mkdir(parents=True,exist_ok=False)
    copy_distribution(output,project)
    shutil.copytree(project/'include',output/'include');shutil.copytree(project/'sdk',output/'sdk')
    for relative,data in headers:
        path=output/relative;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    flags=['-march=armv8-a+crc','-mtune=cortex-a57','-mtp=soft','-fPIC','-O2','-g','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-exceptions','-fno-rtti','-fno-threadsafe-statics','-fvisibility=hidden','-mno-outline-atomics','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(output/'include')]
    flags += ['-ffile-prefix-map='+str(output)+'=/shvnx-plugin', '-fdebug-prefix-map='+str(output)+'=/shvnx-plugin']
    flags += ['-I'+str(output/'headers'/f'include-{i}') for i in range(len(includes))]
    units=[output/'sdk/plugin_api.cpp',output/'sdk/plugin_start.s']
    for i,source in enumerate(sources):
        dest=output/(str(i)+'-'+source.name);shutil.copy2(source,dest);units.append(dest)
    objects=[]
    for i,source in enumerate(units):
        obj=output/(str(i)+'.o');subprocess.run([str(compiler),*flags,'-c',str(source),'-o',str(obj)],check=True,cwd=output);objects.append(str(obj))
    elf=output/'plugin.elf'
    subprocess.run([str(compiler),*flags,'-nostdlib','-nostartfiles','-shared',*objects,'-Wl,-T,'+str(output/'sdk/plugin_link.ld'),'-Wl,-Bsymbolic,-z,defs,--build-id=sha1,--hash-style=both','-o',str(elf)],check=True,cwd=output)
    nro=output/'plugin.nro';subprocess.run([str(elf2nro),str(elf),str(nro)],check=True)
    data=nro.read_bytes();assert data[0x10:0x14]==b'NRO0'
    size=struct.unpack_from('<I',data,0x18)[0];assert size==len(data) and size%4096==0
    mod=struct.unpack_from('<I',data,4)[0];dynamic=mod+struct.unpack_from('<i',data,mod+4)[0]
    while True:
        if dynamic<0 or dynamic+16>len(data):raise ValueError('NRO dynamic table is missing its terminator')
        tag,value=struct.unpack_from('<QQ',data,dynamic);dynamic+=16
        if not tag:break
        if tag in (12,13,25,26,32) and value:raise ValueError('Use explicit plugin initialization instead of global constructors/destructors')
    report={'nro_sha256':hashlib.sha256(data).hexdigest(),'bytes':size,'plugin_abi':1,'caller_abi':1,'runtime_abi':1,'switch_runtime_verified':False,
            'sd_access':stage_sd_access(output,project)}
    report['source_sha256']={p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in output.rglob('*') if p.is_file() and p.suffix.lower() in HEADER_SUFFIXES|{'.cpp','.cc','.cxx','.c','.s','.ld','.sch'}}
    report['include_snapshots']=[f'headers/include-{i}' for i in range(len(includes))]
    report['deployment_sha256']={p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (output/'distribution').rglob('*') if p.is_file()}
    (output/'plugin-build.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True,nargs='+');p.add_argument('--output',required=True,type=Path);p.add_argument('--devkitpro',type=Path)
    p.add_argument('--include',type=Path,action='append',default=[],metavar='DIR',help='Snapshot headers in DIR (repeat for multiple roots; nested paths are preserved)')
    a=p.parse_args();build([s.resolve() for s in a.source],a.output.resolve(),a.devkitpro,a.include)
