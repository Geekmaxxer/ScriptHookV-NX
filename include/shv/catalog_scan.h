#pragma once
#include "../plugin_catalog.h"
#include "containers.h"

namespace ScriptHook::Data {

enum class CatalogState : uint32_t { Idle, Listing, Sorting, Checking, Complete, Failed, Cancelled };
enum class CatalogError : uint32_t { None, InvalidArgument, Unavailable, FileError, EntryLimit, NoMemory, InvalidPage, DuplicateName };
struct CatalogRecord {
    uint64_t bytes = 0;
    uint32_t kind = 0;
    char name[32]{};
};

// One page or one heap-sort/check operation per Step. The vector remains private
// until listing, sorting and duplicate checks complete. No file handles persist.
class CatalogScanTask final {
public:
    CatalogScanTask() noexcept = default;
    CatalogScanTask(const CatalogScanTask&) = delete;
    CatalogScanTask& operator=(const CatalogScanTask&) = delete;

    bool Begin(const char* path, size_t maximum = 128, const char* extension = nullptr,
            bool include_directories = true) noexcept {
        if (state_ != CatalogState::Idle) return false;
        if (!maximum || maximum > SHV_NX_CATALOG_MAX_ENTRIES || !CopyPath(path) || !CopyExtension(extension))
            return Fail(CatalogError::InvalidArgument,ShvNxFile_InvalidArgument);
        if (!scriptCatalogAvailable() || !scriptMemoryAvailable())
            return Fail(CatalogError::Unavailable,ShvNxFile_Unavailable);
        maximum_ = maximum;
        include_directories_ = include_directories;
        state_ = CatalogState::Listing;
        return true;
    }
    CatalogState Step() noexcept {
        if (state_ == CatalogState::Listing) ListPage();
        else if (state_ == CatalogState::Sorting) {
            if (build_) Sift(--build_,end_);
            else if (end_ > 1) { Swap(0,--end_); Sift(0,end_); }
            else { checking_ = 1; state_ = CatalogState::Checking; }
        } else if (state_ == CatalogState::Checking) {
            if (checking_ >= records_.Size()) state_ = CatalogState::Complete;
            else if (Compare(records_.At(checking_-1)->name,records_.At(checking_)->name) == 0)
                (void)Fail(CatalogError::DuplicateName,ShvNxFile_InvalidEntry);
            else ++checking_;
        }
        return state_;
    }
    void Reset() noexcept {
        records_.Reset();
        path_[0] = extension_[0] = 0;
        maximum_ = offset_ = pages_ = scanned_ = skipped_ = build_ = end_ = checking_ = 0;
        state_ = CatalogState::Idle; error_ = CatalogError::None; file_status_ = ShvNxFile_Ok;
    }
    void Cancel() noexcept { Reset(); state_ = CatalogState::Cancelled; }
    CatalogState State() const noexcept { return state_; }
    CatalogError Error() const noexcept { return error_; }
    ShvNxFileStatus FileStatus() const noexcept { return file_status_; }
    size_t Count() const noexcept { return state_ == CatalogState::Complete ? records_.Size() : 0; }
    size_t Collected() const noexcept { return records_.Size(); }
    size_t Pages() const noexcept { return pages_; }
    size_t Scanned() const noexcept { return scanned_; }
    size_t Skipped() const noexcept { return skipped_; }
    const CatalogRecord* At(size_t index) const noexcept {
        return state_ == CatalogState::Complete ? records_.At(index) : nullptr;
    }
    bool BuildPath(size_t index, char* output, size_t capacity) const noexcept {
        if (!output || !capacity) return false;
        output[0] = 0;
        const auto* record = At(index);
        if (!record) return false;
        const size_t parent = Length(path_), name = Length(record->name);
        const size_t required = parent+size_t(parent != 0)+name;
        if (capacity <= required) return false;
        size_t at = 0;
        for (size_t i = 0; i < parent; ++i) output[at++] = path_[i];
        if (parent) output[at++] = '/';
        for (size_t i = 0; i < name; ++i) output[at++] = record->name[i];
        output[at] = 0;
        return true;
    }

private:
    ScriptHook::Containers::Vector<CatalogRecord> records_;
    ShvNxDirectoryEntry page_[4]{};
    char path_[SHV_NX_FILE_MAX_PATH+1]{}, extension_[16]{};
    size_t maximum_ = 0, offset_ = 0, pages_ = 0, scanned_ = 0, skipped_ = 0;
    size_t build_ = 0, end_ = 0, checking_ = 0;
    bool include_directories_ = true;
    CatalogState state_ = CatalogState::Idle;
    CatalogError error_ = CatalogError::None;
    ShvNxFileStatus file_status_ = ShvNxFile_Ok;

