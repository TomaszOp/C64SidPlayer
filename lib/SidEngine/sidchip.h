/**
 * @file sidchip.h
 * @brief Compile-time selection of the SID chip emulation engine.
 *
 * Selected with a preprocessor define (e.g. in platformio.ini):
 * @code
 *   -DSID_ENGINE=0   own lightweight engine (sid.h)
 *   -DSID_ENGINE=1   reSID 0.16 (Dag Lem) - more faithful sound, higher CPU load
 *   -DSID_ENGINE=2   real 6581/8580 chip connected to the ESP32 (real_sid_chip.h,
 *                    model and pins in include/config.h, wiring in docs/REAL_SID.md)
 * @endcode
 * Additionally for reSID:
 * @code
 *   -DSID_RESID_INTERPOLATE=1   sampling with interpolation (better quality, much slower)
 * @endcode
 *
 * The chosen class is exposed as the type ::SidChip and its display name as ::SID_ENGINE_NAME.
 */
#pragma once
#include "sid.h"

#define SID_ENGINE_OWN   0   ///< Engine id: own lightweight emulation (class Sid).
#define SID_ENGINE_RESID 1   ///< Engine id: reSID wrapper (class ResidChip).
#define SID_ENGINE_REAL  2   ///< Engine id: real SID chip on the ESP32 pins (class RealSidChip).

#ifndef SID_ENGINE
/// Selected SID engine (one of SID_ENGINE_OWN, SID_ENGINE_RESID, SID_ENGINE_REAL); default: own engine.
#define SID_ENGINE SID_ENGINE_OWN
#endif
#ifndef SID_RESID_INTERPOLATE
/// 1 = reSID uses SAMPLE_INTERPOLATE, 0 = SAMPLE_FAST (default).
#define SID_RESID_INTERPOLATE 0
#endif

#if SID_ENGINE == SID_ENGINE_REAL
#include "real_sid_chip.h"
/// SID chip implementation used by SidPlayer (selected by SID_ENGINE).
typedef RealSidChip SidChip;
#if REAL_SID_MODEL
#define SID_ENGINE_NAME "prawdziwy SID 8580"   ///< Human-readable engine name shown in the UI.
#else
#define SID_ENGINE_NAME "prawdziwy SID 6581"   ///< Human-readable engine name shown in the UI.
#endif
#elif SID_ENGINE == SID_ENGINE_RESID
#include "resid_chip.h"
/// SID chip implementation used by SidPlayer (selected by SID_ENGINE).
typedef ResidChip SidChip;
#if SID_RESID_INTERPOLATE
#define SID_ENGINE_NAME "reSID interpolate"    ///< Human-readable engine name shown in the UI.
#else
#define SID_ENGINE_NAME "reSID fast"           ///< Human-readable engine name shown in the UI.
#endif
#else
/// SID chip implementation used by SidPlayer (selected by SID_ENGINE).
typedef Sid SidChip;
#define SID_ENGINE_NAME "wlasny"               ///< Human-readable engine name shown in the UI.
#endif
