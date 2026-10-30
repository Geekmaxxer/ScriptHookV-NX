// Host simulation of the real C++ port and SDK wrappers; no game code/assets.
#include "../examples/disarm-nx/disarm_support.hpp"
#include <shv/input.h>
#include <shv/ini.h>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdexcept>

namespace Test {
struct Actor { bool dead = false, hasBone = false; int32_t bone = 0; Entity weapon = 0; bool weaponHit = false; };
struct Simulation {
    std::map<Ped, Actor> actors;
    std::vector<Ped> nearby;
    std::set<int32_t> buttons;
    std::set<Entity> weapons;
    std::vector<std::string> events, reports;
    Ped player = 100, lastAnchor = 0;
    uint32_t now = 0, queries = 0, drops = 0, invalidWeaponCalls = 0;
    uint64_t missing = 0, failCall = 0;
    NativeCallStatus status = NativeCall_Ok;
    int32_t forcedCount = -999;
    bool control = true, arrested = false, corruptHeader = false, fullRange = true;
} sim;
void Reset() { sim = {}; sim.player = 100; sim.forcedCount = -999; sim.control = true; sim.fullRange = true; sim.actors[100] = {}; }
void Armed(Ped ped, int32_t bone, bool hit = false, bool hasBone = true) {
    sim.actors[ped] = {false, hasBone, bone, 1000 + ped, hit}; sim.weapons.insert(1000 + ped); sim.nearby.push_back(ped);
}
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
}

extern "C" NativeCallStatus getNativeCallStatus() { return Test::sim.status; }
extern "C" bool nativeIsAvailable(uint64_t hash) { return hash != Test::sim.missing; }
extern "C" bool scriptSaveFile(const char* name, const void* data, size_t size) {
    Test::Require(std::string(name) == "status.txt" && size < 640, "Report path/bound changed");
    Test::sim.reports.emplace_back(static_cast<const char*>(data), size); return true;
}
extern "C" bool nativeInvokeContext(uint64_t hash, void* raw, uint32_t bytes) {
    using namespace Test;
    auto& ctx = *static_cast<ScriptHook::NativeContext*>(raw);
    Require(bytes == sizeof(ctx), "Wrong context ABI");
    sim.status = NativeCall_Ok;
    if (hash == sim.failCall) { sim.status = NativeCall_MissingHandler; ctx.Fail(sim.status); return false; }
    const auto arg = [&](size_t i) { return static_cast<int32_t>(ctx.m_args[i]); };
    auto result = [&](int32_t value) { ctx.m_returns[0] = static_cast<uint32_t>(value); };
    if (hash == 0x4c1b8c5717647539ULL || hash == 0x0dba73788f6e3c5fULL) result(sim.buttons.count(arg(1)) != 0);
    else if (hash == 0x7653d561c9574763ULL) {}
    else if (hash == 0x9e2d6c50374fcfa9ULL) result(0);
    else if (hash == 0xe2d3d51028f0428aULL) result(sim.player);
    else if (hash == 0x320d1840b6daa1ccULL) result(static_cast<int32_t>(sim.now));
    else if (hash == 0xe5965cdf24f93a9fULL) result(sim.actors.count(arg(0)) || sim.weapons.count(arg(0)));
    else if (hash == 0x55b23fe400fcea6bULL) result(sim.actors.at(arg(0)).dead);
    else if (hash == 0x1c7a1a7e9e4766cfULL) result(sim.control);
    else if (hash == 0x99c20d75af0fb22eULL) result(sim.arrested);
    else if (hash == ScriptHook::World::NearbyPedsNative) {
        ++sim.queries; sim.lastAnchor = arg(0);
        auto* p = reinterpret_cast<uint8_t*>(ctx.m_args[1]); const int32_t capacity = *reinterpret_cast<int32_t*>(p);
        Require(capacity >= 1 && capacity <= 64 && arg(2) == -1 && (ctx.m_args[1] & 7u) == 0, "Bad scanner arguments");
        sim.fullRange &= ctx.m_pointerKinds[1] == ScriptHook::NativeContext::WritePointer && ctx.m_pointerBytes[1] == static_cast<uint32_t>((capacity + 1) * 8);
        int32_t count = 0;
        for (Ped ped : sim.nearby) {
            if (count >= capacity) break;
            *reinterpret_cast<int32_t*>(p + 8 + count * 8) = ped;
            *reinterpret_cast<uint32_t*>(p + 12 + count * 8) = 0xcafebabeu; // Upper half is not part of the handle.
            ++count;
        }
        if (sim.corruptHeader) *reinterpret_cast<int32_t*>(p) = capacity + 1;
        result(sim.forcedCount == -999 ? count : sim.forcedCount);
    }
    else if (hash == 0x4d03373543a78098ULL) { Require(arg(1) == 1, "Weapon dead-check default changed"); result(sim.actors.at(arg(0)).weapon); }
    else if (hash == 0x7709b2482f29a1bbULL) {
        auto& actor = sim.actors.at(arg(0));
        if (actor.hasBone) *reinterpret_cast<int32_t*>(ctx.m_args[1]) = actor.bone;
        result(actor.hasBone);
    }
    else if (hash == 0xe657cd67ef2ecdd3ULL) { sim.actors.at(arg(0)).hasBone = false; sim.events.push_back("bone-clear"); }
    else if (hash == 0xe10b76698e5cf067ULL || hash == 0xe68645525d451a8bULL) {
        if (!sim.weapons.count(arg(0))) { ++sim.invalidWeaponCalls; throw std::runtime_error("Weapon accessed after drop"); }
        auto& actor = sim.actors.at(arg(0) - 1000);
        if (hash == 0xe10b76698e5cf067ULL) result(actor.weaponHit);
        else { actor.weaponHit = false; sim.events.push_back("weapon-clear"); }
    }
    else if (hash == 0xa6d4fbb3b47cf11eULL) {
        auto& actor = sim.actors.at(arg(0)); sim.weapons.erase(actor.weapon); actor.weapon = 0; ++sim.drops; sim.events.push_back("drop");
    }
    else if (hash == 0x38bd019deccc5482ULL || hash == 0x76db21247ae4e4e2ULL || hash == 0x23d3ee043de19c4bULL) {}
    else throw std::runtime_error("Unexpected native hash");
    return true;
}

