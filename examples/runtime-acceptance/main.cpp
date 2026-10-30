#include "script.h"
#include "shv/managed.h"
#include "shv/containers.h"
#include "shv/text.h"
#include "shv/xml.h"

namespace {
constexpr size_t Chunk = 16384;
constexpr uint64_t PayloadBytes = 256 * 1024;
constexpr uint64_t Required = SHV_NX_CAP_MULTIPLE_SCRIPTS | SHV_NX_CAP_RENDER_CALLBACKS |
    SHV_NX_CAP_CUSTOM_TEXTURES | SHV_NX_CAP_ENTITY_ENUMERATION | SHV_NX_CAP_ENTITY_POINTERS;
struct Counters {
    uint32_t run, error, heartbeats, worker_ticks, render_frames, draw_failures;
    uint32_t xml_roundtrips, catalog_pages, entity_samples, valid_entity_borrows;
    uint32_t session_destructors, stop_callbacks, report_failures;
    uint64_t written, verified;
    int texture;
    bool catalog_found, mutation_verified, startup_complete, cleanup_reclaimed, trim_succeeded;
    ShvNxMemoryStats baseline;
};
Counters state{};
uint32_t next_run;
uint8_t buffer[Chunk];
ShvNxDirectoryEntry entries[4];
int entity_handles[16];

template<size_t N> void Field(ScriptHook::Util::TextBuffer<N>& text, const char* key, uint64_t value) {
    text.Add(key).Add("=");
    char reversed[20], digits[21]{}; unsigned count = 0, used = 0;
    do { reversed[count++] = char('0' + value % 10); value /= 10; } while (value);
    while (count) digits[used++] = reversed[--count];
    text.Add(digits).Add("\n");
}

bool Report(const char* phase, const char* file = "status.txt") {
    ScriptHook::Util::TextBuffer<2048> text;
    text.Add("ScriptHookV-NX combined acceptance probe\nphase=").Add(phase).Add("\n");
    Field(text,"schema",1); Field(text,"run",state.run); Field(text,"operation_error",state.error);
    Field(text,"startup_complete",state.startup_complete); Field(text,"heartbeats",state.heartbeats);
    Field(text,"worker_ticks",state.worker_ticks); Field(text,"render_frames",state.render_frames);
    Field(text,"draw_failures",state.draw_failures); Field(text,"written_bytes",state.written);
    Field(text,"verified_bytes",state.verified); Field(text,"xml_roundtrips",state.xml_roundtrips);
    Field(text,"catalog_pages",state.catalog_pages); Field(text,"catalog_payload_found",state.catalog_found);
    Field(text,"mutation_verified",state.mutation_verified); Field(text,"entity_samples",state.entity_samples);
    Field(text,"valid_entity_borrows",state.valid_entity_borrows);
    Field(text,"session_destructors",state.session_destructors); Field(text,"stop_callbacks",state.stop_callbacks);
    Field(text,"resource_count",scriptResourceCount()); Field(text,"report_failures",state.report_failures);
    Field(text,"baseline_allocations",state.baseline.allocations);
    Field(text,"baseline_requested_bytes",state.baseline.requested_bytes);
    Field(text,"cleanup_reclaimed",state.cleanup_reclaimed); Field(text,"trim_succeeded",state.trim_succeeded);
    ShvNxMemoryStats memory{}; const bool stats = scriptGetMemoryStats(&memory);
    Field(text,"memory_stats_available",stats);
    Field(text,"live_allocations",memory.allocations); Field(text,"requested_bytes",memory.requested_bytes);
    Field(text,"charged_bytes",memory.charged_bytes); Field(text,"peak_requested_bytes",memory.peak_requested_bytes);
    Field(text,"memory_failures",memory.failures); Field(text,"release_failures",memory.release_failures);
    const bool saved = text.Complete() && scriptSaveFile(file,text.CStr(),text.Size());
    if (!saved) ++state.report_failures;
    return saved;
}

void Fail(uint32_t code) {
    if (!state.error) state.error = code;
    (void)Report("failed");
    scriptUnregisterAll();
}

struct Session {
    ScriptHook::Containers::String label;
    ScriptHook::Containers::Vector<uint64_t> samples;
    ~Session() noexcept { ++state.session_destructors; }
};

// Registered first, invoked last after the managed session and its children.
// A null context deliberately uses explicit POD module state, not a heap owner.
void FinalAudit(void*) {
    ShvNxMemoryStats memory{};
    const bool before_trim = scriptGetMemoryStats(&memory);
    const bool reclaimed = before_trim && memory.allocations == state.baseline.allocations &&
        memory.requested_bytes == state.baseline.requested_bytes && scriptResourceCount() == 0;
    const bool trimmed = scriptTrimMemory();
    state.cleanup_reclaimed = reclaimed; state.trim_succeeded = trimmed;
    if ((!reclaimed || !trimmed) && !state.error) state.error = 90;
    (void)Report(reclaimed && trimmed ? "stopped" : "cleanup_failed", "cleanup.txt");
    (void)Report(reclaimed && trimmed ? "stopped" : "cleanup_failed");
}

void Render(const ShvNxRenderFrame* frame) {
    if (!frame || frame->size < sizeof(*frame)) return;
    ++state.render_frames;
    ShvNxTextureDraw draw{};
    draw.id = state.texture; draw.sizeX = .035f; draw.sizeY = .035f;
    draw.centerX = .5f; draw.centerY = .5f; draw.posX = .965f; draw.posY = .08f;
    draw.heightScale = 1.f; draw.r = draw.g = draw.b = 1.f; draw.a = .8f;
    if (!drawTextureChecked(&draw)) { ++state.draw_failures; if (!state.error) state.error = 30; }
}
void Worker() { for (;;) { ++state.worker_ticks; WAIT(0); } }

uint8_t Pattern(uint64_t offset) { return uint8_t(offset * 37 + state.run * 11); }

bool Payload() {
    uint64_t size = 0;
    auto status = scriptFileSize("payload.bin", &size);
    if (status == ShvNxFile_NotFound) status = scriptFileCreate("payload.bin",PayloadBytes);
    else if (status == ShvNxFile_Ok && size != PayloadBytes) return false;
    if (status != ShvNxFile_Ok) return false;
    for (uint64_t offset = 0; offset < PayloadBytes; offset += Chunk) {
        for (size_t i = 0; i < Chunk; ++i) buffer[i] = Pattern(offset + i);
        if (scriptFileWriteAt("payload.bin",offset,buffer,Chunk) != ShvNxFile_Ok) return false;
        state.written += Chunk;
        if (!Report("writing")) return false;
        WAIT(0);
    }
    for (uint64_t offset = 0; offset < PayloadBytes; offset += Chunk) {
        size_t got = 0;
        if (scriptFileReadAt("payload.bin",offset,buffer,Chunk,&got) != ShvNxFile_Ok || got != Chunk) return false;
        for (size_t i = 0; i < Chunk; ++i) if (buffer[i] != Pattern(offset + i)) return false;
        state.verified += Chunk;
        WAIT(0);
    }
    return true;
}

bool Catalog() {
    uint32_t offset = 0;
    do {
        ShvNxDirectoryPage page{};
        if (scriptDirectoryList("",offset,entries,4,&page) != ShvNxFile_Ok) return false;
        ++state.catalog_pages;
        for (uint32_t i = 0; i < page.count; ++i) {
            const auto& entry = entries[i];
            if (entry.kind == SHV_NX_CATALOG_FILE && strcmp(entry.name,"payload.bin") == 0 && entry.bytes == PayloadBytes)
                state.catalog_found = true;
        }
        if (page.next_offset && page.next_offset <= offset) return false;
        offset = page.next_offset;
        WAIT(0);
    } while (offset);
    return state.catalog_found;
}

bool Mutation() {
    constexpr char expected[] = "NX probe marker\n";
    for (const char* name : {"rename-source.bin","rename-target.bin"}) {
        uint64_t size = 0; size_t got = 0;
        auto status = scriptFileSize(name,&size);
        if (status == ShvNxFile_NotFound) continue;
        if (status != ShvNxFile_Ok || size != sizeof(expected)-1 ||
            scriptFileReadAt(name,0,buffer,sizeof(expected)-1,&got) != ShvNxFile_Ok ||
            got != sizeof(expected)-1 || memcmp(buffer,expected,got) != 0) return false;
        if (scriptFileDelete(name) != ShvNxFile_Ok) return false;
    }
    if (!scriptSaveFile("rename-source.bin",expected,sizeof(expected)-1) ||
        scriptFileRename("rename-source.bin","rename-target.bin") != ShvNxFile_Ok) return false;
    size_t got = 0;
    if (scriptFileReadAt("rename-target.bin",0,buffer,sizeof(expected)-1,&got) != ShvNxFile_Ok ||
        got != sizeof(expected)-1 || memcmp(buffer,expected,got) != 0) return false;
    return scriptFileDelete("rename-target.bin") == ShvNxFile_Ok;
}

bool Xml() {
    size_t bytes = 0;
    {
        pugi::xml_document document;
        auto root = document.append_child("Probe");
        if (!root || !root.append_attribute("run").set_value(state.run)) return false;
        for (unsigned i = 0; i < 128; ++i) {
            auto row = root.append_child("Sample");
            if (!row || !row.append_attribute("index").set_value(i) ||
                !row.append_attribute("value").set_value(double(i)*.125 + 1.25)) return false;
        }
        if (!ScriptHook::Xml::Save(document,reinterpret_cast<char*>(buffer),sizeof(buffer),bytes)) return false;
    }
    if (!scriptSaveFile("snapshot.xml",buffer,bytes)) return false;
    size_t got = 0;
    if (!scriptReadFile("snapshot.xml",buffer,sizeof(buffer),&got) || got != bytes) return false;
    {
        pugi::xml_document document;
        if (!document.load_buffer(buffer,got)) return false;
        auto root = document.child("Probe");
        if (root.attribute("run").as_uint() != state.run) return false;
        unsigned index = 0;
        for (auto row : root.children("Sample")) {
            double value = 0.; const char* input = row.attribute("value").value();
            if (index >= 128 || row.attribute("index").as_uint() != index ||
                ScriptHook::Numeric::ParseDouble(input,strlen(input),value) != ScriptHook::Numeric::Status::Ok ||
                value != double(index)*.125 + 1.25) return false;
            ++index;
        }
        if (index != 128) return false;
    }
    ++state.xml_roundtrips;
    return true;
}

bool Initialize() {
    return scriptMemoryAvailable() && scriptResourcesAvailable() && scriptFilesAvailable() &&
        scriptCatalogAvailable() && scriptFileMutationsAvailable() &&
        (getScriptHookCapabilities() & Required) == Required;
}
void Stop() { ++state.stop_callbacks; }
void Main() {
    state = {}; state.run = ++next_run;
    if (!state.run || !scriptGetMemoryStats(&state.baseline)) { Fail(1); return; }
    if (!scriptResourceRegister(FinalAudit,nullptr)) { Fail(2); return; }
    auto session = ScriptHook::Memory::Managed<Session>::TryMake();
    if (!session || !session.Get()->label.TryAssign("combined runtime acceptance") ||
        !session.Get()->samples.TryReserve(1024)) { Fail(3); return; }
    state.texture = createTexture("probe.png");
    if (!state.texture || !renderCallbackRegister(Render) || !scriptRegisterAdditionalThread(Worker)) { Fail(4); return; }
    if (!Payload()) { Fail(10); return; }
    if (!Catalog()) { Fail(11); return; }
    state.mutation_verified = Mutation();
    if (!state.mutation_verified) { Fail(12); return; }
    if (!Xml()) { Fail(13); return; }
    state.startup_complete = true;
    for (;;) {
        if (state.error) { Fail(state.error); return; }
        auto* current = session.Get();
        if (!current) { Fail(20); return; }
        if (current->samples.Size() == 1024) current->samples.Clear();
        if (!current->samples.TryPushBack(uint64_t(state.heartbeats))) { Fail(21); return; }
        const int count = worldGetAllPeds(entity_handles,16);
        if (count < 0 || count > 16) { Fail(22); return; }
        ++state.entity_samples;
        if (count && getScriptHandleBaseAddress(entity_handles[0])) ++state.valid_entity_borrows;
        ++state.heartbeats;
        if (state.heartbeats % 10 == 1) {
            if (!Xml()) { Fail(23); return; }
            if (!Report("running")) { Fail(24); return; }
        }
        WAIT(100);
    }
}
}
SHV_NX_PLUGIN_WITH_LIFECYCLE("runtime-acceptance", Main, Initialize, Stop)
