#ifndef NV2A_VSH_CPU_SRC_NV2A_VSH_EMULATOR_H_
#define NV2A_VSH_CPU_SRC_NV2A_VSH_EMULATOR_H_

#include "nv2a_vsh_emulator_execution_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Emulates the given program by applying each step to the given state.
void nv2a_vsh_emu_execute(Nv2aVshExecutionState *state,
                          const Nv2aVshProgram *program);

// Emulates the given step by applying it to the given state.
void nv2a_vsh_emu_apply(Nv2aVshExecutionState *state, const Nv2aVshStep *step);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // NV2A_VSH_CPU_SRC_NV2A_VSH_EMULATOR_H_