    static char Fold(char value) noexcept { return value >= 'A' && value <= 'Z' ? char(value+'a'-'A') : value; }
    static size_t Length(const char* value) noexcept { size_t n = 0; while (value[n]) ++n; return n; }
    static int Compare(const char* left, const char* right) noexcept {
        for (;;) {
            const char a = Fold(*left++), b = Fold(*right++);
            if (a != b) return a < b ? -1 : 1;
            if (!a) return 0;
        }
    }
    bool CopyPath(const char* input) noexcept {
        if (!input) return false;
        for (size_t i = 0; i <= SHV_NX_FILE_MAX_PATH; ++i) {
            const char value = input[i];
            if (!value) { path_[i] = 0; return true; }
            if (i == SHV_NX_FILE_MAX_PATH) return false;
            path_[i] = value;
        }
        return false;
    }
    bool CopyExtension(const char* input) noexcept {
        if (!input || !*input) return true;
        if (*input != '.') return false;
        for (size_t i = 0; i < sizeof(extension_); ++i) {
            const char value = input[i];
            if (!value) { extension_[i] = 0; return i > 1; }
            if (i+1 == sizeof(extension_)) return false;
            if (i && !((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                    (value >= '0' && value <= '9'))) return false;
            extension_[i] = Fold(value);
        }
        return false;
    }
    bool Matches(const char* name) const noexcept {
        const size_t n = Length(name), suffix = Length(extension_);
        return n >= suffix && Compare(name+n-suffix,extension_) == 0;
    }
    bool Addressable(const char* name) const noexcept {
        const size_t parent = Length(path_), leaf = Length(name);
        if (parent+size_t(parent != 0)+leaf > SHV_NX_FILE_MAX_PATH) return false;
        unsigned components = parent ? 2 : 1;
        for (size_t i = 0; i < parent; ++i) if (path_[i] == '/') ++components;
        return components <= 4;
    }
    void ListPage() noexcept {
        ShvNxDirectoryPage page{};
        const auto status = scriptDirectoryList(path_,uint32_t(offset_),page_,4,&page);
        if (status != ShvNxFile_Ok) { (void)Fail(CatalogError::FileError,status); return; }
        if (page.count > 4 || (page.next_offset && (!page.count || page.next_offset <= offset_ ||
                page.next_offset > SHV_NX_CATALOG_MAX_ENTRIES))) {
            (void)Fail(CatalogError::InvalidPage,ShvNxFile_InvalidEntry); return;
        }
        ++pages_;
        for (uint32_t i = 0; i < page.count; ++i) {
            const auto& entry = page_[i]; ++scanned_;
            if (entry.kind != SHV_NX_CATALOG_FILE && entry.kind != SHV_NX_CATALOG_DIRECTORY) {
                (void)Fail(CatalogError::InvalidPage,ShvNxFile_InvalidEntry); return;
            }
            CatalogRecord record; size_t length = 0;
            while (length < 31 && entry.name[length]) { record.name[length] = entry.name[length]; ++length; }
            if (!(entry.flags & SHV_NX_CATALOG_COMPATIBLE_NAME) || !length || entry.name[length] ||
                    !Addressable(record.name) || (entry.kind == SHV_NX_CATALOG_DIRECTORY ?
                        !include_directories_ : !Matches(record.name))) { ++skipped_; continue; }
            if (records_.Size() >= maximum_) { (void)Fail(CatalogError::EntryLimit,ShvNxFile_DirectoryLimit); return; }
            record.bytes = entry.bytes; record.kind = entry.kind;
            if (!records_.TryPushBack(record)) { (void)Fail(CatalogError::NoMemory,ShvNxFile_Ok); return; }
        }
        if (page.next_offset) offset_ = page.next_offset;
        else { build_ = records_.Size()/2; end_ = records_.Size(); state_ = CatalogState::Sorting; }
    }
    void Swap(size_t a, size_t b) noexcept {
        CatalogRecord saved = *records_.At(a); *records_.At(a) = *records_.At(b); *records_.At(b) = saved;
    }
    void Sift(size_t root, size_t count) noexcept {
        while (root < count/2) {
            size_t child = root*2+1;
            if (child+1 < count && Compare(records_.At(child)->name,records_.At(child+1)->name) < 0) ++child;
            if (Compare(records_.At(root)->name,records_.At(child)->name) >= 0) return;
            Swap(root,child); root = child;
        }
    }
    bool Fail(CatalogError error, ShvNxFileStatus status) noexcept {
        state_ = CatalogState::Failed; error_ = error; file_status_ = status;
        return false;
    }
};
}
