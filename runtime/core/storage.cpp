#include "storage.hpp"
namespace ScriptHook {
namespace {
ShvNxFileStatus file_result(uint32_t result) {
    if (!result) return ShvNxFile_Ok;
    if (result == 0x202) return ShvNxFile_NotFound;
    if (result == 0x402) return ShvNxFile_AlreadyExists;
    return ShvNxFile_IoError;
}
struct FileClose {
    uint64_t handle;
    ~FileClose() { main_call<void(*)(uint64_t)>(binding::close_file)(handle); }
};
struct DirectoryClose {
    uint64_t handle;
    ~DirectoryClose() { sdk_call<void(*)(uint64_t)>(binding::fs_close_dir)(handle); }
};
bool compatible_component(const char* name, size_t bytes) {
    if (!bytes || bytes > 31 || name[0] == '.' || name[bytes - 1] == '.') return false;
    for (size_t i = 0; i < bytes; ++i) {
        const char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
        if (c == '.' && i && name[i - 1] == '.') return false;
    }
    return true;
}
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
ShvNxFileStatus Storage::ReadRange(const char* name, uint64_t offset, void* out, size_t capacity, size_t& bytes) {
    bytes = 0;
    char path[192]{};
    if (!mounted) return ShvNxFile_IoError;
    if (offset > SHV_NX_FILE_MAX_BYTES) return ShvNxFile_OutOfRange;
    if (capacity > SHV_NX_FILE_MAX_CHUNK ||
        (capacity && !Memory::IsWritable(reinterpret_cast<uintptr_t>(out), capacity)) ||
        !full_path(path, name)) return ShvNxFile_InvalidArgument;
    uint64_t handle = 0;
    error = main_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&handle, path, 1);
    if (error) return file_result(error);
    const FileClose close{handle};
    int64_t size = 0;
    error = sdk_call<uint32_t(*)(int64_t*,uint64_t)>(binding::fs_size)(&size, handle);
    if (error) return file_result(error);
    if (size < 0 || uint64_t(size) > SHV_NX_FILE_MAX_BYTES || offset > uint64_t(size)) return ShvNxFile_OutOfRange;
    const size_t remaining = size_t(uint64_t(size) - offset);
    const size_t requested = capacity < remaining ? capacity : remaining;
    if (!requested) return ShvNxFile_Ok;
    size_t got = 0;
    error = main_call<uint32_t(*)(size_t*,uint64_t,int64_t,void*,size_t)>(binding::read_file)(&got, handle, int64_t(offset), out, requested);
    if (error) return file_result(error);
    if (got != requested) return ShvNxFile_IncompleteRead;
    bytes = got;
    return ShvNxFile_Ok;
}
ShvNxFileStatus Storage::WriteRange(const char* name, uint64_t offset, const void* data, size_t bytes) {
    char path[192]{};
    if (!mounted) return ShvNxFile_IoError;
    if (offset > SHV_NX_FILE_MAX_BYTES) return ShvNxFile_OutOfRange;
    if (bytes > SHV_NX_FILE_MAX_CHUNK ||
        (bytes && !Memory::IsReadable(reinterpret_cast<uintptr_t>(data), bytes)) ||
        !full_path(path, name)) return ShvNxFile_InvalidArgument;
    uint64_t handle = 0;
    error = main_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::open_file)(&handle, path, 2);
    if (error) return file_result(error);
    const FileClose close{handle};
    int64_t size = 0;
    error = sdk_call<uint32_t(*)(int64_t*,uint64_t)>(binding::fs_size)(&size, handle);
    if (error) return file_result(error);
    if (size < 0 || uint64_t(size) > SHV_NX_FILE_MAX_BYTES || offset > uint64_t(size) || bytes > uint64_t(size) - offset)
        return ShvNxFile_OutOfRange;
    if (!bytes) return ShvNxFile_Ok;
    const int32_t flush = 1;
    error = main_call<uint32_t(*)(uint64_t,int64_t,const void*,size_t,const int32_t*)>(binding::write_file)(handle, int64_t(offset), data, bytes, &flush);
    return file_result(error);
}
ShvNxFileStatus Storage::CreateSized(const char* name, uint64_t bytes) {
    char path[192]{};
    if (!mounted) return ShvNxFile_IoError;
    if (!full_path(path, name)) return ShvNxFile_InvalidArgument;
    if (bytes > SHV_NX_FILE_MAX_BYTES) return ShvNxFile_OutOfRange;
    // The audited CreateFile result distinguishes an existing destination.
    // Do not follow creation with OpenFile/SetFileSize on an existing file.
    error = main_call<uint32_t(*)(const char*,int64_t)>(binding::create_file)(path, int64_t(bytes));
    return file_result(error);
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
ShvNxFileStatus Storage::RenameBounded(const char* from, const char* to) {
    if (!mounted) return ShvNxFile_IoError;
    size_t bytes = 0;
    if (!Size(from, bytes)) return error ? file_result(error) : ShvNxFile_IoError;
    if (bytes > SHV_NX_FILE_MAX_BYTES) return ShvNxFile_OutOfRange;
    if (Rename(from, to)) return ShvNxFile_Ok;
    if (rename_attempted && !rename_result) return ShvNxFile_VerificationFailed;
    return error ? file_result(error) : ShvNxFile_IoError;
}
ShvNxFileStatus Storage::DeleteBounded(const char* relative) {
    if (!mounted) return ShvNxFile_IoError;
    size_t bytes = 0;
    if (!Size(relative, bytes)) return error ? file_result(error) : ShvNxFile_IoError;
    if (bytes > SHV_NX_FILE_MAX_BYTES) return ShvNxFile_OutOfRange;
    if (!Delete(relative)) return error ? file_result(error) : ShvNxFile_IoError;
    if (!Size(relative, bytes) && error == 0x202) return ShvNxFile_Ok;
    return ShvNxFile_VerificationFailed;
}
bool Storage::Directory(const char* relative) {
    char path[192]{};
    if(!mounted||!full_path(path,relative))return false;
    error=main_call<uint32_t(*)(const char*)>(binding::create_directory)(path);
    if(error==0x402)error=0; // Already exists; subsequent I/O verifies the path.
    return error==0;
}
bool Storage::DirectoryChecked(const char* relative) {
    if (!Directory(relative)) return false;
    char path[192]{};
    if (!full_path(path, relative)) return false;
    uint64_t handle = 0;
    error = sdk_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::fs_open_dir)(&handle, path, 2);
    if (error) return false;
    sdk_call<void(*)(uint64_t)>(binding::fs_close_dir)(handle);
    return true;
}
ShvNxFileStatus Storage::CatalogPage(const char* relative, uint32_t offset,
    ShvNxDirectoryEntry* entries, uint32_t capacity, ShvNxDirectoryPage& page) {
    page = {};
    if (!mounted) return ShvNxFile_IoError;
    char path[192]{};
    if (!capacity || capacity > SHV_NX_CATALOG_PAGE_ENTRIES || !full_path(path, relative))
        return ShvNxFile_InvalidArgument;
    if (offset > SHV_NX_CATALOG_MAX_ENTRIES) return ShvNxFile_OutOfRange;
    uint64_t handle = 0;
    // Reviewed target accessor accepts bits 0/1 and forwards them unchanged:
    // mode 3 requests both directories and files, including their file sizes.
    error = sdk_call<uint32_t(*)(uint64_t*,const char*,int32_t)>(binding::fs_open_dir)(&handle, path, 3);
    if (error) return file_result(error);
    const DirectoryClose close{handle};
    // The owner-thread service never yields/reenters. Keep the target-sized
    // read buffer off plugin cooperative stacks and out of the plugin heap.
    static DirectoryEntry scratch[SHV_NX_CATALOG_PAGE_ENTRIES];
    uint32_t seen = 0, written = 0;
    for (;;) {
        const uint32_t remaining = seen < offset ? offset - seen : capacity - written;
        const uint32_t request = !remaining ? 1 : remaining < SHV_NX_CATALOG_PAGE_ENTRIES ? remaining : SHV_NX_CATALOG_PAGE_ENTRIES;
        int64_t got = 0;
        error = sdk_call<uint32_t(*)(int64_t*,DirectoryEntry*,uint64_t,int64_t)>(binding::fs_read_dir)(&got, scratch, handle, request);
        if (error) return file_result(error);
        if (got < 0 || uint64_t(got) > request) return ShvNxFile_InvalidEntry;
        if (!got) { page = {written, 0}; return ShvNxFile_Ok; }
        if (uint64_t(got) > SHV_NX_CATALOG_MAX_ENTRIES - seen) return ShvNxFile_DirectoryLimit;
        if (seen < offset) { seen += uint32_t(got); continue; }
        // One lookahead entry distinguishes a full final page from a page
        // with a successor. It is reread on the next independent request.
        if (written == capacity) { page = {written, seen}; return ShvNxFile_Ok; }
        for (int64_t i = 0; i < got; ++i) {
            const auto& input = scratch[i];
            size_t length = 0;
            while (length < sizeof(input.name) && input.name[length]) ++length;
            if (!length || length == sizeof(input.name) || input.type > 1 || (input.type == 1 && input.bytes < 0))
                return ShvNxFile_InvalidEntry;
            ShvNxDirectoryEntry output{};
            output.kind = input.type == 0 ? SHV_NX_CATALOG_DIRECTORY : SHV_NX_CATALOG_FILE;
            output.flags = compatible_component(input.name, length) ? SHV_NX_CATALOG_COMPATIBLE_NAME : 0;
            output.bytes = input.type == 1 ? uint64_t(input.bytes) : 0;
            for (size_t n = 0; n < length; ++n) output.name[n] = input.name[n];
            entries[written++] = output;
        }
        seen += uint32_t(got);
    }
}
bool Storage::MigrateFile(const char* legacy,const char* current) {
    size_t bytes=0;
    if(Size(current,bytes))return true;
    if(error!=0x202)return false;
    if(!Size(legacy,bytes))return error==0x202;
    // nn::fs::RenameFile refuses an existing destination and preserves the
    // source on failure. Rename also verifies size and source disappearance.
    return Rename(legacy,current);
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
        if(uint64_t(got)>SIZE_MAX-count){error=0x1000f;break;}
        count+=size_t(got);
        for(size_t i=0;i<size_t(got);++i)visit(entries[i],context);
    }
    sdk_call<void(*)(uint64_t)>(binding::fs_close_dir)(handle);
    return !error;
}
}
