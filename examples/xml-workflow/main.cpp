#include "script.h"
#include "shv/file_verify.h"
#include "shv/file_replacement.h"
#include "shv/session.h"
#include "shv/xml_buffer.h"
#include "shv/text.h"
#include "shv/catalog_scan.h"

namespace {
constexpr size_t InputBytes = 256*1024, PayloadBytes = 96*1024, Chunk = 16384;
constexpr char Original[] = "<root><value>1</value></root>";
constexpr char Preset[] = "<preset value=\"2699\"/>";
uint8_t chunk[Chunk];
char payload[PayloadBytes+1];
uint32_t next_run, stage;
size_t output_bytes;
bool catalog_verified;
uint32_t catalog_count, catalog_pages, catalog_skipped;
struct Session {
    ScriptHook::Data::FileReadTask input;
    ScriptHook::Data::PreparedFileWriteTask output;
    ScriptHook::Data::FileVerifyTask verify;
    ScriptHook::Xml::OwnedBufferWriter serialized;
    ScriptHook::Data::CatalogScanTask catalog;
    pugi::xml_document document;
    Session() noexcept = default;
};
using Shared = ScriptHook::Memory::SessionLocal<Session>;
using TransferState = ScriptHook::Data::TransferState;

void Report(bool good) {
    ScriptHook::Util::TextBuffer<384> text;
    text.Add("run=").UInt(next_run).Add("\nfailed=").UInt(!good).Add("\nstage=").UInt(stage)
        .Add("\ninput_bytes=").UInt(InputBytes).Add("\nserialized_bytes=").UInt(uint32_t(output_bytes))
        .Add("\npublication_verified=").UInt(good).Add("\nbackup_restored=").UInt(good)
        .Add("\ncatalog_verified=").UInt(catalog_verified).Add("\ncatalog_count=").UInt(catalog_count)
        .Add("\ncatalog_pages=").UInt(catalog_pages).Add("\ncatalog_skipped=").UInt(catalog_skipped).Add("\n");
    (void)scriptSaveFile("result.txt",text.CStr(),text.Size());
}
void FinalAudit(void*) {
    ShvNxMemoryStats memory{};
    const bool clean = !Shared::Get() && scriptGetMemoryStats(&memory) &&
        !memory.allocations && !scriptResourceCount() && scriptTrimMemory();
    ScriptHook::Util::TextBuffer<128> text;
    text.Add("run=").UInt(next_run).Add("\nowner_cleanup_complete=").UInt(clean).Add("\n");
    (void)scriptSaveFile("cleanup.txt",text.CStr(),text.Size());
}
bool Verify(const char* path, const void* data, size_t bytes) {
    auto* session = Shared::Get();
    if (!session || !session->verify.Reset() || !session->verify.Begin(path,bytes,Chunk)) return false;
    const auto* reference = static_cast<const uint8_t*>(data);
    while (session->verify.Transferred() < bytes) {
        const size_t offset = size_t(session->verify.Transferred());
        const size_t count = bytes-offset < Chunk ? bytes-offset : Chunk;
        if (session->verify.Compare(reference+offset,count) == TransferState::Failed) return false;
        WAIT(0);
        session = Shared::Get();
        if (!session) return false;
    }
    return session->verify.Finish() == TransferState::Complete;
}
bool Write(const char* path, const void* data, size_t bytes) {
    auto* session = Shared::Get();
    if (!session) return false;
    session->output.Reset();
    if (!session->output.Begin(path,bytes)) return false;
    const auto* source = static_cast<const uint8_t*>(data);
    while (session->output.Transferred() < bytes) {
        const size_t offset = size_t(session->output.Transferred());
        const size_t count = bytes-offset < Chunk ? bytes-offset : Chunk;
        if (session->output.Append(source+offset,count) == TransferState::Failed) return false;
        WAIT(0);
        session = Shared::Get();
        if (!session) return false;
    }
    return session->output.Finish() == TransferState::Complete;
}
bool FixtureFile(const char* path, const char* data, size_t bytes) {
    const auto created = scriptFileCreate(path,bytes);
    if (created == ShvNxFile_Ok && scriptFileWriteAt(path,0,data,bytes) != ShvNxFile_Ok) return false;
    if (created != ShvNxFile_Ok && created != ShvNxFile_AlreadyExists) return false;
    uint64_t length = 0; size_t read = 0;
    return scriptFileSize(path,&length) == ShvNxFile_Ok && length == bytes &&
        scriptFileReadAt(path,0,chunk,bytes,&read) == ShvNxFile_Ok && read == bytes &&
        memcmp(chunk,data,bytes) == 0;
}
bool PresetCatalog() {
    if (scriptDirectoryCreate("presets") != ShvNxFile_Ok ||
            scriptDirectoryCreate("presets/nested") != ShvNxFile_Ok) return false;
    for (unsigned i = 0; i < 13; ++i) {
        char path[] = "presets/preset-00.xml";
        constexpr size_t digit = sizeof("presets/preset-")-1;
        path[digit] = char('0'+i/10); path[digit+1] = char('0'+i%10);
        if (i & 1) { path[sizeof(path)-4] = 'X'; path[sizeof(path)-3] = 'M'; path[sizeof(path)-2] = 'L'; }
        if (!FixtureFile(path,Preset,sizeof(Preset)-1)) return false;
        WAIT(0);
    }
    constexpr char note[] = "catalog filter sample";
    if (!FixtureFile("presets/notes.txt",note,sizeof(note)-1)) return false;
    auto* session = Shared::Get();
    if (!session || !session->catalog.Begin("presets",32,".xml",true)) return false;
    for (;;) {
        const auto state = session->catalog.Step();
        if (state == ScriptHook::Data::CatalogState::Failed) return false;
        if (state == ScriptHook::Data::CatalogState::Complete) break;
        WAIT(0); session = Shared::Get(); if (!session) return false;
    }
    catalog_count = uint32_t(session->catalog.Count()); catalog_pages = uint32_t(session->catalog.Pages());
    catalog_skipped = uint32_t(session->catalog.Skipped());
    if (catalog_count != 14 || catalog_pages < 4 || catalog_skipped != 1) return false;
    const auto* folder = session->catalog.At(0);
    if (!folder || strcmp(folder->name,"nested") || folder->kind != SHV_NX_CATALOG_DIRECTORY) return false;
    for (size_t i = 1; i < 14; ++i) {
        const auto* record = session->catalog.At(i);
        if (!record || record->kind != SHV_NX_CATALOG_FILE || record->bytes != sizeof(Preset)-1 ||
                record->name[7] != char('0'+(i-1)/10) || record->name[8] != char('0'+(i-1)%10)) return false;
    }
    char selected[SHV_NX_FILE_MAX_PATH+1]{};
    if (!session->catalog.BuildPath(1,selected,sizeof(selected)) ||
            !FixtureFile(selected,Preset,sizeof(Preset)-1) ||
            !session->document.load_buffer(chunk,sizeof(Preset)-1) ||
            session->document.child("preset").attribute("value").as_int() != 2699) return false;
    catalog_verified = true;
    return true;
}
bool Work() {
    auto* session = Shared::TryGet();
    if (!session) return false;
    stage = 9;
    if (!PresetCatalog()) return false;
    session = Shared::Get(); if (!session) return false;
    stage = 1;
    uint64_t bytes = 0;
    const auto status = scriptFileSize("input.xml",&bytes);
    if (status == ShvNxFile_NotFound) {
        if (!session->output.Begin("input.xml",InputBytes)) return false;
        constexpr char start[] = "<root><value>2699</value></root>";
        while (session->output.Transferred() < InputBytes) {
            const uint64_t offset = session->output.Transferred();
            for (size_t i = 0; i < Chunk; ++i) chunk[i] = offset+i < sizeof(start)-1 ? uint8_t(start[offset+i]) : uint8_t(' ');
            if (session->output.Append(chunk,Chunk) == TransferState::Failed) return false;
            WAIT(0);
            session = Shared::Get();
            if (!session) return false;
        }
        if (session->output.Finish() != TransferState::Complete) return false;
    } else if (status != ShvNxFile_Ok || bytes != InputBytes) return false;
    stage = 2;
    if (!session->input.Begin("input.xml",InputBytes,Chunk)) return false;
    while (session->input.State() == TransferState::Active) {
        session->input.Step(); WAIT(0);
        session = Shared::Get(); if (!session) return false;
    }
    if (!session->input.Data() || session->input.Size() != InputBytes ||
            !session->document.load_buffer(session->input.Data(),InputBytes)) return false;
    auto root = session->document.child("root");
    if (root.child("value").text().as_int() != 2699) return false;
    stage = 3;
    for (size_t i = 0; i < PayloadBytes; ++i) payload[i] = 'p';
    if (!root.append_child("payload").text().set(payload) ||
            !session->serialized.Serialize(session->document,512*1024)) return false;
    output_bytes = session->serialized.Bytes();
    if (output_bytes <= 65536) return false;
    ScriptHook::Data::FileReplacement publication("current.xml","prepared.xml","backup.xml");
    ScriptHook::Data::ReplacementState files;
    if (publication.Inspect(files) != ShvNxFile_Ok) return false;
    uint64_t published_bytes = 0;
    const auto published = scriptFileSize("published.xml",&published_bytes);
    if (published == ShvNxFile_Ok) {
        // Completed runs are verification-only. Unknown/partial states remain.
        stage = 8;
        return files.present == files.Current &&
            Verify("current.xml",Original,sizeof(Original)-1) &&
            Verify("published.xml",session->serialized.Data(),output_bytes);
    }
    if (published != ShvNxFile_NotFound || files.present) return false;
    stage = 4;
    if (!Write("current.xml",Original,sizeof(Original)-1) ||
            !Verify("current.xml",Original,sizeof(Original)-1)) return false;
    session = Shared::Get(); if (!session) return false;
    stage = 5;
    if (!Write("prepared.xml",session->serialized.Data(),output_bytes)) return false;
    session = Shared::Get(); if (!session) return false;
    if (!Verify("prepared.xml",session->serialized.Data(),output_bytes)) return false;
    stage = 6;
    if (publication.Publish() != ShvNxFile_Ok) return false;
    session = Shared::Get(); if (!session) return false;
    if (!Verify("current.xml",session->serialized.Data(),output_bytes) ||
            !Verify("backup.xml",Original,sizeof(Original)-1)) return false;
    stage = 7;
    // Preserve the verified replacement before explicitly restoring the backup.
    if (scriptFileRename("current.xml","published.xml") != ShvNxFile_Ok ||
            publication.RestoreBackup() != ShvNxFile_Ok) return false;
    session = Shared::Get(); if (!session) return false;
    return Verify("current.xml",Original,sizeof(Original)-1) &&
        Verify("published.xml",session->serialized.Data(),output_bytes);
}
bool Initialize() {
    return scriptFilesAvailable() && scriptCatalogAvailable() && scriptFileMutationsAvailable() &&
        scriptMemoryAvailable() && scriptResourcesAvailable();
}
void Main() {
    ++next_run; stage = 0; output_bytes = 0;
    catalog_verified = false; catalog_count = catalog_pages = catalog_skipped = 0;
    if (!scriptResourceRegister(FinalAudit,nullptr)) return;
    Report(Work());
}
}
SHV_NX_PLUGIN_WITH_INIT("xml-workflow-example",Main,Initialize)
