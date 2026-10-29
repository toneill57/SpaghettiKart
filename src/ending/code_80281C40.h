#ifndef CODE_80281C40_H
#define CODE_80281C40_H

/* Function Prototypes */

void func_80281C40(void);
void func_80281CB4(s32, s32, s32, s32);
void func_80281D00(void);

extern s32 gGotoMenu;
extern u16 gIsInQuitToMenuTransition;
extern Gfx D_00284F70[];
extern Gfx D_00284EE0[];

// FASE 1.4c: read by the oracle's HashCGlobals (src/port/oracle); the defining file includes this header,
// so the compiler checks each declaration against its definition.
extern s32 D_80287554; // code_80281C40.c:32

#endif
