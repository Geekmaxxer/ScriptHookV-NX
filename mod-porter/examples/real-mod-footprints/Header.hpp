#pragma once
#include "main.h"
#include "natives.h"

void ScriptInit();
bool RequestPtfxAsset(const char* ptfxName);
void DisplaySubtitle(const char* text, int timeMs);