int main() {
    using namespace Test;
    uint32_t passed = 0, failed = 0;
    auto check = [&](const char* name, auto run) {
        Reset();
        try { run(); ++passed; std::printf("PASS %s\n", name); }
        catch (const std::exception& e) { ++failed; std::printf("FAIL %s: %s\n", name, e.what()); }
    };
    check("VM array header, stride, low handle word and full writable range", [&] {
        Armed(7, 0); ScriptHook::World::NearbyPeds<4> list;
        Require(list.Refresh(100) && list.Size() == 1 && list[0] == 7 && list[1] == 0 && sim.fullRange, "Bad packed array");
    });
    check("Empty scanner output is valid", [&] { ScriptHook::World::NearbyPeds<> list; Require(list.Refresh(100) && list.Size() == 0, "Empty scan rejected"); });
    check("Missing anchor avoids native query", [&] { ScriptHook::World::NearbyPeds<> list; Require(!list.Refresh(0) && sim.queries == 0, "Invalid anchor queried"); });
    check("Failed query clears previous handles", [&] {
        Armed(1, 0); ScriptHook::World::NearbyPeds<> list; Require(list.Refresh(100), "Initial query failed");
        sim.failCall = ScriptHook::World::NearbyPedsNative;
        Require(!list.Refresh(100) && list.Size() == 0 && list[0] == 0, "Stale query output retained");
    });
    check("Oversized and negative query counts rejected", [&] {
        ScriptHook::World::NearbyPeds<4> list; sim.forcedCount = 5; Require(!list.Refresh(100), "Overflow accepted");
        sim.forcedCount = -1; Require(!list.Refresh(100) && list.Size() == 0, "Negative count accepted");
    });
    check("Mutated capacity header rejected", [&] { sim.corruptHeader = true; ScriptHook::World::NearbyPeds<> list; Require(!list.Refresh(100), "Bad header accepted"); });
    check("Right hand and forearm disarm", [&] {
        Armed(1, DisarmNx::RightHand); Armed(2, DisarmNx::RightForearm);
        DisarmNx::Controller c; Require(c.Start(), "Start failed"); c.Tick();
        Require(sim.drops == 2 && c.Stats().handHits == 1 && c.Stats().armHits == 1 && c.Stats().weaponHits == 0, "Limb disarm failed");
    });
    check("Both upper arms and forearms disarm", [&] {
        for (int32_t bone : {DisarmNx::RightUpperArm, DisarmNx::RightForearm, DisarmNx::LeftUpperArm, DisarmNx::LeftForearm}) {
            Reset(); Armed(1, bone); DisarmNx::Controller c; c.Start(); c.Tick();
            Require(sim.drops == 1 && c.Stats().armHits == 1 && c.Stats().handHits == 0, "Arm hit missed");
        }
    });
    check("Weapon hit disarms without bone output", [&] {
        Armed(1, DisarmNx::RightHand, true, false); DisarmNx::Controller c; c.Start(); c.Tick();
        Require(sim.drops == 1 && c.Stats().weaponHits == 1 && c.Stats().handHits == 0, "Weapon disarm failed");
    });
    check("False bone return cannot trigger stale value", [&] {
        Armed(1, DisarmNx::RightHand, false, false); DisarmNx::Controller c; c.Start(); c.Tick(); Require(sim.drops == 0, "False output used");
    });
    check("Other bone and undamaged weapon do not disarm", [&] {
        Armed(1, 1234); DisarmNx::Controller c; c.Start(); c.Tick(); Require(sim.drops == 0 && !sim.actors.at(1).hasBone, "Unrelated damage disarmed");
    });
    check("Damage consumed before drop; combined hits drop once", [&] {
        Armed(1, DisarmNx::RightHand, true); DisarmNx::Controller c; c.Start(); c.Tick();
        Require(sim.events == std::vector<std::string>{"bone-clear", "weapon-clear", "drop"} && sim.invalidWeaponCalls == 0 && sim.drops == 1, "Unsafe drop ordering");
        c.Tick(); Require(sim.drops == 1, "Repeated drop without new weapon/hit");
    });
    check("Dead, invalid, player and unarmed handles skipped", [&] {
        Armed(1, DisarmNx::RightHand); sim.actors.at(1).dead = true; sim.actors[2] = {};
        sim.nearby = {1, 2, 999, 0, 100}; DisarmNx::Controller c; c.Start(); c.Tick(); Require(sim.drops == 0, "Ineligible ped touched");
    });
    check("Death, loss of control and arrest suspend scanning", [&] {
        Armed(1, DisarmNx::RightHand); DisarmNx::Controller c; c.Start(); sim.actors.at(100).dead = true; c.Tick();
        sim.actors.at(100).dead = false; sim.control = false; c.Tick(); sim.control = true; sim.arrested = true; c.Tick();
        Require(sim.queries == 0 && sim.drops == 0, "Suspended player queried");
    });
    check("Character change refreshes scanner immediately", [&] {
        DisarmNx::Controller c; c.Start(); c.Tick(); sim.player = 200; sim.actors[200] = {}; c.Tick(); Require(sim.lastAnchor == 200 && sim.queries == 2, "Old character retained");
    });
    check("Processing budget rotates through all 32 nearby peds", [&] {
        for (Ped ped = 1; ped <= 32; ++ped) Armed(ped, DisarmNx::RightHand);
        DisarmNx::Controller c; c.Start();
        for (uint32_t frame = 1; frame <= 4; ++frame) { c.Tick(); Require(sim.drops == frame * 8 && c.Stats().pedChecks == frame * 8 && sim.queries == 1, "Frame budget/cursor wrong"); sim.now += 16; }
    });
    check("Scanner refresh survives game timer wrap", [&] {
        sim.now = 0xfffffff0u; DisarmNx::Controller c; c.Start(); c.Tick(); sim.now = 100; c.Tick(); Require(sim.queries == 1, "Premature refresh");
        sim.now = 300; c.Tick(); Require(sim.queries == 2, "Wrapped timer stuck");
    });
    check("Missing required native prevents activation", [&] {
        sim.missing = ScriptHook::World::NearbyPedsNative; DisarmNx::Controller c; Require(!c.Start(), "Missing native accepted"); c.Tick();
        Require(sim.queries == 0 && sim.reports.back().find("required-native-unavailable") != std::string::npos, "Missing-native status absent");
    });
    check("Native read failure does not drop a weapon", [&] {
        Armed(1, DisarmNx::RightHand); DisarmNx::Controller c; c.Start(); sim.failCall = 0x7709b2482f29a1bbULL; c.Tick();
        Require(sim.drops == 0 && c.Stats().nativeFailures == 1, "Failed call applied");
    });
    check("Query failure is counted without processing stale handles", [&] {
        Armed(1, DisarmNx::RightHand); DisarmNx::Controller c; c.Start(); sim.failCall = ScriptHook::World::NearbyPedsNative; c.Tick();
        Require(sim.drops == 0 && c.Stats().queryFailures == 1 && c.Stats().pedChecks == 0, "Failed query applied");
    });
    check("Status writes are bounded and do not run every frame", [&] {
        Armed(1, DisarmNx::RightHand); DisarmNx::Controller c; c.Start();
        for (unsigned frame = 0; frame < 1000; ++frame) { c.Tick(); sim.now += 16; }
        Require(sim.reports.size() == 2 && sim.reports.back().find("disarms=1") != std::string::npos, "Excessive/missing status writes");
    });
    check("INI sections, default section, duplicate keys and CRLF", [&] {
        ScriptHook::Util::Ini<> ini;const char text[]="global=7\r\n[a]\r\nkey=1\r\nKEY=2\nflag=on\n";
        Require(ini.Parse(text,sizeof(text)-1) && ini.Int("","global",0)==7 && ini.Int("A","key",0)==2 && ini.Bool("a","flag",false),"INI lookup failed");
    });
    check("INI integer range and overflow preserve defaults", [&] {
        ScriptHook::Util::Ini<> ini;const char text[]="[a]\nlarge=999999999999999999\nnegative=-2147483648\nwrong=2no\n";
        Require(ini.Parse(text,sizeof(text)-1) && ini.Int("a","large",9)==9 && ini.Int("a","negative",0)==INT32_MIN && ini.Int("a","wrong",8)==8,"INI integer handling failed");
    });
    check("INI malformed sections, quoted and NUL input rejected", [&] {
        ScriptHook::Util::Ini<> ini;
        Require(!ini.Parse("[a",2) && !ini.Parse("a=\"quoted\"",10) && !ini.Parse("a=1\0b=2",7),"INI malformed input accepted");
    });
    check("INI bad UTF-8 and capacity limits rejected", [&] {
        ScriptHook::Util::Ini<32,1> ini;const char bad[]={'\xff','=', '1'};
        Require(!ini.Parse(bad,3)&&!ini.Parse("a=1\nb=2",7)&&!ini.Parse("x",32),"INI limits ignored");
    });
    check("INI BOM, comments, empty values and strict booleans", [&] {
        ScriptHook::Util::Ini<> ini;const char text[]="\xef\xbb\xbf; comment\n[a] # ok\nempty=\nflag=maybe\n";
        Require(ini.Parse(text,sizeof(text)-1)&&ini.Get("a","empty","fallback")[0]==0&&ini.Bool("a","flag",true),"INI defaults failed");
    });
    check("Nintendo Y chord triggers once per complete press", [&] {
        ScriptHook::Input::ShoulderChord chord;
        sim.buttons={205,206,203};Require(chord.Poll()&&!chord.Poll(),"Held chord repeats");
        sim.buttons.erase(203);Require(!chord.Poll(),"Release triggers");sim.buttons.insert(203);Require(chord.Poll(),"Second press absent");
    });
    check("Chord requires both shoulders and selected face button", [&] {
        ScriptHook::Input::ShoulderChord chord(ScriptHook::Input::Button::X);
        sim.buttons={205,204};Require(!chord.Poll(),"One shoulder accepted");sim.buttons={205,206,203};Require(!chord.Poll(),"Wrong face accepted");sim.buttons={205,206,204};Require(chord.Poll(),"Nintendo X absent");
    });
    std::printf("RESULT %u passed; %u failed. Host simulation; Switch gameplay pending.\n", passed, failed);
    return failed ? 1 : 0;
}
