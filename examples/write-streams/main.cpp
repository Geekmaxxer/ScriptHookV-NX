#include "script.h"
#include "shv/stdio.h"
#include "shv/xml.h"
#include "shv/text.h"

namespace {
uint32_t run, failed, completed, binary_verified, update_verified, append_verified,
    hole_verified, formatted_verified, xml_verified;
bool cleanup_complete;
constexpr uint32_t Bytes = 16387, Hole = 513;
unsigned char Byte(uint32_t at) { return static_cast<unsigned char>(at * 43u + 255u); }

void Report() {
    ShvNxMemoryStats stats{};
    const bool available = scriptGetMemoryStats(&stats);
    ScriptHook::Util::TextBuffer<512> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nbinary_verified=").UInt(binary_verified)
        .Add("\nupdate_verified=").UInt(update_verified).Add("\nappend_verified=").UInt(append_verified)
        .Add("\nhole_verified=").UInt(hole_verified).Add("\nformatted_verified=").UInt(formatted_verified)
        .Add("\nxml_verified=").UInt(xml_verified).Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
        .Add("\nowned_allocations=").UInt(available ? uint32_t(stats.allocations) : UINT32_MAX)
        .Add("\nregistered_resources=").UInt(scriptResourceCount()).Add("\n");
    if (text.Complete()) (void)scriptSaveFile("result.txt", text.CStr(), text.Size());
}
void Audit(void*) {
    ShvNxMemoryStats stats{};
    cleanup_complete = scriptGetMemoryStats(&stats) && !stats.allocations &&
        !scriptResourceCount() && scriptTrimMemory();
    if (!cleanup_complete) ++failed;
    Report();
}
bool Initialize() {
    return scriptMemoryAvailable() && scriptFilesAvailable() && scriptResourcesAvailable() &&
        scriptFileResizeAvailable();
}
bool Workflow() {
    FILE* file = fopen("output.bin", "w+b");
    if (!file) return false;
    unsigned char buffer[777];
    for (uint32_t at = 0; at < Bytes;) {
        const size_t count = Bytes - at < sizeof(buffer) ? Bytes - at : sizeof(buffer);
        for (size_t i = 0; i < count; ++i) buffer[i] = Byte(at + uint32_t(i));
        if (fwrite(buffer, 1, count, file) != count) return false;
        at += uint32_t(count);
        WAIT(0);
    }
    if (fflush(file) || fseek(file, 0, SEEK_SET)) return false;
    for (uint32_t at = 0; at < Bytes;) {
        const size_t count = Bytes - at < sizeof(buffer) ? Bytes - at : sizeof(buffer);
        if (fread(buffer, 1, count, file) != count) return false;
        for (size_t i = 0; i < count; ++i) if (buffer[i] != Byte(at + uint32_t(i))) return false;
        at += uint32_t(count);
        WAIT(0);
    }
    binary_verified = 1;
    if (fseek(file, 17, SEEK_SET) || fwrite("edited", 2, 3, file) != 3 ||
        fseek(file, Bytes + Hole, SEEK_SET) || fputc(255, file) != 255 ||
        fputs("tail", file) < 0 || fclose(file)) return false;
    file = fopen("output.bin", "a+b");
    if (!file || ftell(file) != 0 || fseek(file, 2, SEEK_SET) ||
        fwrite("NX", 1, 2, file) != 2 || ftell(file) != Bytes + Hole + 7 ||
        fseek(file, 0, SEEK_SET)) return false;
    uint32_t at = 0;
    const unsigned char suffix[] = {255, 't', 'a', 'i', 'l', 'N', 'X'};
    while (const size_t got = fread(buffer, 1, sizeof(buffer), file)) {
        for (size_t i = 0; i < got; ++i, ++at) {
            unsigned char expected = 0;
            if (at >= Bytes + Hole) {
                if (at - (Bytes + Hole) >= sizeof(suffix)) return false;
                expected = suffix[at - (Bytes + Hole)];
            } else if (at < Bytes) expected = at >= 17 && at < 23 ? static_cast<unsigned char>("edited"[at - 17]) : Byte(at);
            if (buffer[i] != expected) return false;
        }
        WAIT(0);
    }
    if (at != Bytes + Hole + 7 || !feof(file) || ferror(file) || fclose(file)) return false;
    update_verified = append_verified = hole_verified = 1;
    FILE* log = fopen("formatted.txt", "w");
    if (!log || fprintf(log, "build=%u hex=%08x %s\n", 2699u, 255u, "complete") != 33 ||
        fflush(nullptr) || fclose(log)) return false;
    log = fopen("formatted.txt", "r");
    char line[64];
    if (!log || !fgets(line, sizeof(line), log) || strcmp(line, "build=2699 hex=000000ff complete\n") || fclose(log)) return false;
    formatted_verified = 1;
    {
        pugi::xml_document document;
        document.append_child("settings").append_child("value").text().set(2699);
        if (!document.save_file("direct.xml")) return false;
    }
    {
        pugi::xml_document document;
        if (!document.load_file("direct.xml") || document.child("settings").child("value").text().as_int() != 2699) return false;
    }
    xml_verified = 1;
    // Successful output is already written; final owner cleanup only frees the
    // unclosed stream object and never performs delayed writes or publication.
    file = fopen("retained.txt", "w");
    if (!file || fputs("committed\n", file) < 0) return false;
    WAIT(0);
    return true;
}
void Main() {
    ++run;
    failed = completed = binary_verified = update_verified = append_verified =
        hole_verified = formatted_verified = xml_verified = 0;
    cleanup_complete = false;
    if (!scriptResourceRegister(Audit, nullptr)) { ++failed; Report(); return; }
    if (!Workflow()) { ++failed; return; }
    completed = 1;
}
}
SHV_NX_PLUGIN_WITH_INIT("write-streams-example", Main, Initialize)
