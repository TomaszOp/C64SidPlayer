// reSID 0.16 (Dag Lem) - jedna jednostka kompilacji, jak w Teensy-reSID (Frank Bosing).
// Pliki .inc to oryginalne zrodla .cc; tablice przebiegow musza byc widoczne przed wave.inc.
#include "../sidchip.h"
#if SID_ENGINE == SID_ENGINE_RESID
#include "sid.h"
#include "envelope.inc"
#include "extfilt.inc"
#include "filter.inc"
#include "pot.inc"
#include "version.inc"
#include "voice.inc"
#include "wave6581__ST.inc"
#include "wave6581_P_T.inc"
#include "wave6581_PS_.inc"
#include "wave6581_PST.inc"
#include "wave8580__ST.inc"
#include "wave8580_P_T.inc"
#include "wave8580_PS_.inc"
#include "wave8580_PST.inc"
#include "wave.inc"
#include "sid.inc"
#endif
