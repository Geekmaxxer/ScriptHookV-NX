#include "storage.hpp"
namespace ScriptHook {
namespace {
bool full_path(char (&out)[192], const char* relative) {
    constexpr char prefix[]="shvnx:/switch/gtav-scripthook/";
    if (!relative) return false;
    size_t at=0;
    for (char c:prefix) { if (!c) break; out[at++]=c; }
    bool component=false;
    for (size_t i=0;i<128;++i) {
        char c=0;if (!Memory::Read(reinterpret_cast<uintptr_t>(relative)+i,c)) return false;
        if (!c) {out[at]=0;return component;}
        if (at+1>=sizeof(out) || (c=='.' && (!component || relative[i-1]=='.'))) return false;
        if (c=='/') {if (!component) return false;component=false;}
        else {if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.')) return false;component=true;}
        out[at++]=c;
    }
    return false;
}
}
Storage::Storage() {
    if (!Memory::ResolveSdk()) return;
    sdk_call<void(*)(uintptr_t*)>(binding::disable_thread_abort)(&previous); scoped=true;
    error=sdk_call<uint32_t(*)(const char*)>(binding::mount_sd_debug)("shvnx");mounted=!error;
    if (!mounted) return;
    const char* dirs[]={"shvnx:/switch","shvnx:/switch/gtav-scripthook","shvnx:/switch/gtav-scripthook/plugins","shvnx:/switch/gtav-scripthook/settings","shvnx:/switch/gtav-scripthook/bindings"};
    for (const char* p:dirs)
        (void)main_call<uint32_t(*)(const char*)>(binding::create_directory)(p);
}
Storage::~Storage() {
    if (mounted) sdk_call<void(*)(const char*)>(binding::unmount)("shvnx");
    if (scoped) sdk_call<void(*)(uintptr_t)>(binding::restore_thread_context)(previous);
}
bool Storage::Size(const char* name,size_t& bytes) {
    bytes=0;char path[192]{};if (!mounted||!full_path(path,name)) return false;
    uint64_t handle=0;error=main_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&handle,path,1);
    if (error) return false;
    int64_t size=0;error=sdk_call<uint32_t(*)(int64_t*,uint64_t)>(binding::fs_size)(&size,handle);
    main_call<void(*)(uint64_t)>(binding::close_file)(handle);
    if (error||size<0) return false;
    bytes=size;return true;
}
bool Storage::Read(const char* name,void* out,size_t capacity,size_t& bytes) {
    bytes=0;char path[192]{};if (!mounted||!capacity||!Memory::IsWritable(reinterpret_cast<uintptr_t>(out),capacity)||!full_path(path,name)) return false;
    uint64_t handle=0;error=main_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&handle,path,1);
    if (error) return false;
    int64_t size=0;error=sdk_call<uint32_t(*)(int64_t*,uint64_t)>(binding::fs_size)(&size,handle);
    if (!error && size>=0 && uint64_t(size)<=capacity)
        error=main_call<uint32_t(*)(size_t*,uint64_t,int64_t,void*,size_t)>(binding::read_file)(&bytes,handle,0,out,size);
    else if (!error) error=0x10001;
    main_call<void(*)(uint64_t)>(binding::close_file)(handle);
    return !error&&bytes==uint64_t(size);
}
bool Storage::Write(const char* name,const void* data,size_t bytes) {
    char path[192]{};if (!mounted||!bytes||bytes>65536||!Memory::IsReadable(reinterpret_cast<uintptr_t>(data),bytes)||!full_path(path,name)) return false;
    (void)main_call<uint32_t(*)(const char*,int64_t)>(binding::create_file)(path,bytes);
    uint64_t handle=0;error=main_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&handle,path,2);
    if (error) return false;
    error=main_call<uint32_t(*)(uint64_t,int64_t)>(binding::set_file_size)(handle,bytes);
    const int32_t flush=1;
    if (!error) error=main_call<uint32_t(*)(uint64_t,int64_t,const void*,size_t,const int32_t*)>(binding::write_file)(handle,0,data,bytes,&flush);
    main_call<void(*)(uint64_t)>(binding::close_file)(handle);return !error;
}
bool Storage::Rename(const char* fromRelative, const char* toRelative) {
    char fromFull[192]{}; char toFull[192]{};
    rename_attempted=false;rename_result=UINT32_MAX;rename_postcheck=0;
    if (!mounted || !full_path(fromFull, fromRelative) || !full_path(toFull, toRelative)) return false;
    size_t before=0,after=0,old=0;
    if(!Size(fromRelative,before))return false;
    rename_postcheck|=1;rename_attempted=true;rename_postcheck|=2;
    // Verified target export _ZN2nn2fs10RenameFileEPKcS2_: X0=old path,
    // X1=new path; W0 is the nn::Result (zero is success).
    rename_result=main_call<uint32_t(*)(const char*,const char*)>(binding::rename_file)(fromFull,toFull);
    if(rename_result){error=rename_result;return false;}
    rename_postcheck|=4;
    if(Size(toRelative,after)){rename_postcheck|=8;if(after==before)rename_postcheck|=16;}
    // Only the exact target FS PathNotFound result proves source absence.
    // Other failures (SD unavailable, permission or I/O) are not success.
    if(!Size(fromRelative,old)&&error==0x202)rename_postcheck|=32;
    if(rename_postcheck!=63){error=0x1001d;return false;}
    error=0;return true;
}
bool Storage::Delete(const char* relative) {
    char full[192]{};
    if (!mounted || !full_path(full, relative)) return false;
    error = main_call<uint32_t(*)(const char*)>(binding::delete_file)(full);
    return error == 0;
}
bool Storage::Plugins(DirectoryEntry* entries,size_t capacity,size_t& count) {
    return List("plugins",entries,capacity,count);
}
bool Storage::Bindings(DirectoryEntry* entries,size_t capacity,size_t& count) {
    return List("bindings",entries,capacity,count);
}
bool Storage::List(const char* relative,DirectoryEntry* entries,size_t capacity,size_t& count) {
    count=0;char path[192]{};if (!mounted||capacity>64||!full_path(path,relative)) return false;
    uint64_t handle=0;error=sdk_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::fs_open_dir)(&handle,path,2);
    if (error) return false;
    int64_t got=0;error=sdk_call<uint32_t(*)(int64_t*,DirectoryEntry*,uint64_t,int64_t)>(binding::fs_read_dir)(&got,entries,handle,capacity);
    sdk_call<void(*)(uint64_t)>(binding::fs_close_dir)(handle);
    if (error||got<0||uint64_t(got)>capacity) return false;
    count=got;return true;
}
bool Storage::Scan(const char* relative,DirectoryEntry* entries,size_t capacity,
                   void (*visit)(const DirectoryEntry&,void*),void* context,size_t& count) {
    count=0;char path[192]{};
    if(!mounted||!capacity||capacity>64||!visit||!full_path(path,relative)) {error=0x10010;return false;}
    uint64_t handle=0;error=sdk_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::fs_open_dir)(&handle,path,2);
    if(error)return false;
    for(;;){
        int64_t got=0;error=sdk_call<uint32_t(*)(int64_t*,DirectoryEntry*,uint64_t,int64_t)>(binding::fs_read_dir)(&got,entries,handle,capacity);
        if(error)break;
        if(got<0||uint64_t(got)>capacity){error=0x10011;break;}
        if(!got)break;
        if(uint64_t(got)>4096-count){error=0x1000f;break;}
        count+=size_t(got);
        for(size_t i=0;i<size_t(got);++i)visit(entries[i],context);
    }
    sdk_call<void(*)(uint64_t)>(binding::fs_close_dir)(handle);
    return !error;
}
}
