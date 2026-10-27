#ifndef RACE_LOGIC_H
#define RACE_LOGIC_H

/* Function Prototypes */

void func_802903D8(Player*, Player*);
void func_8028DF00(void);
void func_8028DF38(void);
void func_8028E028(void);
void update_player_battle_status(void);
void func_8028E298(void);
void set_next_course(void);
void func_8028E438(void);
void func_8028E678(void);
void func_8028EC38(s32);
void func_8028EC98(void);
void start_race(void);
f32 func_8028EE8C(s32);
void func_8028EEF0(s32);
void func_8028EF28(void);
void func_8028F3E8(void);
void update_race_position_data(void);
void func_8028F474(void);
void func_8028F4E8(void);
void func_8028F588(void);
void func_8028F8BC(void);
void func_8028F914(void);
void func_8028F970(void);
void func_8028FBD4(void);
void func_8028FC34(void);
void func_8028FCBC(void);
void func_80290314(void);
void func_80290338(void);
void func_80290360(void);
void func_80290388(void);
void func_802903B0(void);
void func_802909F0(void);
void func_80290B14(void);

extern f32 gLapCompletionPercentByPlayerId[];
extern s32 gGPCurrentRaceRankByPlayerId[]; // D_801643B8 (position for each player)
extern u16 bCourseGhostDisabled;

// FASE 1.4c: read by the oracle's HashCGlobals (src/port/oracle); the defining file includes this header,
// so the compiler checks each declaration against its definition.
extern u16 D_802BA030; // race_logic.c:40
extern u16 D_802BA032; // race_logic.c:41
extern float D_802BA034; // race_logic.c:43
extern s32 D_802BA038; // race_logic.c:45
extern s16 D_802BA040[4]; // race_logic.c:48
extern u16 D_802BA048; // race_logic.c:49

#endif
