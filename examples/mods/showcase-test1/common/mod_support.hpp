#pragma once
#include "script.h"
#include "native_requirements.hpp"

namespace Showcase {
inline bool Due(uint32_t now, uint32_t deadline) { return int32_t(now - deadline) >= 0; }
inline uint32_t Now() { return uint32_t(MISC::GET_GAME_TIMER()); }
inline bool NativeOk() { return getNativeCallStatus() == NativeCall_Ok; }

class TextLine {
    char value[192]{};
    size_t used = 0;
public:
    TextLine& Add(const char* text) {
        while (*text && used + 1 < sizeof(value)) value[used++] = *text++;
        value[used] = 0; return *this;
    }
    TextLine& Number(uint32_t number) {
        char reversed[10]; size_t count = 0;
        do { reversed[count++] = char('0' + number % 10); number /= 10; } while (number);
        while (count && used + 1 < sizeof(value)) value[used++] = reversed[--count];
        value[used] = 0; return *this;
    }
    TextLine& Fixed1(float number) {
        if (!(number >= 0.0f && number <= 1000000.0f)) return Add("--");
        const uint32_t tenths = uint32_t(number * 10.0f + 0.5f);
        return Number(tenths / 10).Add(".").Number(tenths % 10);
    }
    const char* CStr() const { return value; }
};
inline bool SaveStatus(const char* state) {
    TextLine line; line.Add("ScriptHookV-NX Showcase Test 1\nstate=").Add(state).Add("\n");
    size_t size = 0; while (line.CStr()[size]) ++size;
    return scriptSaveFile("status.txt", line.CStr(), size);
}
inline bool Start() {
    if (getNativeCallerAbiVersion() != 1 || getScriptRuntimeAbiVersion() != 1) {
        SaveStatus("unsupported_abi"); return false;
    }
    for (uint64_t hash : RequiredNatives) if (!nativeIsAvailable(hash)) {
        SaveStatus("missing_required_native"); return false;
    }
    return SaveStatus("ready");
}
inline bool Visible() {
    if (HUD::IS_PAUSE_MENU_ACTIVE() || CAMERA::IS_SCREEN_FADED_OUT() || CUTSCENE::IS_CUTSCENE_ACTIVE()) return false;
    const Ped ped = PLAYER::PLAYER_PED_ID();
    return ped && ENTITY::DOES_ENTITY_EXIST(ped) && !ENTITY::IS_ENTITY_DEAD(ped, true);
}
inline Vehicle PlayerVehicle(bool driverOnly = false) {
    const Ped ped = PLAYER::PLAYER_PED_ID();
    if (!ped || !ENTITY::DOES_ENTITY_EXIST(ped) || ENTITY::IS_ENTITY_DEAD(ped, true)) return 0;
    const Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    if (!vehicle || !ENTITY::DOES_ENTITY_EXIST(vehicle)) return 0;
    if (driverOnly && VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, false) != ped) return 0;
    return vehicle;
}

enum class Direction { Up = 188, Left = 189, Right = 190 };
inline bool Pressed(int action) {
    return PAD::IS_CONTROL_PRESSED(2, action) || PAD::IS_DISABLED_CONTROL_PRESSED(2, action);
}
inline bool Hotkey(Direction direction) {
    // CONTROL_ACTION values are derived from the pinned dev_ng commands_pad.sch.
    // Only suppress related gameplay actions while BOTH shoulders are held.
    if (!Pressed(205) || !Pressed(206)) return false;
    const int suppressed[] = {27, 37, 44, 68, 76, 85};
    for (int action : suppressed) PAD::DISABLE_CONTROL_ACTION(0, action, true);
    const int key = static_cast<int>(direction);
    const int scriptKey = direction == Direction::Up ? 232 : direction == Direction::Left ? 234 : 235;
    PAD::DISABLE_CONTROL_ACTION(0, scriptKey, true);
    PAD::DISABLE_CONTROL_ACTION(2, key, true);
    return PAD::IS_CONTROL_JUST_PRESSED(2, key) || PAD::IS_DISABLED_CONTROL_JUST_PRESSED(2, key);
}

struct Color { int r, g, b; };
inline void DrawLine(float x, float y, float width, const char* text, Color accent = {80, 210, 255}) {
    GRAPHICS::DRAW_RECT(x + width / 2.0f, y + 0.020f, width, 0.043f, 9, 15, 22, 205, false);
    GRAPHICS::DRAW_RECT(x + 0.002f, y + 0.020f, 0.003f, 0.043f, accent.r, accent.g, accent.b, 240, false);
    HUD::SET_TEXT_FONT(0);
    HUD::SET_TEXT_SCALE(0.31f, 0.31f);
    HUD::SET_TEXT_COLOUR(238, 246, 251, 255);
    HUD::SET_TEXT_CENTRE(false);
    HUD::SET_TEXT_RIGHT_JUSTIFY(false);
    HUD::SET_TEXT_WRAP(x + 0.008f, x + width - 0.004f);
    HUD::SET_TEXT_DROPSHADOW(1, 0, 0, 0, 160);
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text);
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x + 0.008f, y + 0.003f, 0);
}

class SavedWords {
    struct Record { uint32_t magic, version, generation, count, words[4], crc; };
    static_assert(sizeof(Record) == 36);
    uint32_t magic, generation = 0, selected = 1;
    static uint32_t Checksum(const Record& record) {
        uint32_t crc = ~0u;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&record);
        for (size_t i = 0; i < offsetof(Record, crc); ++i) {
            crc ^= bytes[i];
            for (unsigned j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & uint32_t(0 - int32_t(crc & 1)));
        }
        return ~crc;
    }
public:
    explicit SavedWords(uint32_t tag) : magic(tag) {}
    bool Load(uint32_t* words, size_t count) {
        if (!words || !count || count > 4) return false;
        Record records[2]{}; bool valid[2]{};
        const char* names[] = {"settings-a.bin", "settings-b.bin"};
        for (unsigned i = 0; i < 2; ++i) {
            size_t bytes = 0;
            valid[i] = scriptReadFile(names[i], &records[i], sizeof(Record), &bytes) && bytes == sizeof(Record) &&
                records[i].magic == magic && records[i].version == 1 && records[i].count == count && records[i].crc == Checksum(records[i]);
        }
        if (!valid[0] && !valid[1]) return false;
        selected = valid[1] && (!valid[0] || int32_t(records[1].generation - records[0].generation) > 0) ? 1 : 0;
        generation = records[selected].generation;
        for (size_t i = 0; i < count; ++i) words[i] = records[selected].words[i];
        return true;
    }
    bool Save(const uint32_t* words, size_t count) {
        if (!words || !count || count > 4) return false;
        Record record{}; record.magic = magic; record.version = 1; record.generation = generation + 1; record.count = uint32_t(count);
        for (size_t i = 0; i < count; ++i) record.words[i] = words[i];
        record.crc = Checksum(record);
        const uint32_t next = selected ^ 1;
        if (!scriptSaveFile(next ? "settings-b.bin" : "settings-a.bin", &record, sizeof(record))) return false;
        generation = record.generation; selected = next; return true;
    }
};
} // namespace Showcase
