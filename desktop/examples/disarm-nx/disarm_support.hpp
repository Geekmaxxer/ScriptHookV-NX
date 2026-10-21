#pragma once
#include <script.h>
#include <shv/time.h>
#include <shv/text.h>
#include <shv/world.h>
#include "native_requirements.hpp"

// Switch behavior port of GTAVDisarm by ikt32. This implementation replaces
// PC pool access with a bounded scanner and validates handles/output values.
namespace DisarmNx {
inline constexpr int32_t RightHand = 57005;
inline constexpr int32_t RightForearm = 28252;
inline constexpr int32_t RightUpperArm = 40269;
inline constexpr int32_t LeftUpperArm = 45509;
inline constexpr int32_t LeftForearm = 61163;
inline constexpr uint32_t ScanCapacity = 32, ChecksPerFrame = 8, RefreshMs = 250;
static_assert(ChecksPerFrame >= 1 && ChecksPerFrame <= ScanCapacity);

struct Statistics {
    uint32_t frames = 0, queries = 0, queryFailures = 0, pedChecks = 0;
    uint32_t disarms = 0, handHits = 0, armHits = 0, weaponHits = 0, nativeFailures = 0;
};

class Controller {
    ScriptHook::World::NearbyPeds<ScanCapacity> nearby;
    ScriptHook::Util::Every scanTimer, reportTimer;
    Statistics stats;
    Ped anchor = 0;
    uint32_t cursor = 0, lastReportedDisarms = 0, lastReportedFailures = 0;
    bool ready = false, firstDisarmReported = false;

    bool NativeOk() {
        if (getNativeCallStatus() == NativeCall_Ok) return true;
        ++stats.nativeFailures;
        return false;
    }
    void Report(const char* state) {
        ScriptHook::Util::TextBuffer<640> text;
        text.Add("Disarm NX test 2\nstate=").Add(state)
            .Add("\nscan_capacity=").UInt(ScanCapacity).Add("\nchecks_per_frame=").UInt(ChecksPerFrame)
            .Add("\nframes=").UInt(stats.frames).Add("\nqueries=").UInt(stats.queries)
            .Add("\nquery_failures=").UInt(stats.queryFailures).Add("\nped_checks=").UInt(stats.pedChecks)
            .Add("\ndisarms=").UInt(stats.disarms).Add("\nhand_hits=").UInt(stats.handHits)
            .Add("\narm_hits=").UInt(stats.armHits).Add("\nweapon_hits=").UInt(stats.weaponHits).Add("\nnative_failures=").UInt(stats.nativeFailures).Add("\n");
        if (text.Complete()) (void)scriptSaveFile("status.txt", text.CStr(), text.Size());
        lastReportedDisarms = stats.disarms;
        lastReportedFailures = stats.nativeFailures + stats.queryFailures;
    }
    void ResetScanner() {
        nearby.Clear(); scanTimer.Reset(); anchor = 0; cursor = 0;
    }
    void CheckPed(Ped ped, Ped playerPed) {
        if (!ped || ped == playerPed) return;
        ++stats.pedChecks;
        const bool exists = ENTITY::DOES_ENTITY_EXIST(ped);
        if (!NativeOk() || !exists) return;
        const bool dead = ENTITY::IS_ENTITY_DEAD(ped, false);
        if (!NativeOk() || dead) return;
        const Entity weapon = WEAPON::GET_CURRENT_PED_WEAPON_ENTITY_INDEX(ped, true);
        if (!NativeOk() || !weapon) return;
        const bool weaponExists = ENTITY::DOES_ENTITY_EXIST(weapon);
        if (!NativeOk() || !weaponExists) return;

        int32_t bone = 0;
        const bool hasBone = PED::GET_PED_LAST_DAMAGE_BONE(ped, &bone);
        if (!NativeOk()) return;
        const bool handHit = hasBone && bone == RightHand;
        const bool armHit = hasBone && (bone == RightUpperArm || bone == RightForearm || bone == LeftUpperArm || bone == LeftForearm);
        const bool weaponHit = ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ANY_PED(weapon);
        if (!NativeOk()) return;

        // Consume the reported damage before dropping: the weapon object may
        // change during the drop. Never read or clear the old object afterward.
        if (hasBone) { PED::CLEAR_PED_LAST_DAMAGE_BONE(ped); if (!NativeOk()) return; }
        if (weaponHit) { ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(weapon); if (!NativeOk()) return; }
        if (handHit || armHit || weaponHit) {
            WEAPON::SET_PED_DROPS_WEAPON(ped);
            if (!NativeOk()) return;
            ++stats.disarms;
            if (handHit) ++stats.handHits;
            if (armHit) ++stats.armHits;
            if (weaponHit) ++stats.weaponHits;
        }
    }
public:
    bool Start() {
        ready = false;
        for (uint64_t hash : RequiredNatives) {
            if (!nativeIsAvailable(hash)) { Report("required-native-unavailable"); return false; }
        }
        ready = true;
        HUD::BEGIN_TEXT_COMMAND_PRINT("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Disarm NX active: shoot an NPC's weapon, hand or either arm.");
        HUD::END_TEXT_COMMAND_PRINT(6000, true);
        Report("running");
        return true;
    }
    void Tick() {
        if (!ready) return;
        ++stats.frames;
        const uint32_t now = static_cast<uint32_t>(MISC::GET_GAME_TIMER());
        if (!NativeOk()) return;
        const Player player = PLAYER::PLAYER_ID();
        if (!NativeOk()) { ResetScanner(); return; }
        const Ped playerPed = PLAYER::PLAYER_PED_ID();
        if (!NativeOk() || !playerPed) { ResetScanner(); return; }
        const bool exists = ENTITY::DOES_ENTITY_EXIST(playerPed);
        if (!NativeOk() || !exists) { ResetScanner(); return; }
        const bool dead = ENTITY::IS_ENTITY_DEAD(playerPed, false);
        if (!NativeOk() || dead) { ResetScanner(); return; }
        const bool control = PLAYER::IS_PLAYER_CONTROL_ON(player);
        if (!NativeOk() || !control) { ResetScanner(); return; }
        const bool arrested = PLAYER::IS_PLAYER_BEING_ARRESTED(player, true);
        if (!NativeOk() || arrested) { ResetScanner(); return; }

        if (anchor != playerPed) { ResetScanner(); anchor = playerPed; }
        if (scanTimer.Ready(now, RefreshMs)) {
            ++stats.queries;
            if (!nearby.Refresh(playerPed)) ++stats.queryFailures;
            cursor = 0;
        }
        const uint32_t count = nearby.Size();
        const uint32_t budget = count < ChecksPerFrame ? count : ChecksPerFrame;
        for (uint32_t i = 0; i < budget; ++i) {
            const Ped ped = nearby[cursor];
            cursor = (cursor + 1u) % count;
            CheckPed(ped, playerPed);
        }
        // One report at the first successful drop, then at most once per 30s
        // and only when disarm/failure counters changed. No per-frame SD writes.
        if (!firstDisarmReported && stats.disarms) { firstDisarmReported = true; Report("running"); }
        if (reportTimer.Ready(now, 30000) && (lastReportedDisarms != stats.disarms ||
            lastReportedFailures != stats.nativeFailures + stats.queryFailures)) Report("running");
    }
    const Statistics& Stats() const { return stats; }
};
}
