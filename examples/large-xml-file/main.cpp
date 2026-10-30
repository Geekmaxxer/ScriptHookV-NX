#include "script.h"
#include "shv/file_transfer.h"
#include "shv/session.h"
#include "shv/xml.h"
#include "shv/text.h"

namespace {
constexpr size_t Length = 256*1024, Chunk = 16384;
uint8_t chunk[Chunk];
struct Session {
    ScriptHook::Data::FileReadTask reader;
    ScriptHook::Data::PreparedFileWriteTask writer;
    pugi::xml_document document;
    // The pinned no-exceptions XML document constructor only initializes its
    // embedded state; parse/DOM allocation failures remain explicit results.
    Session() noexcept = default;
};
using Shared = ScriptHook::Memory::SessionLocal<Session>;
void Report(bool passed) {
    constexpr char good[] = "verified_bytes=262144\nxml_value=2699\nfailed=0\n";
    constexpr char bad[] = "failed=1\nexisting_file_preserved=1\n";
    if (passed) (void)scriptSaveFile("result.txt",good,sizeof(good)-1);
    else (void)scriptSaveFile("result.txt",bad,sizeof(bad)-1);
}
void FinalAudit(void*) {
    ShvNxMemoryStats memory{};
    const bool clean = !Shared::Get() && scriptGetMemoryStats(&memory) &&
        !memory.allocations && !scriptResourceCount() && scriptTrimMemory();
    ScriptHook::Util::TextBuffer<128> text;
    text.Add("owner_cleanup_complete=").UInt(clean).Add("\n");
    (void)scriptSaveFile("cleanup.txt",text.CStr(),text.Size());
}
bool Initialize() { return scriptFilesAvailable() && scriptMemoryAvailable() && scriptResourcesAvailable(); }
void Main() {
    if (!scriptResourceRegister(FinalAudit,nullptr)) return;
    auto* session = Shared::TryGet();
    if (!session) return;
    uint64_t existing = 0;
    const auto status = scriptFileSize("sample.xml",&existing);
    if (status == ShvNxFile_NotFound) {
        if (!session->writer.Begin("sample.xml",Length)) { Report(false); return; }
        constexpr char start[] = "<root><value>2699</value></root>";
        while (session->writer.Transferred() < Length) {
            const uint64_t offset = session->writer.Transferred();
            for (size_t i = 0; i < Chunk; ++i) chunk[i] = offset+i < sizeof(start)-1 ? uint8_t(start[offset+i]) : uint8_t(' ');
            if (session->writer.Append(chunk,Chunk) == ScriptHook::Data::TransferState::Failed) { Report(false); return; }
            WAIT(0);
            session = Shared::Get();
            if (!session) return;
        }
        if (session->writer.Finish() != ScriptHook::Data::TransferState::Complete) { Report(false); return; }
    } else if (status != ShvNxFile_Ok || existing != Length) { Report(false); return; }
    if (!session->reader.Begin("sample.xml",Length,Chunk)) { Report(false); return; }
    while (session->reader.State() == ScriptHook::Data::TransferState::Active) {
        session->reader.Step();
        WAIT(0);
        session = Shared::Get();
        if (!session) return;
    }
    if (!session->reader.Data() || session->reader.Size() != Length) { Report(false); return; }
    const auto parsed = session->document.load_buffer(session->reader.Data(),size_t(session->reader.Size()));
    Report(parsed && session->document.child("root").child("value").text().as_int() == 2699);
}
}
SHV_NX_PLUGIN_WITH_INIT("large-xml-example",Main,Initialize)
