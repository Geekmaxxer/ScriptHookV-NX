#include "script.h"

namespace {
constexpr char Path[] = "catalog/sample.bin";
constexpr size_t Chunk = 32768;
constexpr uint64_t Length = 256 * 1024;
uint8_t buffer[Chunk];
ShvNxDirectoryEntry entries[4];

bool Initialize() { return scriptFilesAvailable() && scriptCatalogAvailable(); }

void Report(bool passed) {
    constexpr char success[] = "verified_bytes=262144\nchunk_bytes=32768\ncatalog_verified=1\nfailed=0\n";
    constexpr char failure[] = "failed=1\nexisting_sample_preserved=1\n";
    if (passed) (void)scriptSaveFile("result.txt", success, sizeof(success) - 1);
    else (void)scriptSaveFile("result.txt", failure, sizeof(failure) - 1);
}

void Main() {
    if (scriptDirectoryCreate("catalog") != ShvNxFile_Ok) { Report(false); return; }
    const auto created = scriptFileCreate(Path, Length);
    if (created != ShvNxFile_Ok && created != ShvNxFile_AlreadyExists) { Report(false); return; }
    uint64_t bytes = 0;
    if (scriptFileSize(Path, &bytes) != ShvNxFile_Ok || bytes != Length) { Report(false); return; }
    for (uint64_t offset = 0; offset < Length; offset += Chunk) {
        if (created == ShvNxFile_Ok) {
            for (size_t i = 0; i < Chunk; ++i) buffer[i] = uint8_t((offset + i) * 37 + 11);
            if (scriptFileWriteAt(Path, offset, buffer, Chunk) != ShvNxFile_Ok) { Report(false); return; }
        }
        size_t got = 0;
        if (scriptFileReadAt(Path, offset, buffer, Chunk, &got) != ShvNxFile_Ok || got != Chunk) { Report(false); return; }
        for (size_t i = 0; i < Chunk; ++i) {
            if (buffer[i] != uint8_t((offset + i) * 37 + 11)) { Report(false); return; }
        }
        WAIT(0);
    }
    bool found = false;
    uint32_t offset = 0;
    do {
        ShvNxDirectoryPage page{};
        if (scriptDirectoryList("catalog", offset, entries, 4, &page) != ShvNxFile_Ok) { Report(false); return; }
        for (uint32_t i = 0; i < page.count; ++i) {
            constexpr char expected[] = "sample.bin";
            size_t at = 0;
            while (expected[at] && expected[at] == entries[i].name[at]) ++at;
            if (!expected[at] && !entries[i].name[at]) {
                found = entries[i].kind == SHV_NX_CATALOG_FILE && entries[i].bytes == Length;
            }
        }
        if (page.next_offset && page.next_offset <= offset) { Report(false); return; }
        offset = page.next_offset;
        WAIT(0);
    } while (offset);
    Report(found);
}
}
SHV_NX_PLUGIN_WITH_INIT("streamed-catalog-example", Main, Initialize)
