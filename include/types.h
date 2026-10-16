#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint32_t Void;
typedef uint64_t Any;
typedef int32_t BOOL; // Native BOOL outputs occupy four bytes.
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t uint;
typedef uint32_t Hash;
typedef int32_t Entity;
typedef int32_t Player;
typedef int32_t FireId;
typedef int32_t Ped;
typedef int32_t Vehicle;
typedef int32_t Cam;
typedef int32_t CarGenerator;
typedef int32_t Group;
typedef int32_t Train;
typedef int32_t Pickup;
typedef int32_t Object;
typedef int32_t Weapon;
typedef int32_t Interior;
typedef int32_t Blip;
typedef int32_t Texture;
typedef int32_t TextureDict;
typedef int32_t CoverPoint;
typedef int32_t Camera;
typedef int32_t TaskSequence;
typedef int32_t ColourIndex;
typedef int32_t Sphere;
typedef int32_t ScrHandle;

// TASK_FOLLOW_NAV_MESH_TO_COORD_ADVANCED consumes these as three FLOAT slots.
typedef struct NativeNavData {
    float slideToCoordHeading, maxSlopeNavigable, clampMaxSearchDistance;
} NativeNavData;

#ifdef __cplusplus
struct alignas(8) Vector3 {
    float x;
    float _padX;
    float y;
    float _padY;
    float z;
    float _padZ;

    Vector3() : x(0.f), _padX(0.f), y(0.f), _padY(0.f), z(0.f), _padZ(0.f) {}
    Vector3(float _x, float _y, float _z) : x(_x), _padX(0.f), y(_y), _padY(0.f), z(_z), _padZ(0.f) {}
};
static_assert(sizeof(Vector3) == 24, "Vector3 in RAGE must be 24 bytes (padded)");
static_assert(offsetof(Vector3, x) == 0 && offsetof(Vector3, y) == 8 && offsetof(Vector3, z) == 16);
#else
typedef struct Vector3 { float x, _padX, y, _padY, z, _padZ; } Vector3;
#endif
