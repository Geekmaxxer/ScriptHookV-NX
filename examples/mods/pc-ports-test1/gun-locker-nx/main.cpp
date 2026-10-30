// Native C++ adaptation of root-cause/v-spgunlocker (MIT).
#include "components.hpp"
#include "port.hpp"
using namespace Port;
static Entity locker = 0;
static Blip pin = 0;
void Stop() {
    ClearOwned();
    locker = 0;
    pin = 0;
}
uint32_t Bits(float f) {
    uint32_t w;
    __builtin_memcpy(&w, &f, 4);
    return w;
}
float Float(uint32_t w) {
    float f;
    __builtin_memcpy(&f, &w, 4);
    return f;
}
bool PositionValid(uint32_t *w) {
    return Float(w[1]) > -10000 && Float(w[1]) < 10000 && Float(w[2]) > -10000 && Float(w[2]) < 10000 &&
           Float(w[3]) > -500 && Float(w[3]) < 2000;
}
const char *WeaponName(Hash hash) {
    struct Name {
        const char *model;
        const char *label;
    };
    const Name names[] = {{"WEAPON_PISTOL", "Pistol"},
                          {"WEAPON_COMBATPISTOL", "Combat Pistol"},
                          {"WEAPON_APPISTOL", "AP Pistol"},
                          {"WEAPON_PISTOL50", ".50 Pistol"},
                          {"WEAPON_SNSPISTOL", "SNS Pistol"},
                          {"WEAPON_HEAVYPISTOL", "Heavy Pistol"},
                          {"WEAPON_REVOLVER", "Heavy Revolver"},
                          {"WEAPON_DOUBLEACTION", "Double-action Revolver"},
                          {"WEAPON_NAVYREVOLVER", "Navy Revolver"},
                          {"WEAPON_SMG", "SMG"},
                          {"WEAPON_MICROSMG", "Micro SMG"},
                          {"WEAPON_ASSAULTSMG", "Assault SMG"},
                          {"WEAPON_ASSAULTRIFLE", "Assault Rifle"},
                          {"WEAPON_CARBINERIFLE", "Carbine Rifle"},
                          {"WEAPON_ADVANCEDRIFLE", "Advanced Rifle"},
                          {"WEAPON_SPECIALCARBINE", "Special Carbine"},
                          {"WEAPON_PUMPSHOTGUN", "Pump Shotgun"},
                          {"WEAPON_SAWNOFFSHOTGUN", "Sawed-off Shotgun"},
                          {"WEAPON_ASSAULTSHOTGUN", "Assault Shotgun"},
                          {"WEAPON_BULLPUPSHOTGUN", "Bullpup Shotgun"},
                          {"WEAPON_SNIPERRIFLE", "Sniper Rifle"},
                          {"WEAPON_HEAVYSNIPER", "Heavy Sniper"},
                          {"WEAPON_MG", "MG"},
                          {"WEAPON_COMBATMG", "Combat MG"},
                          {"WEAPON_RPG", "RPG"},
                          {"WEAPON_MINIGUN", "Minigun"},
                          {"WEAPON_GRENADELAUNCHER", "Grenade Launcher"},
                          {"WEAPON_KNIFE", "Knife"},
                          {"WEAPON_BAT", "Baseball Bat"},
                          {"WEAPON_STUNGUN", "Stun Gun"}};
    for (const auto &n : names)
        if (H(n.model) == hash)
            return n.label;
    return nullptr;
}
void ScriptMain() {
    Stop();
    App app(6, "Single Player Gun Locker NX");
    uint32_t data[160]{};
    int bank = -1;
    uint32_t next = 0, retry = 0;
    const char *namesA[] = {"michael-a.bin", "franklin-a.bin", "trevor-a.bin"};
    const char *namesB[] = {"michael-b.bin", "franklin-b.bin", "trevor-b.bin"};
    Report("ready");
    for (;;) {
        if (!app.Frame()) {
            WAIT(100);
            continue;
        }
        int key = CashKey(), current = key == int(H("SP0_TOTAL_CASH"))   ? 0
                                       : key == int(H("SP1_TOTAL_CASH")) ? 1
                                       : key == int(H("SP2_TOTAL_CASH")) ? 2
                                                                         : -1;
        if (current != bank) {
            Stop();
            for (auto &w : data)
                w = 0;
            bank = current;
            if (bank >= 0) {
                ScriptHook::Util::SavedSettings<160> load(0x474c4b31, 1, namesA[bank], namesB[bank]);
                if (!load.Load(data) || data[0] > 1 || !PositionValid(data)) {
                    for (auto &w : data)
                        w = 0;
                }
            }
            retry = 0;
        }
        auto save = [&]() {
            if (bank < 0)
                return false;
            ScriptHook::Util::SavedSettings<160> s(0x474c4b31, 1, namesA[bank], namesB[bank]);
            return s.Save(data);
        };
        Vector3 pos = V(Float(data[1]), Float(data[2]), Float(data[3])),
                player = ENTITY::GET_ENTITY_COORDS(PedSelf(), true);
        bool near = data[0] && Dist2(player, pos) < 9;
        int used = 0;
        for (int i = 0; i < 10; ++i)
            if (data[8 + i * 14])
                ++used;
        TextBuffer<80> status;
        if (bank < 0)
            status.Add("Story character required");
        else if (!data[0])
            status.Add("No locker purchased");
        else
            status.Add(near ? "NEAR | " : "Saved | ").UInt(unsigned(used)).Add(" / 10 weapons");
        app.Status(status.CStr(), data[0] ? "Locate the locker; stand within 3m to use it"
                                          : "On foot: buy a locker for $2,500 in the menu");
        if (app.Open()) {
            app.menu.ReadOnly(1, "Character",
                              bank >= 0 ? (bank == 0   ? "Michael"
                                           : bank == 1 ? "Franklin"
                                                       : "Trevor")
                                        : "Unsupported model");
            app.menu.Action(2, "Buy locker here - $2,500",
                            "One persistent locker per story character. Choose a safe place on foot.",
                            bank >= 0 && !data[0]);
            app.menu.Action(3, "Locate locker", "", data[0] != 0);
            app.menu.Action(
                4, "Store current weapon",
                "Stores ammo, clip, tint and known compatible components; removes it from the "
                "player after saving.",
                near && used < 10);
            for (int i = 0; i < 10; ++i) {
                TextBuffer<64> label;
                label.Add("Slot ").UInt(unsigned(i + 1));
                if (data[8 + i * 14]) {
                    label.Add(" | ");
                    const char *name = WeaponName(data[8 + i * 14]);
                    if (name)
                        label.Add(name);
                    else
                        label.UInt(data[8 + i * 14]);
                } else
                    label.Add(" | Empty");
                app.menu.Action(
                    100 + unsigned(i), label.CStr(),
                    "Withdraw this weapon. You must be near the locker and not already carrying it.",
                    near && data[8 + i * 14]);
            }
            app.menu.Action(5, "Refund empty locker", "Only empty lockers can be returned.",
                            near && !used);
        }
        app.End();
        int a = int(app.menu.Activated());
        if (a == 2 && Car())
            Notice("Exit your vehicle before buying a locker");
        if (a == 2 && bank >= 0 && !data[0] && !Car()) {
            Hash model = H("bkr_prop_gunlocker_01a");
            if (Model(model)) {
                Release(model);
                int cash = 0;
                if (STATS::STAT_GET_INT(key, &cash, -1) && cash >= 2500) {
                    data[0] = 1;
                    Vector3 at =
                        ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(PedSelf(), V(0, 1.5f, -1.f));
                    data[1] = Bits(at.x);
                    data[2] = Bits(at.y);
                    data[3] = Bits(at.z);
                    data[4] = Bits(ENTITY::GET_ENTITY_HEADING(PedSelf()));
                    if (save()) {
                        if (!Pay(-2500)) {
                            data[0] = 0;
                            save();
                        } else
                            Notice("Locker purchased and saved");
                    } else {
                        data[0] = 0;
                        Notice("Locker save failed");
                    }
                } else
                    Notice("Not enough cash");
            }
        }
        if (a == 3 && data[0]) {
            Route(pos);
            Notice("Locker waypoint set; approach within three metres");
        }
        if (a == 4 && near && used < 10) {
            Hash weapon = 0;
            if (WEAPON::GET_CURRENT_PED_WEAPON(PedSelf(), &weapon, true) &&
                weapon != H("WEAPON_UNARMED") && WEAPON::IS_WEAPON_VALID(weapon)) {
                int slot = -1;
                for (int i = 0; i < 10; ++i)
                    if (!data[8 + i * 14]) {
                        slot = i;
                        break;
                    }
                if (slot >= 0) {
                    uint32_t *w = &data[8 + slot * 14];
                    w[0] = weapon;
                    w[1] = uint32_t(WEAPON::GET_AMMO_IN_PED_WEAPON(PedSelf(), weapon));
                    int clip = 0;
                    WEAPON::GET_AMMO_IN_CLIP(PedSelf(), weapon, &clip);
                    w[2] = uint32_t(clip);
                    w[3] = uint32_t(WEAPON::GET_PED_WEAPON_TINT_INDEX(PedSelf(), weapon));
                    for (unsigned i = 0; i < 10; ++i)
                        w[4 + i] = 0;
                    for (unsigned i = 0; i < Data::componentCount; ++i) {
                        Hash component = H(Data::components[i]);
                        if (WEAPON::DOES_WEAPON_TAKE_WEAPON_COMPONENT(weapon, int(component)) &&
                            WEAPON::HAS_PED_GOT_WEAPON_COMPONENT(PedSelf(), weapon, int(component)))
                            w[4 + i / 32] |= 1u << (i % 32);
                    }
                    if (save()) {
                        WEAPON::REMOVE_WEAPON_FROM_PED(PedSelf(), weapon);
                        Notice("Weapon stored");
                        Report("weapon_stored");
                    } else {
                        for (int i = 0; i < 14; ++i)
                            w[i] = 0;
                        Notice("Weapon save failed; inventory unchanged");
                    }
                }
            } else
                Notice("Equip a valid weapon first");
        }
        if (a >= 100 && a < 110 && near) {
            uint32_t *w = &data[8 + (a - 100) * 14];
            Hash weapon = w[0];
            if (weapon && WEAPON::IS_WEAPON_VALID(weapon) &&
                !WEAPON::HAS_PED_GOT_WEAPON(PedSelf(), weapon, false)) {
                uint32_t old[14];
                for (int i = 0; i < 14; ++i) {
                    old[i] = w[i];
                    w[i] = 0;
                }
                if (save()) {
                    WEAPON::GIVE_WEAPON_TO_PED(PedSelf(), weapon, int(old[1] > 9999 ? 9999 : old[1]),
                                               false, true);
                    for (unsigned i = 0; i < Data::componentCount; ++i)
                        if (old[4 + i / 32] & (1u << (i % 32)))
                            WEAPON::GIVE_WEAPON_COMPONENT_TO_PED(PedSelf(), weapon,
                                                                 int(H(Data::components[i])));
                    WEAPON::SET_PED_WEAPON_TINT_INDEX(PedSelf(), weapon, int(old[3]));
                    WEAPON::SET_AMMO_IN_CLIP(PedSelf(), weapon, int(old[2] > 9999 ? 9999 : old[2]));
                    Notice("Weapon withdrawn");
                    Report("weapon_withdrawn");
                } else {
                    for (int i = 0; i < 14; ++i)
                        w[i] = old[i];
                    Notice("Withdrawal save failed");
                }
            } else
                Notice("Weapon already carried or unavailable");
        }
        if (a == 5 && near && !used) {
            data[0] = 0;
            if (save()) {
                Stop();
                Pay(2500);
                Notice("Empty locker refunded");
            } else {
                data[0] = 1;
                Notice("Refund save failed");
            }
        }
        if (Due(Now(), next)) {
            next = Now() + 1000;
            if (data[0] && Dist2(player, pos) < 6400) {
                if (!locker && Due(Now(), retry)) {
                    retry = Now() + 5000;
                    locker = Prop("bkr_prop_gunlocker_01a", pos, V(0, 0, Float(data[4])));
                    if (locker)
                        pin = Pin(pos, 110, 3);
                }
            } else {
                Delete(locker);
                Unpin(pin);
            }
        }
        WAIT(0);
    }
}
SHV_NX_PLUGIN_WITH_STOP("gun-locker-nx", ScriptMain, Stop)
