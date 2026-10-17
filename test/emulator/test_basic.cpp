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

// Sets an input that was reset by clear_step, keeping its .xyzw swizzle.
static void set_input(Nv2aVshInput* in, Nv2aVshRegisterType type,
                      uint32_t index) {
  in->type = type;
  in->index = index;
}

static void set_output(Nv2aVshOutput* out, Nv2aVshRegisterType type,
                       uint32_t index,
                       Nv2aVshWritemask writemask = NV2AWM_XYZW) {
  out->type = type;
  out->index = index;
  out->writemask = writemask;
}

static void set_register(float* bank, uint32_t index, float x, float y, float z,
                         float w) {
  float* reg = bank + index * 4;
  reg[0] = x;
  reg[1] = y;
  reg[2] = z;
  reg[3] = w;
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

BOOST_AUTO_TEST_CASE(execute_stops_at_final) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 1.0f, 2.0f, 3.0f, 4.0f);

  Nv2aVshStep steps[3];
  // MOV R1, v0
  clear_step(&steps[0]);
  steps[0].mac.opcode = NV2AOP_MOV;
  set_output(&steps[0].mac.outputs[0], NV2ART_TEMPORARY, 1);
  set_input(&steps[0].mac.inputs[0], NV2ART_INPUT, 0);

  // MOV oPos, R1 (final)
  clear_step(&steps[1]);
  steps[1].mac.opcode = NV2AOP_MOV;
  set_output(&steps[1].mac.outputs[0], NV2ART_OUTPUT, NV2AOR_POS);
  set_input(&steps[1].mac.inputs[0], NV2ART_TEMPORARY, 1);
  steps[1].is_final = true;

  // MOV oFog, v0 - must not execute since it follows the final step.
  clear_step(&steps[2]);
  steps[2].mac.opcode = NV2AOP_MOV;
  set_output(&steps[2].mac.outputs[0], NV2ART_OUTPUT, NV2AOR_FOG_COORD);
  set_input(&steps[2].mac.inputs[0], NV2ART_INPUT, 0);

  Nv2aVshProgram program;
  program.steps = steps;
  nv2a_vsh_emu_execute(&state, &program);

  CHECK_REGISTER(state.temp_regs, 1, &full_state.input_regs[0]);
  CHECK_REGISTER(state.output_regs, NV2AOR_POS, &full_state.input_regs[0]);
  float zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
  CHECK_REGISTER(state.output_regs, NV2AOR_FOG_COORD, zero);
}

BOOST_AUTO_TEST_CASE(track_context_writes_multi_step) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 1.0f, 2.0f, 3.0f, 4.0f);
  set_register(full_state.input_regs, 1, -1.0f, -2.0f, -3.0f, -4.0f);

  Nv2aVshStep steps[3];
  // MOV c2, v0
  clear_step(&steps[0]);
  steps[0].mac.opcode = NV2AOP_MOV;
  set_output(&steps[0].mac.outputs[0], NV2ART_CONTEXT, 2);
  set_input(&steps[0].mac.inputs[0], NV2ART_INPUT, 0);

  // MOV c7, v1 (final)
  clear_step(&steps[1]);
  steps[1].mac.opcode = NV2AOP_MOV;
  set_output(&steps[1].mac.outputs[0], NV2ART_CONTEXT, 7);
  set_input(&steps[1].mac.inputs[0], NV2ART_INPUT, 1);
  steps[1].is_final = true;

  // MOV c9, v0 - must not execute since it follows the final step.
  clear_step(&steps[2]);
  steps[2].mac.opcode = NV2AOP_MOV;
  set_output(&steps[2].mac.outputs[0], NV2ART_CONTEXT, 9);
  set_input(&steps[2].mac.inputs[0], NV2ART_INPUT, 0);

  Nv2aVshProgram program;
  program.steps = steps;
  bool context_dirty[192] = {false};
  nv2a_vsh_emu_execute_track_context_writes(&state, &program, context_dirty);

  CHECK_REGISTER(state.context_regs, 2, &full_state.input_regs[0]);
  CHECK_REGISTER(state.context_regs, 7, &full_state.input_regs[4]);
  float zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
  CHECK_REGISTER(state.context_regs, 9, zero);
  for (uint32_t i = 0; i < 192; ++i) {
    BOOST_TEST_INFO(i);
    BOOST_TEST(context_dirty[i] == (i == 2 || i == 7));
  }
}

