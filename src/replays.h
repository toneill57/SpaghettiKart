#ifndef REPLAYS_H
#define REPLAYS_H

#include <libultraship.h>
#include <common_structs.h>

void func_80005B18(void);
void load_course_ghost(void);
void load_post_time_trial_replay(void);
void load_player_ghost(void);
void set_staff_ghost(void);
s32 func_800051C4(void);
void func_8000522C(void);
void func_800052A4(void);
void func_80005310(void);
void process_post_time_trial_replay(void);
void process_course_ghost_replay(void);
void process_player_ghost_replay(void);
void save_player_replay(void);
void func_80005AE8(Player*);
void func_80005E6C(void);
void replays_loop(void);

// mi0decode

extern s32 mio0encode(uintptr_t input, s32, uintptr_t);

extern StaffGhost* D_80162DC4;
extern s32 D_80162DC8;
extern s32 D_80162DCC;
extern u16 bPlayerGhostDisabled;
extern u16 bCourseGhostDisabled;
extern u16 D_80162DD8;
extern s32 D_80162E00;
extern s32 D_80162DE0;
extern s32 D_80162DE4;
extern s32 D_80162DE8;
extern s32 gPauseTriggered;
extern s32 D_80162DF4;
extern s32 gPostTimeTrialReplayCannotSave;

extern u8* sReplayGhostBuffer;
extern size_t sReplayGhostBufferSize;
extern u32* sPlayerGhostReplay;

// FASE 1.4c: read by the oracle's HashCGlobals (src/port/oracle); the defining file includes this header,
// so the compiler checks each declaration against its definition.
extern s16 D_80162D86; // replays.c:22
extern s32 D_80162DD0; // replays.c:46
extern s32 D_80162DDC; // replays.c:50
extern s32 sUnusedReplayCounter; // replays.c:54
extern uintptr_t staff_ghost_track_ptr; // replays.c:42
extern u16 sPlayerGhostButtonsPrev; // replays.c
extern u32 sPlayerGhostFramesRemaining; // replays.c
extern s16 sPlayerGhostReplayIdx; // replays.c
extern u16 sButtonsPrevCourseGhost; // replays.c
extern u32 sCourseGhostFramesRemaining; // replays.c
extern s16 sCourseGhostReplayIdx; // replays.c
extern u32* sCourseGhostReplay; // replays.c
extern u16 sPostTTButtonsPrev; // replays.c
extern s32 sPostTTFramesRemaining; // replays.c
extern s16 sPostTTReplayIdx; // replays.c
extern u32* sPostTTReplay; // replays.c
extern s16 sPlayerInputIdx; // replays.c
extern u32* sPlayerInputs; // replays.c

#endif /* REPLAYS_H */
