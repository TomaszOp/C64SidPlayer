// Wybor silnika emulacji ukladu SID (dyrektywa preprocesora, np. w platformio.ini):
//   -DSID_ENGINE=0   wlasny, lekki silnik (sid.h)
//   -DSID_ENGINE=1   reSID 0.16 (Dag Lem) - wierniejszy dzwiek, wieksze obciazenie CPU
// Dla reSID dodatkowo:
//   -DSID_RESID_INTERPOLATE=1   probkowanie z interpolacja (lepsza jakosc, duzo wolniej)
#pragma once
#include "sid.h"

#define SID_ENGINE_OWN   0
#define SID_ENGINE_RESID 1

#ifndef SID_ENGINE
#define SID_ENGINE SID_ENGINE_OWN
#endif
#ifndef SID_RESID_INTERPOLATE
#define SID_RESID_INTERPOLATE 0
#endif

#if SID_ENGINE == SID_ENGINE_RESID
#include "resid_chip.h"
typedef ResidChip SidChip;
#if SID_RESID_INTERPOLATE
#define SID_ENGINE_NAME "reSID interpolate"
#else
#define SID_ENGINE_NAME "reSID fast"
#endif
#else
typedef Sid SidChip;
#define SID_ENGINE_NAME "wlasny"
#endif