BOOST_AUTO_TEST_CASE(ilu_only_step) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 4.0f, 0.0f, 0.0f, 0.0f);

  // RCP R0, v0.x
  Nv2aVshStep step;
  clear_step(&step);
  step.ilu.opcode = NV2AOP_RCP;
  set_output(&step.ilu.outputs[0], NV2ART_TEMPORARY, 0);
  set_input(&step.ilu.inputs[0], NV2ART_INPUT, 0);
  nv2a_vsh_emu_apply(&state, &step);

  float expected[] = {0.25f, 0.25f, 0.25f, 0.25f};
  CHECK_REGISTER(state.temp_regs, 0, expected);
}

BOOST_AUTO_TEST_CASE(mac_second_output_marks_context_dirty) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 1.0f, 2.0f, 3.0f, 4.0f);

  // MOV R2, v0 + MOV c10, v0
  Nv2aVshStep steps[1];
  clear_step(&steps[0]);
  steps[0].mac.opcode = NV2AOP_MOV;
  set_output(&steps[0].mac.outputs[0], NV2ART_TEMPORARY, 2);
  set_output(&steps[0].mac.outputs[1], NV2ART_CONTEXT, 10);
  set_input(&steps[0].mac.inputs[0], NV2ART_INPUT, 0);
  steps[0].is_final = true;

  Nv2aVshProgram program;
  program.steps = steps;
  bool context_dirty[192] = {false};
  nv2a_vsh_emu_execute_track_context_writes(&state, &program, context_dirty);

  CHECK_REGISTER(state.temp_regs, 2, &full_state.input_regs[0]);
  CHECK_REGISTER(state.context_regs, 10, &full_state.input_regs[0]);
  for (uint32_t i = 0; i < 192; ++i) {
    BOOST_TEST_INFO(i);
    BOOST_TEST(context_dirty[i] == (i == 10));
  }
}

BOOST_AUTO_TEST_CASE(ilu_second_output_marks_context_dirty) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 4.0f, 0.0f, 0.0f, 0.0f);

  // RCP R3, v0.x + RCP c20, v0.x
  Nv2aVshStep steps[1];
  clear_step(&steps[0]);
  steps[0].ilu.opcode = NV2AOP_RCP;
  set_output(&steps[0].ilu.outputs[0], NV2ART_TEMPORARY, 3);
  set_output(&steps[0].ilu.outputs[1], NV2ART_CONTEXT, 20);
  set_input(&steps[0].ilu.inputs[0], NV2ART_INPUT, 0);
  steps[0].is_final = true;

  Nv2aVshProgram program;
  program.steps = steps;
  bool context_dirty[192] = {false};
  nv2a_vsh_emu_execute_track_context_writes(&state, &program, context_dirty);

  float expected[] = {0.25f, 0.25f, 0.25f, 0.25f};
  CHECK_REGISTER(state.temp_regs, 3, expected);
  CHECK_REGISTER(state.context_regs, 20, expected);
  for (uint32_t i = 0; i < 192; ++i) {
    BOOST_TEST_INFO(i);
    BOOST_TEST(context_dirty[i] == (i == 20));
  }
}

BOOST_AUTO_TEST_CASE(r12_reads_output_position) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.output_regs, NV2AOR_POS, 1.0f, 2.0f, 3.0f, 4.0f);

  // MOV oD0, R12
  Nv2aVshStep step;
  clear_step(&step);
  step.mac.opcode = NV2AOP_MOV;
  set_output(&step.mac.outputs[0], NV2ART_OUTPUT, NV2AOR_DIFFUSE);
  set_input(&step.mac.inputs[0], NV2ART_TEMPORARY, 12);
  nv2a_vsh_emu_apply(&state, &step);

  CHECK_REGISTER(state.output_regs, NV2AOR_DIFFUSE,
                 &full_state.output_regs[NV2AOR_POS * 4]);
}

