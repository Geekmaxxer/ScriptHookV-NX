#pragma once
#include "../plugin_file_mutations.h"

namespace ScriptHook::Data {

struct ReplacementState {
    static constexpr uint32_t Current = 1, Prepared = 2, Backup = 4;
    uint32_t present = 0;
    uint64_t current_bytes = 0, prepared_bytes = 0, backup_bytes = 0;
};

// Application-validated, single-writer publication. Files are never removed
// automatically. Inspect again after every failed mutation before retrying.
class FileReplacement {
public:
    FileReplacement(const char* current, const char* prepared, const char* backup) noexcept {
        valid_ = Copy(current, paths_[0]) && Copy(prepared, paths_[1]) && Copy(backup, paths_[2]);
        valid_ = valid_ && !Equal(paths_[0], paths_[1]) && !Equal(paths_[0], paths_[2]) && !Equal(paths_[1], paths_[2]);
    }

    ShvNxFileStatus Inspect(ReplacementState& state) const noexcept {
        state = {};
        if (!valid_) return ShvNxFile_InvalidArgument;
        if (!scriptFilesAvailable()) return ShvNxFile_Unavailable;
        ReplacementState observed;
        uint64_t* sizes[] = {&observed.current_bytes, &observed.prepared_bytes, &observed.backup_bytes};
        for (unsigned i = 0; i < 3; ++i) {
            const auto status = scriptFileSize(paths_[i], sizes[i]);
            if (status == ShvNxFile_Ok) observed.present |= 1u << i;
            else if (status != ShvNxFile_NotFound) return status;
        }
        state = observed;
        return ShvNxFile_Ok;
    }

    // Call only after verifying prepared data. An existing backup blocks
    // publication until the application explicitly resolves its ownership.
    ShvNxFileStatus Publish() const noexcept {
        if (!valid_) return ShvNxFile_InvalidArgument;
        if (!scriptFileMutationsAvailable()) return ShvNxFile_Unavailable;
        ReplacementState state;
        const auto status = Inspect(state);
        if (status != ShvNxFile_Ok) return status;
        if (!(state.present & ReplacementState::Prepared)) return ShvNxFile_NotFound;
        if (state.present & ReplacementState::Backup) return ShvNxFile_AlreadyExists;
        if (state.present & ReplacementState::Current) {
            const auto saved = scriptFileRename(paths_[0], paths_[2]);
            if (saved != ShvNxFile_Ok) return saved;
        }
        return scriptFileRename(paths_[1], paths_[0]);
    }

    // Explicit recovery authorization. An existing current file is preserved,
    // even when its contents require application-specific validation.
    ShvNxFileStatus RestoreBackup() const noexcept {
        if (!valid_) return ShvNxFile_InvalidArgument;
        if (!scriptFileMutationsAvailable()) return ShvNxFile_Unavailable;
        ReplacementState state;
        const auto status = Inspect(state);
        if (status != ShvNxFile_Ok) return status;
        if (state.present & ReplacementState::Current) return ShvNxFile_AlreadyExists;
        if (!(state.present & ReplacementState::Backup)) return ShvNxFile_NotFound;
        return scriptFileRename(paths_[2], paths_[0]);
    }

private:
    char paths_[3][SHV_NX_FILE_MAX_PATH + 1]{};
    bool valid_ = false;

    static bool Equal(const char* a, const char* b) noexcept {
        for (;;) {
            char left = *a++, right = *b++;
            if (left >= 'A' && left <= 'Z') left += 'a' - 'A';
            if (right >= 'A' && right <= 'Z') right += 'a' - 'A';
            if (left != right) return false;
            if (!left) return true;
        }
    }

    static bool Copy(const char* input, char (&output)[SHV_NX_FILE_MAX_PATH + 1]) noexcept {
        if (!input) return false;
        size_t component = 0;
        unsigned components = 1;
        char previous = 0;
        for (size_t i = 0; i <= SHV_NX_FILE_MAX_PATH; ++i) {
            const char c = input[i];
            if (!c) { output[i] = 0; return component && previous != '.'; }
            if (i == SHV_NX_FILE_MAX_PATH) return false;
            if (c == '/') {
                if (!component || previous == '.' || ++components > 4) return false;
                component = 0;
            } else {
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
                if (++component > 31 || (c == '.' && (component == 1 || previous == '.'))) return false;
            }
            output[i] = c;
            previous = c;
        }
        return false;
    }
};
}
