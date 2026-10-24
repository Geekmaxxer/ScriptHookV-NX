#pragma once
namespace MineData {
struct Set {
    const char *model;
    const char *weapons[5];
    int count;
};
static constexpr Set sets[] = {
    {"w_ex_vehiclemine", {"VEHICLE_WEAPON_MINE"}, 1},
    {"w_ex_arena_landmine_01b", {"VEHICLE_WEAPON_MINE_KINETIC_RC", "VEHICLE_WEAPON_MINE_EMP_RC"}, 2},
    {"w_ex_vehiclemine",
     {"VEHICLE_WEAPON_MINE_KINETIC", "VEHICLE_WEAPON_MINE_SPIKE", "VEHICLE_WEAPON_MINE_EMP",
      "VEHICLE_WEAPON_MINE_SLICK", "VEHICLE_WEAPON_MINE_TAR"},
     5},
    {"w_ex_vehiclemine", {"VEHICLE_WEAPON_MINE_SPIKE", "VEHICLE_WEAPON_MINE_SLICK"}, 2},
};
struct Vehicle {
    const char *model;
    int set;
};
static constexpr Vehicle vehicles[] = {
    {"apc", 0},        {"dune3", 0},      {"halftrack", 0},  {"tampa3", 0},     {"insurgent3", 0},
    {"technical3", 0}, {"khanjali", 0},   {"speedo4", 0},    {"mule4", 0},      {"pounder2", 0},
    {"jb7002", 3},     {"rcbandito", 1},  {"bruiser", 2},    {"bruiser2", 2},   {"bruiser3", 2},
    {"brutus", 2},     {"brutus2", 2},    {"brutus3", 2},    {"cerberus", 2},   {"cerberus2", 2},
    {"cerberus3", 2},  {"dominator4", 2}, {"dominator5", 2}, {"dominator6", 2}, {"impaler2", 2},
    {"impaler3", 2},   {"impaler4", 2},   {"imperator", 2},  {"imperator2", 2}, {"imperator3", 2},
    {"issi4", 2},      {"issi5", 2},      {"issi6", 2},      {"monster3", 2},   {"monster4", 2},
    {"monster5", 2},   {"scarab", 2},     {"scarab2", 2},    {"scarab3", 2},    {"slamvan4", 2},
    {"slamvan5", 2},   {"slamvan6", 2},   {"zr380", 2},      {"zr3802", 2},     {"zr3803", 2},
};
} // namespace MineData