BOOST_AUTO_TEST_CASE(xss_state_writes_external_context) {
  Nv2aVshCPUXVSSExecutionState xss_state = {};
  float context_regs[192 * 4] = {0.0f};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_xss_execution_state(&xss_state, context_regs);
  BOOST_TEST(state.context_regs == context_regs);
  set_register(xss_state.input_regs, 0, 1.0f, 2.0f, 3.0f, 4.0f);

  // MOV c5, v0
  Nv2aVshStep step;
  clear_step(&step);
  step.mac.opcode = NV2AOP_MOV;
  set_output(&step.mac.outputs[0], NV2ART_CONTEXT, 5);
  set_input(&step.mac.inputs[0], NV2ART_INPUT, 0);
  nv2a_vsh_emu_apply(&state, &step);

  CHECK_REGISTER(context_regs, 5, xss_state.input_regs);
}

BOOST_AUTO_TEST_CASE(fog_write_uses_most_significant_masked_component) {
  struct TestCase {
    Nv2aVshWritemask writemask;
    float expected[4];
  };
  static constexpr TestCase kTests[] = {
      {NV2AWM_XYZW, {1.0f, 2.0f, 3.0f, 4.0f}},
      {NV2AWM_XZ, {1.0f, -1.0f, 3.0f, -1.0f}},
      {NV2AWM_YZW, {2.0f, 2.0f, 3.0f, 4.0f}},
      {NV2AWM_Y, {2.0f, 2.0f, -1.0f, -1.0f}},
      {NV2AWM_ZW, {3.0f, -1.0f, 3.0f, 4.0f}},
      {NV2AWM_Z, {3.0f, -1.0f, 3.0f, -1.0f}},
      {NV2AWM_W, {4.0f, -1.0f, -1.0f, 4.0f}},
  };

  for (const auto& test : kTests) {
    BOOST_TEST_INFO(test.writemask);
    Nv2aVshCPUFullExecutionState full_state = {};
    Nv2aVshExecutionState state =
        nv2a_vsh_emu_initialize_full_execution_state(&full_state);
    set_register(full_state.input_regs, 0, 1.0f, 2.0f, 3.0f, 4.0f);
    set_register(full_state.output_regs, NV2AOR_FOG_COORD, -1.0f, -1.0f, -1.0f,
                 -1.0f);

    // MOV oFog.<mask>, v0
    Nv2aVshStep step;
    clear_step(&step);
    step.mac.opcode = NV2AOP_MOV;
    set_output(&step.mac.outputs[0], NV2ART_OUTPUT, NV2AOR_FOG_COORD,
               test.writemask);
    set_input(&step.mac.inputs[0], NV2ART_INPUT, 0);
    nv2a_vsh_emu_apply(&state, &step);

    CHECK_REGISTER(state.output_regs, NV2AOR_FOG_COORD, test.expected);
  }
}

BOOST_AUTO_TEST_CASE(
    fog_write_from_ilu_uses_most_significant_masked_component) {
  Nv2aVshCPUFullExecutionState full_state = {};
  Nv2aVshExecutionState state =
      nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  set_register(full_state.input_regs, 0, 1.0f, 2.0f, 4.0f, 8.0f);

  // RCP oFog.w, v0.z
  Nv2aVshStep step;
  clear_step(&step);
  step.ilu.opcode = NV2AOP_RCP;
  set_output(&step.ilu.outputs[0], NV2ART_OUTPUT, NV2AOR_FOG_COORD, NV2AWM_W);
  set_input(&step.ilu.inputs[0], NV2ART_INPUT, 0);
  step.ilu.inputs[0].swizzle[0] = NV2ASW_Z;
  step.ilu.inputs[0].swizzle[1] = NV2ASW_Z;
  step.ilu.inputs[0].swizzle[2] = NV2ASW_Z;
  step.ilu.inputs[0].swizzle[3] = NV2ASW_Z;
  nv2a_vsh_emu_apply(&state, &step);

  float expected[] = {0.25f, 0.0f, 0.0f, 0.25f};
  CHECK_REGISTER(state.output_regs, NV2AOR_FOG_COORD, expected);
}

BOOST_AUTO_TEST_SUITE_END()
