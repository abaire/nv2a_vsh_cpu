#include <boost/test/unit_test.hpp>

#include "nv2a_vsh_disassembler.h"
#include "nv2a_vsh_emulator.h"

#define CHECK_REGISTER(bank, index, actual)   \
  do {                                        \
    float* expected = bank + index * 4;       \
    BOOST_TEST((expected)[0] == (actual)[0]); \
    BOOST_TEST((expected)[1] == (actual)[1]); \
    BOOST_TEST((expected)[2] == (actual)[2]); \
    BOOST_TEST((expected)[3] == (actual)[3]); \
  } while (0)

static void clear_step(Nv2aVshStep* out) {
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
  uint32_t reg = 11 * 4;
  full_state.input_regs[reg + 0] = 123.0f;
  full_state.input_regs[reg + 1] = -456.0f;
  full_state.input_regs[reg + 2] = 0.789f;
  full_state.input_regs[reg + 3] = 32.64f;

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

  CHECK_REGISTER(state.output_regs, NV2AOR_TEX2, &full_state.input_regs[reg]);
}

BOOST_AUTO_TEST_CASE(program_context_tracked) {
  Nv2aVshCPUFullExecutionState full_state;
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  uint32_t reg = 11 * 4;
  full_state.input_regs[reg + 0] = 123.0f;
  full_state.input_regs[reg + 1] = -456.0f;
  full_state.input_regs[reg + 2] = 0.789f;
  full_state.input_regs[reg + 3] = 32.64f;

  // MOV c1.xyzw, v11
  Nv2aVshStep steps[1];
  Nv2aVshStep* step = &steps[0];
  clear_step(step);
  step->mac.opcode = NV2AOP_MOV;
  step->mac.outputs[0].type = NV2ART_CONTEXT;
  step->mac.outputs[0].index = 1;
  step->mac.outputs[0].writemask = NV2AWM_XYZW;
  step->mac.inputs[0].type = NV2ART_INPUT;
  step->mac.inputs[0].index = 11;
  step->is_final = true;

  Nv2aVshProgram program;
  program.steps = steps;

  bool context_dirty[192] = {false};
  nv2a_vsh_emu_execute_track_context_writes(&state, &program, context_dirty);

  CHECK_REGISTER(state.context_regs, 1, &full_state.input_regs[reg]);
  BOOST_TEST(!context_dirty[0]);
  BOOST_TEST(context_dirty[1]);
  for (uint32_t i = 2; i < 192; ++i) {
    BOOST_TEST_INFO(i);
    BOOST_TEST(!context_dirty[i]);
  }
}

BOOST_AUTO_TEST_CASE(arl_relative_context_read) {
  Nv2aVshCPUFullExecutionState full_state;
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  for (uint32_t i = 0; i < 192 * 4; ++i) {
    full_state.context_regs[i] = static_cast<float>(i);
  }
  full_state.temp_regs[0] = 3.0f;

  // ARL A0, R0.x
  static constexpr uint32_t kARL[] = {0x00000000, 0x01A00000, 0x0436106C,
                                      0x20700FF8};
  Nv2aVshStep step;
  BOOST_TEST(nv2a_vsh_parse_step(&step, kARL) == NV2AVPR_SUCCESS);
  nv2a_vsh_emu_apply(&state, &step);
  BOOST_TEST(state.address_reg[0] == 3.0f);

  // MOV oFog.xyzw, c[A0+2]
  clear_step(&step);
  step.mac.opcode = NV2AOP_MOV;
  step.mac.outputs[0].type = NV2ART_OUTPUT;
  step.mac.outputs[0].index = NV2AOR_FOG_COORD;
  step.mac.outputs[0].writemask = NV2AWM_XYZW;
  step.mac.inputs[0].type = NV2ART_CONTEXT;
  step.mac.inputs[0].index = 2;
  step.mac.inputs[0].is_relative = true;
  nv2a_vsh_emu_apply(&state, &step);

  CHECK_REGISTER(state.output_regs, NV2AOR_FOG_COORD,
                 &full_state.context_regs[5 * 4]);
}

BOOST_AUTO_TEST_CASE(paired_mac_ilu_reads_before_writes) {
  Nv2aVshCPUFullExecutionState full_state;
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  memset(full_state.context_regs, 0, sizeof(full_state.context_regs));
  float* v0 = &full_state.input_regs[0 * 4];
  v0[0] = 2.0f;
  v0[1] = 3.0f;
  v0[2] = 4.0f;
  v0[3] = 5.0f;
  float* v1 = &full_state.input_regs[1 * 4];
  v1[0] = 0.5f;
  v1[1] = 2.0f;
  v1[2] = 0.25f;
  v1[3] = -1.0f;
  float* r0 = &full_state.temp_regs[0 * 4];
  r0[0] = 4.0f;
  r0[1] = 0.0f;
  r0[2] = 0.0f;
  r0[3] = 0.0f;

  // MUL R0.xyzw, v0, v1 + RCP c3.xyzw, R0.x
  Nv2aVshStep steps[1];
  Nv2aVshStep* step = &steps[0];
  clear_step(step);
  step->mac.opcode = NV2AOP_MUL;
  step->mac.outputs[0].type = NV2ART_TEMPORARY;
  step->mac.outputs[0].index = 0;
  step->mac.outputs[0].writemask = NV2AWM_XYZW;
  step->mac.inputs[0].type = NV2ART_INPUT;
  step->mac.inputs[0].index = 0;
  step->mac.inputs[1].type = NV2ART_INPUT;
  step->mac.inputs[1].index = 1;

  step->ilu.opcode = NV2AOP_RCP;
  step->ilu.outputs[0].type = NV2ART_CONTEXT;
  step->ilu.outputs[0].index = 3;
  step->ilu.outputs[0].writemask = NV2AWM_XYZW;
  step->ilu.inputs[0].type = NV2ART_TEMPORARY;
  step->ilu.inputs[0].index = 0;
  step->ilu.inputs[0].swizzle[1] = NV2ASW_X;
  step->ilu.inputs[0].swizzle[2] = NV2ASW_X;
  step->ilu.inputs[0].swizzle[3] = NV2ASW_X;
  step->is_final = true;

  Nv2aVshProgram program;
  program.steps = steps;

  bool context_dirty[192] = {false};
  nv2a_vsh_emu_execute_track_context_writes(&state, &program, context_dirty);

  float expected_r0[] = {1.0f, 6.0f, 1.0f, -5.0f};
  CHECK_REGISTER(state.temp_regs, 0, expected_r0);

  // The ILU must see R0 as it was before the MAC wrote to it (1/4, not 1/1).
  float expected_c3[] = {0.25f, 0.25f, 0.25f, 0.25f};
  CHECK_REGISTER(state.context_regs, 3, expected_c3);

  for (uint32_t i = 0; i < 192; ++i) {
    BOOST_TEST_INFO(i);
    BOOST_TEST(context_dirty[i] == (i == 3));
  }
}

BOOST_AUTO_TEST_SUITE_END()
