#pragma once
/* The ONLY oracle header game C code includes (src/main.c). Must stay C-compilable: no C++ headers, no bool. */
#include <stdint.h>
#include <libultraship/libultra/controller.h> /* OSContPad */

#ifdef __cplusplus
extern "C" {
#endif

int32_t Oracle_IsActive(void);              /* 1 while recording or replaying */
void Oracle_ApplySessionGuards(void);       /* FASE 1.2 — once per thread5_iteration */
void Oracle_OnPadsRead(OSContPad* pads);    /* FASE 1.3 / 1.5 — between main.c:397 and :398 (stub until then) */
void Oracle_OnIterationEnd(void);           /* FASE 1.4 — after game_state_handler() (stub until then) */
void Oracle_OnSetupRace(void);              /* FASE 1.7 — before setup_race() at main.c:1116 (stub until then) */

#ifdef __cplusplus
}
#endif
