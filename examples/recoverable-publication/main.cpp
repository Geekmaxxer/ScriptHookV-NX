#include "script.h"
#include "shv/file_replacement.h"

namespace {
constexpr size_t Chunk = 16384;
constexpr uint64_t Length = 256 * 1024;
uint8_t buffer[Chunk];

bool Initialize() { return scriptFilesAvailable() && scriptFileMutationsAvailable(); }

void Report(bool ok) {
    constexpr char success[] = "verified_bytes=262144\nbackup_retained=1\nfailed=0\n";
    constexpr char failure[] = "failed=1\ninspection_required=1\n";
    if (ok) (void)scriptSaveFile("result.txt", success, sizeof(success) - 1);
    else (void)scriptSaveFile("result.txt", failure, sizeof(failure) - 1);
}

bool Verify(const char* path) {
    uint64_t bytes = 0;
    if (scriptFileSize(path, &bytes) != ShvNxFile_Ok || bytes != Length) return false;
    for (uint64_t offset = 0; offset < Length; offset += Chunk) {
        size_t got = 0;
        if (scriptFileReadAt(path, offset, buffer, Chunk, &got) != ShvNxFile_Ok || got != Chunk) return false;
        for (size_t i = 0; i < Chunk; ++i) {
            if (buffer[i] != uint8_t((offset + i) * 37 + 11)) return false;
        }
        WAIT(0);
    }
    return true;
}

bool VerifyBackup() {
    uint64_t bytes = 0;
    size_t got = 0;
    return scriptFileSize("backup.bin", &bytes) == ShvNxFile_Ok && bytes == 3 &&
        scriptFileReadAt("backup.bin", 0, buffer, 3, &got) == ShvNxFile_Ok && got == 3 &&
        buffer[0] == 'o' && buffer[1] == 'l' && buffer[2] == 'd';
}

void Main() {
    ScriptHook::Data::FileReplacement publication("current.bin", "prepared.bin", "backup.bin");
    ScriptHook::Data::ReplacementState state;
    if (publication.Inspect(state) != ShvNxFile_Ok) { Report(false); return; }
    if (state.present) {
        // A completed run is verification-only. Interrupted or unfamiliar
        // states are preserved for an explicit recovery decision.
        Report(state.present == (state.Current | state.Backup) && Verify("current.bin") && VerifyBackup());
        return;
    }
    constexpr char old[] = "old";
    if (scriptFileCreate("current.bin", 3) != ShvNxFile_Ok ||
        scriptFileWriteAt("current.bin", 0, old, 3) != ShvNxFile_Ok ||
        scriptFileCreate("prepared.bin", Length) != ShvNxFile_Ok) { Report(false); return; }
    for (uint64_t offset = 0; offset < Length; offset += Chunk) {
        for (size_t i = 0; i < Chunk; ++i) buffer[i] = uint8_t((offset + i) * 37 + 11);
        if (scriptFileWriteAt("prepared.bin", offset, buffer, Chunk) != ShvNxFile_Ok) { Report(false); return; }
        WAIT(0);
    }
    if (!Verify("prepared.bin")) { Report(false); return; }
    if (publication.Publish() != ShvNxFile_Ok) { Report(false); return; }
    Report(Verify("current.bin") && VerifyBackup());
}
}
SHV_NX_PLUGIN_WITH_INIT("publication-example", Main, Initialize)
