#include <boost/test/unit_test.hpp>

#include "nv2a_vsh_emulator.h"

#define CHECK_REGISTER(bank, index, actual) \
  do {                                      \
    float *expected = bank + index * 4;     \
    BOOST_TEST((expected)[0] == actual[0]); \
    BOOST_TEST((expected)[1] == actual[1]); \
    BOOST_TEST((expected)[2] == actual[2]); \
    BOOST_TEST((expected)[3] == actual[3]); \
  } while (0)

static void clear_step(Nv2aVshStep *out) {
  out->is_final = false;
  memset(out->mac.outputs, 0, sizeof(out->mac.outputs));
  memset(out->mac.inputs, 0, sizeof(out->mac.inputs));
  memset(out->ilu.outputs, 0, sizeof(out->ilu.outputs));
  memset(out->ilu.inputs, 0, sizeof(out->ilu.inputs));

  out->mac.opcode = NV2AOP_NOP;
  out->mac.inputs[0].type = NV2ART_NONE;
  out->mac.inputs[0].swizzle[0] = NV2ASW_X;
  out->mac.inputs[0].swizzle[1] = NV2ASW_Y;
  out->mac.inputs[0].swizzle[2] = NV2ASW_Z;
  out->mac.inputs[0].swizzle[3] = NV2ASW_W;
  out->mac.inputs[1].type = NV2ART_NONE;
  out->mac.inputs[1].swizzle[0] = NV2ASW_X;
  out->mac.inputs[1].swizzle[1] = NV2ASW_Y;
  out->mac.inputs[1].swizzle[2] = NV2ASW_Z;
  out->mac.inputs[1].swizzle[3] = NV2ASW_W;
  out->mac.inputs[2].type = NV2ART_NONE;
  out->mac.inputs[2].swizzle[0] = NV2ASW_X;
  out->mac.inputs[2].swizzle[1] = NV2ASW_Y;
  out->mac.inputs[2].swizzle[2] = NV2ASW_Z;
  out->mac.inputs[2].swizzle[3] = NV2ASW_W;
  out->mac.outputs[0].type = NV2ART_NONE;
  out->mac.outputs[1].type = NV2ART_NONE;

  out->ilu.opcode = NV2AOP_NOP;
  out->ilu.inputs[0].type = NV2ART_NONE;
  out->ilu.inputs[0].swizzle[0] = NV2ASW_X;
  out->ilu.inputs[0].swizzle[1] = NV2ASW_Y;
  out->ilu.inputs[0].swizzle[2] = NV2ASW_Z;
  out->ilu.inputs[0].swizzle[3] = NV2ASW_W;
  out->ilu.inputs[1].type = NV2ART_NONE;
  out->ilu.inputs[1].swizzle[0] = NV2ASW_X;
  out->ilu.inputs[1].swizzle[1] = NV2ASW_Y;
  out->ilu.inputs[1].swizzle[2] = NV2ASW_Z;
  out->ilu.inputs[1].swizzle[3] = NV2ASW_W;
  out->ilu.inputs[2].type = NV2ART_NONE;
  out->ilu.inputs[2].swizzle[0] = NV2ASW_X;
  out->ilu.inputs[2].swizzle[1] = NV2ASW_Y;
  out->ilu.inputs[2].swizzle[2] = NV2ASW_Z;
  out->ilu.inputs[2].swizzle[3] = NV2ASW_W;
  out->ilu.outputs[0].type = NV2ART_NONE;
  out->ilu.outputs[1].type = NV2ART_NONE;
}

BOOST_AUTO_TEST_SUITE(basic_operation_suite)

BOOST_AUTO_TEST_CASE(step_trivial) {
  Nv2aVshCPUFullExecutionState full_state;
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  full_state.input_regs[11].reg.x = 123.0f;
  full_state.input_regs[11].reg.y = -456.0f;
  full_state.input_regs[11].reg.z = 0.789f;
  full_state.input_regs[11].reg.w = 32.64f;

  // MOV oT2.xyzw, v11
  Nv2aVshStep step;
  clear_step(&step);
  step.mac.opcode = NV2AOP_MOV;
  step.mac.outputs[0].type = NV2ART_OUTPUT;
  step.mac.outputs[0].index = 11;
  step.mac.outputs[0].writemask = NV2AWM_XYZW;
  step.mac.inputs[0].type = NV2ART_INPUT;
  step.mac.inputs[0].index = 11;

  nv2a_vsh_emu_apply(&state, &step);

  CHECK_REGISTER(state.output_regs, NV2AOR_TEX2, full_state.input_regs[11].raw);
}

BOOST_AUTO_TEST_SUITE_END()
