#include "script.h"
#include "shv/stdio.h"
#include "shv/xml.h"
#include "shv/text.h"

namespace {
uint32_t run, failed, binary_verified, text_verified, seek_verified, xml_verified, completed;
bool cleanup_complete;
constexpr uint32_t Bytes = 16387;
unsigned char Byte(uint32_t index) { return static_cast<unsigned char>(index * 43u + 255u); }

void Report() {
    ShvNxMemoryStats stats{};
    const bool available = scriptGetMemoryStats(&stats);
    ScriptHook::Util::TextBuffer<384> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\ncompleted=").UInt(completed).Add("\nbinary_verified=").UInt(binary_verified)
        .Add("\ntext_verified=").UInt(text_verified).Add("\nseek_verified=").UInt(seek_verified)
        .Add("\nxml_verified=").UInt(xml_verified)
        .Add("\nowner_cleanup_complete=").UInt(cleanup_complete)
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
bool Fixtures() {
    if (scriptDirectoryCreate("fixtures") != ShvNxFile_Ok) return false;
    uint64_t size = 0;
    const auto status = scriptFileSize("fixtures/data.bin", &size);
    if (status == ShvNxFile_NotFound) {
        if (scriptFileCreate("fixtures/data.bin", Bytes) != ShvNxFile_Ok) return false;
        unsigned char buffer[1024];
        for (uint32_t at = 0; at < Bytes; at += sizeof(buffer)) {
            const auto count = Bytes - at < sizeof(buffer) ? Bytes - at : sizeof(buffer);
            for (size_t i = 0; i < count; ++i) buffer[i] = Byte(at + uint32_t(i));
            if (scriptFileWriteAt("fixtures/data.bin", at, buffer, count) != ShvNxFile_Ok) return false;
            WAIT(0);
        }
    } else if (status != ShvNxFile_Ok || size != Bytes) return false;
    const char lines[] = "first\r\nsecond\nlast";
    const char xml[] = "<settings><value>2699</value></settings>";
    for (uint32_t i = 0; i < 2; ++i) {
        const char* path = i ? "fixtures/settings.xml" : "fixtures/lines.txt";
        const char* content = i ? xml : lines;
        const size_t bytes = i ? sizeof(xml) - 1 : sizeof(lines) - 1;
        const auto found = scriptFileSize(path, &size);
        if (found == ShvNxFile_NotFound) {
            if (scriptFileCreate(path, bytes) != ShvNxFile_Ok ||
                scriptFileWriteAt(path, 0, content, bytes) != ShvNxFile_Ok) return false;
        } else if (found != ShvNxFile_Ok || size != bytes) return false;
    }
    return true;
}
bool Initialize() { return scriptMemoryAvailable() && scriptFilesAvailable() && scriptResourcesAvailable(); }
void Main() {
    ++run;
    failed = completed = binary_verified = text_verified = seek_verified = xml_verified = 0;
    cleanup_complete = false;
    if (!scriptResourceRegister(Audit, nullptr)) { ++failed; Report(); return; }
    if (!Fixtures()) { ++failed; return; }
    FILE* input = fopen("fixtures/data.bin", "rb");
    if (!input) { ++failed; return; }
    unsigned char buffer[777];
    uint32_t total = 0;
    while (const size_t got = fread(buffer, 1, sizeof(buffer), input)) {
        for (size_t i = 0; i < got; ++i) if (buffer[i] != Byte(total + uint32_t(i))) { ++failed; return; }
        total += uint32_t(got);
        WAIT(0);
    }
    if (total != Bytes || !feof(input) || ferror(input)) { ++failed; return; }
    binary_verified = 1;
    if (fseek(input, -3, SEEK_END) || ftell(input) != Bytes - 3 || feof(input) ||
        fgetc(input) != Byte(Bytes - 3)) { ++failed; return; }
    rewind(input);
    if (ftell(input) || fgetc(input) != Byte(0) || fclose(input)) { ++failed; return; }
    seek_verified = 1;
    FILE* lines = fopen("fixtures/lines.txt", "r");
    if (!lines) { ++failed; return; }
    const char* expected[] = {"first\r\n", "second\n", "last"};
    char line[32];
    for (uint32_t i = 0; i < 3; ++i) {
        if (!fgets(line, sizeof(line), lines) || strcmp(line, expected[i])) { ++failed; return; }
        WAIT(0);
    }
    if (fgets(line, sizeof(line), lines) || !feof(lines) || fclose(lines)) { ++failed; return; }
    text_verified = 1;
    {
        pugi::xml_document document;
        if (!document.load_file("fixtures/settings.xml") ||
            document.child("settings").child("value").text().as_int() != 2699) { ++failed; return; }
    }
    xml_verified = 1;
    // Deliberately leave one buffered stream registered across a wait. The
    // runtime drains it before Audit on completion or whole-plugin Kill.
    FILE* retained = fopen("fixtures/data.bin", "rb");
    if (!retained || fgetc(retained) != Byte(0)) { ++failed; return; }
    WAIT(0);
    completed = 1;
}
}
SHV_NX_PLUGIN_WITH_INIT("read-streams-example", Main, Initialize)
