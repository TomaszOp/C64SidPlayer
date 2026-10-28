/**
 * @file LcdSettings.h
 * @brief LcdSettings - LCD settings remembered in NVS, independent of the player
 *        and web panel settings. List of settings: LcdSettings.def.
 *
 * Generates the class LcdSettings (see NvsSettingsClass.h) in NVS namespace "c64lcd".
 */
#pragma once
#include <NvsSettings.h>

#define NVS_CLASS     LcdSettings
#define NVS_DEF       "LcdSettings.def"
#define NVS_NAMESPACE "c64lcd"
#define NVS_VERSION   1
#include <NvsSettingsClass.h>
