#include "script.h"
#include "shv/json_file.h"
#include "shv/file_verify.h"
#include "shv/file_replacement.h"
#include "shv/managed.h"
#include "shv/text.h"

using namespace ScriptHook;
namespace {
uint32_t run, failed, stage, completed, parsed, serialized, publication_verified, restored;
bool cleanup_complete;
constexpr size_t Chunk = 512, PayloadBytes = 8192;
struct State {
    Json::FileLoadTask load;
    Json::Document initial, edited;
    Json::Buffer original, output;
    Data::PreparedFileWriteTask writer;
    Data::FileVerifyTask verify;
    char payload[PayloadBytes + 1]{};

    bool Fail() { ++failed; return true; }
    int WriteStep(const Json::Buffer& buffer) {
        const size_t at = size_t(writer.Transferred());
        if (at < buffer.Bytes()) {
            const size_t count = buffer.Bytes() - at < Chunk ? buffer.Bytes() - at : Chunk;
            return writer.Append(buffer.Text() + at, count) == Data::TransferState::Failed ? -1 : 0;
        }
        return writer.Finish() == Data::TransferState::Complete ? 1 : -1;
    }
    int VerifyStep(const char* path, const Json::Buffer& buffer) {
        if (verify.State() == Data::TransferState::Idle)
            return verify.Begin(path, buffer.Bytes(), Chunk) ? 0 : -1;
        const size_t at = size_t(verify.Transferred());
        if (at < buffer.Bytes()) {
            const size_t count = buffer.Bytes() - at < Chunk ? buffer.Bytes() - at : Chunk;
            return verify.Compare(buffer.Text() + at, count) == Data::TransferState::Failed ? -1 : 0;
        }
        return verify.Finish() == Data::TransferState::Complete ? 1 : -1;
    }
    bool Step() {
        Data::FileReplacement files("current.json", "prepared.json", "backup.json");
        if (stage == 0) {
            for (size_t i = 0; i < PayloadBytes; ++i) payload[i] = char('a' + i % 26);
            uint64_t bytes = 0;
            const auto found = scriptFileSize("current.json", &bytes);
            if (found == ShvNxFile_Ok) { stage = 2; return false; }
            if (found != ShvNxFile_NotFound || !initial.CreateObject(256 * 1024) ||
                !initial.SetUInt("version", 2699) || !initial.SetBool("enabled", true) ||
                !initial.SetReal("ratio", 1.25) || !initial.SetString("label", "日本語 中文", strlen("日本語 中文")) ||
                !initial.SetString("payload", payload, PayloadBytes) ||
                !original.Serialize(initial, 32768, 256 * 1024) ||
                !writer.Begin("current.json", original.Bytes())) return Fail();
            stage = 1;
        } else if (stage == 1) {
            const int result = WriteStep(original);
            if (result < 0) return Fail();
            if (result) { writer.Reset(); stage = 2; }
        } else if (stage == 2) {
            if (!load.Begin("current.json", 32768, 256 * 1024, Chunk)) return Fail();
            stage = 3;
        } else if (stage == 3) {
            const auto result = load.Step();
            if (result == Json::LoadState::Failed) return Fail();
            if (result != Json::LoadState::Complete) return false;
            const auto* root = load.Current().ReadRoot();
            const auto* version = yyjson_obj_get(root, "version");
            const auto* text = yyjson_obj_get(root, "payload");
            const auto* label = yyjson_obj_get(root, "label");
            if (!yyjson_is_uint(version) || yyjson_get_uint(version) != 2699 ||
                !yyjson_is_true(yyjson_obj_get(root, "enabled")) ||
                yyjson_get_real(yyjson_obj_get(root, "ratio")) != 1.25 ||
                !yyjson_is_str(text) || yyjson_get_len(text) != PayloadBytes || memcmp(yyjson_get_str(text), payload, PayloadBytes) ||
                !yyjson_get_str(label) || strcmp(yyjson_get_str(label), "日本語 中文") ||
                !original.Serialize(load.Current(), 32768, 256 * 1024)) return Fail();
            (void)initial.Reset(); parsed = 1; stage = 4;
        } else if (stage == 4) {
            const int result = VerifyStep("current.json", original);
            if (result < 0) return Fail();
            if (!result) return false;
            if (!verify.Reset() || !edited.CopyFrom(load.Current(), 256 * 1024) ||
                !edited.SetUInt("version", 2700) || !edited.SetBool("enabled", false) ||
                !edited.SetReal("ratio", 2.5) || !edited.SetString("status", "edited", 6) ||
                !output.Serialize(edited, 32768, 256 * 1024)) return Fail();
            serialized = 1; stage = 5;
        } else if (stage == 5) {
            if (!writer.Begin("prepared.json", output.Bytes())) return Fail();
            stage = 6;
        } else if (stage == 6) {
            const int result = WriteStep(output);
            if (result < 0) return Fail();
            if (result) { writer.Reset(); stage = 7; }
        } else if (stage == 7) {
            const int result = VerifyStep("prepared.json", output);
            if (result < 0) return Fail();
            if (!result) return false;
            if (!verify.Reset() || files.Publish() != ShvNxFile_Ok) return Fail();
            stage = 8;
        } else if (stage == 8) {
            const int result = VerifyStep("current.json", output);
            if (result < 0) return Fail();
            if (result) { if (!verify.Reset()) return Fail(); publication_verified = 1; stage = 9; }
        } else if (stage == 9) {
            const int result = VerifyStep("backup.json", original);
            if (result < 0) return Fail();
            if (!result) return false;
            if (!verify.Reset() || scriptFileDelete("current.json") != ShvNxFile_Ok ||
                files.RestoreBackup() != ShvNxFile_Ok) return Fail();
            stage = 10;
        } else if (stage == 10) {
            const int result = VerifyStep("current.json", original);
            if (result < 0) return Fail();
            if (result) { restored = completed = 1; return true; }
        }
        return false;
    }
};
void Report() {
    ShvNxMemoryStats stats{};
    const bool available = scriptGetMemoryStats(&stats);
    Util::TextBuffer<384> text;
    text.Add("run=").UInt(run).Add("\nfailed=").UInt(failed)
        .Add("\nstage=").UInt(stage).Add("\ncompleted=").UInt(completed)
        .Add("\nparsed=").UInt(parsed).Add("\nserialized=").UInt(serialized)
        .Add("\npublication_verified=").UInt(publication_verified).Add("\nrestored=").UInt(restored)
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
bool Initialize() { return scriptMemoryAvailable() && scriptResourcesAvailable() && scriptFilesAvailable() && scriptFileMutationsAvailable(); }
void Main() {
    ++run; failed = stage = completed = parsed = serialized = publication_verified = restored = 0;
    cleanup_complete = false;
    if (!scriptResourceRegister(Audit, nullptr)) { ++failed; Report(); return; }
    auto owner = Memory::Managed<State>::TryMake();
    if (!owner) { ++failed; return; }
    for (;;) {
        { auto* state = owner.Get(); if (!state) { ++failed; return; } if (state->Step()) return; }
        WAIT(0);
    }
}
}
SHV_NX_PLUGIN_WITH_INIT("json-presets-example", Main, Initialize)
