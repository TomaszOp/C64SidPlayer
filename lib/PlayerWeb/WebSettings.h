/**
 * @file WebSettings.h
 * @brief WebSettings - web panel settings stored in NVS, independent of the
 *        player settings. List of settings: WebSettings.def.
 */
#pragma once
#include <NvsSettings.h>

#define NVS_CLASS     WebSettings
#define NVS_DEF       "WebSettings.def"
#define NVS_NAMESPACE "c64web"
#define NVS_VERSION   1
#include <NvsSettingsClass.h>
