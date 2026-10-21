#include "Header.hpp"

void RequestPtfxAsset(const char* ptfxName)
{
	STREAMING::REQUEST_NAMED_PTFX_ASSET(ptfxName);
	while (!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(ptfxName)) WAIT(0);
}

void DisplaySubtitle(const char* text, int timeMs)
{
	UI::_SET_TEXT_ENTRY_2("STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(text);
	UI::_DRAW_SUBTITLE_TIMED(timeMs, 1);
}

void ScriptInit()
{
	const Hash cheatCodeHash = GAMEPLAY::GET_HASH_KEY("ONFIRE"); // "ONFIRE" is our cheat code

	while (true)
	{
		if (GAMEPLAY::_HAS_CHEAT_STRING_JUST_BEEN_ENTERED(cheatCodeHash)) // If the player entered our cheat code...
		{
			Ped playerPed = PLAYER::PLAYER_PED_ID();

			if (PED::GET_PED_CONFIG_FLAG(playerPed, 421, true)) // Check if their ped has flag 421
			{
				// Ped has flag 421, which means flaming footprints are enabled.
				GRAPHICS::_0xBA3D194057C79A7B("");
				PED::SET_PED_CONFIG_FLAG(playerPed, 421, false);

				DisplaySubtitle("Flaming footprints ~r~disabled.", 5000);
			}
			else
			{
				// Ped doesn't have flag 421, which means flaming footprints are not enabled.
				if (!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED("scr_bike_adversary")) RequestPtfxAsset("scr_bike_adversary");

				GRAPHICS::_0xBA3D194057C79A7B("scr_adversary_foot_flames");
				PED::SET_PED_CONFIG_FLAG(playerPed, 421, true);

				DisplaySubtitle("Flaming footprints ~g~enabled.", 5000);
			}
		}

		WAIT(0);
	}
}