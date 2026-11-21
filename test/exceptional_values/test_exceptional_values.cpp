// Checks emulator results against bit-exact values measured on Xbox NV2A
// hardware for signed zeros, infinities, NaNs, denormals and the float32 range
// edges. See expectations.json and generate_expectations.py.
//
// Operands are carried as uint32_t and copied into the register file with
// memcpy so that no NaN payload passes through host float math before it
// reaches the emulator.

#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

#include "exceptional_values/expectations.h"
#include "nv2a_vsh_disassembler.h"
#include "nv2a_vsh_emulator.h"

using exceptional_values::Expectation;
using exceptional_values::kAddressOutOfRange;
using exceptional_values::kExpectations;
using exceptional_values::kNotAddress;

namespace {

constexpr uint32_t kNumExpectations =
    sizeof(kExpectations) / sizeof(kExpectations[0]);

constexpr uint32_t kOutputRegister = 3;
constexpr uint32_t kNumMarkerRegisters = 8;

struct Location {
  Nv2aVshRegisterType type;
  uint32_t index;
};

void clear_input(Nv2aVshInput* in) {
  memset(in, 0, sizeof(*in));
  in->type = NV2ART_NONE;
  in->swizzle[0] = NV2ASW_X;
  in->swizzle[1] = NV2ASW_Y;
  in->swizzle[2] = NV2ASW_Z;
  in->swizzle[3] = NV2ASW_W;
}

void clear_operation(Nv2aVshOperation* op) {
  op->opcode = NV2AOP_NOP;
  memset(op->outputs, 0, sizeof(op->outputs));
  for (auto& in : op->inputs) {
    clear_input(&in);
  }
}

void clear_step(Nv2aVshStep* step) {
  step->is_final = false;
  clear_operation(&step->mac);
  clear_operation(&step->ilu);
}

void set_input(Nv2aVshInput* in, Location loc, uint8_t swizzle = 0xFF) {
  in->type = loc.type;
  in->index = loc.index;
  if (swizzle != 0xFF) {
    memset(in->swizzle, swizzle, sizeof(in->swizzle));
  }
}

void set_output(Nv2aVshOutput* out, Location loc,
                Nv2aVshWritemask writemask = NV2AWM_XYZW) {
  out->type = loc.type;
  out->index = loc.index;
  out->writemask = writemask;
}

bool is_ilu(Nv2aVshOpcode opcode) {
  return opcode >= NV2AOP_RCP && opcode <= NV2AOP_LIT;
}

// Builds a step that applies `opcode` to `inputs` and writes `out`.
Nv2aVshStep make_step(Nv2aVshOpcode opcode, std::initializer_list<Location> ins,
                      Location out, Nv2aVshWritemask writemask = NV2AWM_XYZW) {
  Nv2aVshStep step;
  clear_step(&step);
  Nv2aVshOperation* op = is_ilu(opcode) ? &step.ilu : &step.mac;
  op->opcode = opcode;
  uint32_t i = 0;
  for (const auto& in : ins) {
    set_input(&op->inputs[i++], in);
  }
  set_output(&op->outputs[0], out, writemask);
  return step;
}

float* reg(float* bank, uint32_t index) { return bank + index * 4; }

void load(float* dst, const uint32_t* bits) { memcpy(dst, bits, 4 * 4); }

uint32_t bits_of(const float* value) {
  uint32_t ret;
  memcpy(&ret, value, sizeof(ret));
  return ret;
}

uint32_t marker_bits(uint32_t index) { return 0x42C80000u + (index << 16); }

void run(Nv2aVshExecutionState* state, Nv2aVshStep* steps, uint32_t count) {
  steps[count - 1].is_final = true;
  Nv2aVshProgram program;
  program.steps = steps;
  nv2a_vsh_emu_execute(state, &program);
}

struct Harness {
  Harness() {
    state = nv2a_vsh_emu_initialize_full_execution_state(&full_state);
  }

  Nv2aVshCPUFullExecutionState full_state;
  Nv2aVshExecutionState state;
};

const Location kC0{NV2ART_CONTEXT, 0};
const Location kC1{NV2ART_CONTEXT, 1};
const Location kC2{NV2ART_CONTEXT, 2};
const Location kOut{NV2ART_CONTEXT, kOutputRegister};

struct OpInfo {
  const char* name;
  Nv2aVshOpcode opcode;
  uint32_t num_inputs;
};

const OpInfo kSimpleOps[] = {
    {"MUL", NV2AOP_MUL, 2},  {"ADD", NV2AOP_ADD, 2}, {"MAD", NV2AOP_MAD, 3},
    {"DP3", NV2AOP_DP3, 2},  {"DPH", NV2AOP_DPH, 2}, {"DP4", NV2AOP_DP4, 2},
    {"MIN", NV2AOP_MIN, 2},  {"MAX", NV2AOP_MAX, 2}, {"SLT", NV2AOP_SLT, 2},
    {"SGE", NV2AOP_SGE, 2},  {"DST", NV2AOP_DST, 2}, {"RCP", NV2AOP_RCP, 1},
    {"RCC", NV2AOP_RCC, 1},  {"RSQ", NV2AOP_RSQ, 1}, {"EXPP", NV2AOP_EXP, 1},
    {"LOGP", NV2AOP_LOG, 1}, {"LIT", NV2AOP_LIT, 1},
};

const OpInfo* find_simple_op(const std::string& name) {
  for (const auto& info : kSimpleOps) {
    if (name == info.name) {
      return &info;
    }
  }
  return nullptr;
}

// Runs `OP c3, c0[, c1[, c2]]` with a, b and c in c0, c1 and c2.
bool run_simple(const Expectation& e, uint32_t* result) {
  const OpInfo* info = find_simple_op(e.op);
  if (!info) {
    return false;
  }
  Harness h;
  load(reg(h.state.context_regs, 0), e.a);
  load(reg(h.state.context_regs, 1), e.b);
  load(reg(h.state.context_regs, 2), e.c);

  Nv2aVshStep step;
  switch (info->num_inputs) {
    case 1:
      step = make_step(info->opcode, {kC0}, kOut);
      break;
    case 2:
      step = make_step(info->opcode, {kC0, kC1}, kOut);
      break;
    default:
      step = make_step(info->opcode, {kC0, kC1, kC2}, kOut);
      break;
  }
  run(&h.state, &step, 1);
  *result = bits_of(reg(h.state.context_regs, kOutputRegister) + e.component);
  return true;
}

// Runs the attribute-vs-constant "paths" programs from the constant file.
bool run_path(const Expectation& e, uint32_t* result) {
  static const std::string kPrefix = "const ";
  std::string op = e.op;
  if (op.compare(0, kPrefix.size(), kPrefix)) {
    return false;
  }
  op = op.substr(kPrefix.size());

  struct PathOp {
    const char* name;
    Nv2aVshOpcode opcode;
    uint32_t num_inputs;
    Location inputs[3];
  };
  static const PathOp kPathOps[] = {
      {"A * B", NV2AOP_MUL, 2, {kC0, kC1}},
      {"B * A", NV2AOP_MUL, 2, {kC1, kC0}},
      {"A * B + C", NV2AOP_MAD, 3, {kC0, kC1, kC2}},
      {"B * A + C", NV2AOP_MAD, 3, {kC1, kC0, kC2}},
      {"A + B", NV2AOP_ADD, 2, {kC0, kC1}},
      {"DP3(A, B)", NV2AOP_DP3, 2, {kC0, kC1}},
      {"DP4(A, B)", NV2AOP_DP4, 2, {kC0, kC1}},
      {"DPH(A, B)", NV2AOP_DPH, 2, {kC0, kC1}},
      {"DPH(B, A)", NV2AOP_DPH, 2, {kC1, kC0}},
      {"MIN(A, B)", NV2AOP_MIN, 2, {kC0, kC1}},
      {"MAX(A, B)", NV2AOP_MAX, 2, {kC0, kC1}},
  };

  for (const auto& path : kPathOps) {
    if (op != path.name) {
      continue;
    }
    Harness h;
    load(reg(h.state.context_regs, 0), e.a);
    load(reg(h.state.context_regs, 1), e.b);
    load(reg(h.state.context_regs, 2), e.c);
    Nv2aVshStep step = make_step(path.opcode, {}, kOut);
    for (uint32_t i = 0; i < path.num_inputs; ++i) {
      set_input(&step.mac.inputs[i], path.inputs[i]);
    }
    run(&h.state, &step, 1);
    *result = bits_of(reg(h.state.context_regs, kOutputRegister) + e.component);
    return true;
  }
  return false;
}

// Runs `MOV c3, [-]src[.swizzle]` where src is c0, r0 or v1.
bool run_mov(const Expectation& e, uint32_t* result) {
  static const std::string kPrefix = "MOV ";
  std::string op = e.op;
  if (op.compare(0, kPrefix.size(), kPrefix)) {
    return false;
  }
  op = op.substr(kPrefix.size());

  bool negate = false;
  if (!op.empty() && op[0] == '-') {
    negate = true;
    op = op.substr(1);
  }

  std::string swizzle = "xyzw";
  auto dot = op.find('.');
  if (dot != std::string::npos) {
    swizzle = op.substr(dot + 1);
    op = op.substr(0, dot);
  }
  if (swizzle.size() != 4) {
    return false;
  }

  Harness h;
  Location src = kC0;
  float* bank = nullptr;
  if (op == "c") {
    src = kC0;
    bank = h.state.context_regs;
  } else if (op == "r0") {
    src = {NV2ART_TEMPORARY, 0};
    bank = h.state.temp_regs;
  } else if (op == "v1") {
    src = {NV2ART_INPUT, 1};
    bank = h.state.input_regs;
  } else {
    return false;
  }
  load(reg(bank, src.index), e.a);

  Nv2aVshStep step = make_step(NV2AOP_MOV, {src}, kOut);
  Nv2aVshInput* in = &step.mac.inputs[0];
  in->is_negated = negate;
  for (uint32_t i = 0; i < 4; ++i) {
    static const char kComponents[] = "xyzw";
    const char* found = strchr(kComponents, swizzle[i]);
    if (!found || !*found) {
      return false;
    }
    in->swizzle[i] = static_cast<uint8_t>(found - kComponents);
  }

  run(&h.state, &step, 1);
  *result = bits_of(reg(h.state.context_regs, kOutputRegister) + e.component);
  return true;
}

// Runs `ARL a0.x, v0.x; MOV r0, c[a0.x]` with distinct marker values in the
// first constant registers, so the result identifies the register that was
// read. Out-of-range reads produce 0.
bool run_arl(const Expectation& e, uint32_t* result) {
  if (strcmp(e.op, "ARL") != 0) {
    return false;
  }
  Harness h;
  load(reg(h.state.input_regs, 0), e.a);
  for (uint32_t i = 0; i < kNumMarkerRegisters; ++i) {
    uint32_t marker[4];
    for (auto& m : marker) {
      m = marker_bits(i);
    }
    load(reg(h.state.context_regs, i), marker);
  }

  Nv2aVshStep steps[2];
  steps[0] =
      make_step(NV2AOP_ARL, {{NV2ART_INPUT, 0}}, {NV2ART_ADDRESS, 0}, NV2AWM_X);
  memset(steps[0].mac.inputs[0].swizzle, NV2ASW_X,
         sizeof(steps[0].mac.inputs[0].swizzle));
  steps[1] = make_step(NV2AOP_MOV, {kC0}, {NV2ART_TEMPORARY, 0});
  steps[1].mac.inputs[0].is_relative = true;
  run(&h.state, steps, 2);
  *result = bits_of(reg(h.state.temp_regs, 0) + e.component);
  return true;
}

// Runs the fog distance chain with P in c0 or v0:
//   RCP r0.x, P.w
//   MUL r1, P, r0.x
//   DP3 r2.x, r1, r1
//   RSQ r3.x, r2.x
//   MUL r4.x, r2.x, r3.x
bool run_fog_chain(const Expectation& e, uint32_t* result) {
  std::string op = e.op;
  Location p;
  Harness h;
  if (!op.compare(0, 6, "const ")) {
    p = kC0;
    load(reg(h.state.context_regs, 0), e.a);
    op = op.substr(6);
  } else if (!op.compare(0, 5, "attr ")) {
    p = {NV2ART_INPUT, 0};
    load(reg(h.state.input_regs, 0), e.a);
    op = op.substr(5);
  } else {
    return false;
  }

  uint32_t result_reg;
  if (op == "rcp(P.w)") {
    result_reg = 0;
  } else if (op == "P * rcp(P.w)") {
    result_reg = 1;
  } else if (op == "d2") {
    result_reg = 2;
  } else if (op == "rsq(d2)") {
    result_reg = 3;
  } else if (op == "d2 * rsq(d2)") {
    result_reg = 4;
  } else {
    return false;
  }

  const Location r0{NV2ART_TEMPORARY, 0};
  const Location r1{NV2ART_TEMPORARY, 1};
  const Location r2{NV2ART_TEMPORARY, 2};
  const Location r3{NV2ART_TEMPORARY, 3};
  const Location r4{NV2ART_TEMPORARY, 4};

  Nv2aVshStep steps[5];
  steps[0] = make_step(NV2AOP_RCP, {p}, r0, NV2AWM_X);
  set_input(&steps[0].ilu.inputs[0], p, NV2ASW_W);
  steps[1] = make_step(NV2AOP_MUL, {p, r0}, r1);
  set_input(&steps[1].mac.inputs[1], r0, NV2ASW_X);
  steps[2] = make_step(NV2AOP_DP3, {r1, r1}, r2, NV2AWM_X);
  steps[3] = make_step(NV2AOP_RSQ, {r2}, r3, NV2AWM_X);
  set_input(&steps[3].ilu.inputs[0], r2, NV2ASW_X);
  steps[4] = make_step(NV2AOP_MUL, {r2, r3}, r4, NV2AWM_X);
  set_input(&steps[4].mac.inputs[0], r2, NV2ASW_X);
  set_input(&steps[4].mac.inputs[1], r3, NV2ASW_X);
  run(&h.state, steps, 5);

  *result = bits_of(reg(h.state.temp_regs, result_reg) + e.component);
  return true;
}

// Every (group, op) pair that has a test case below.
const char* const kCheckedOps[][2] = {
    {"mac", "MUL"},
    {"mac", "ADD"},
    {"mac", "MAD"},
    {"mac", "DP3"},
    {"mac", "DPH"},
    {"mac", "DP4"},
    {"mac", "MIN"},
    {"mac", "MAX"},
    {"mac", "SLT"},
    {"mac", "SGE"},
    {"mac", "DST"},
    {"mov", "MOV c"},
    {"mov", "MOV -c"},
    {"mov", "MOV c.wzyx"},
    {"mov", "MOV -c.yxwz"},
    {"mov", "MOV r0"},
    {"mov", "MOV -r0"},
    {"mov", "MOV v1"},
    {"mov", "MOV -v1"},
    {"ilu", "RCP"},
    {"ilu", "RCC"},
    {"ilu", "RSQ"},
    {"ilu", "EXPP"},
    {"ilu", "LOGP"},
    {"ilu", "LIT"},
    {"ilu", "ARL"},
    {"paths", "const A * B"},
    {"paths", "const B * A"},
    {"paths", "const A * B + C"},
    {"paths", "const B * A + C"},
    {"paths", "const A + B"},
    {"paths", "const DP3(A, B)"},
    {"paths", "const DP4(A, B)"},
    {"paths", "const DPH(A, B)"},
    {"paths", "const DPH(B, A)"},
    {"paths", "const MIN(A, B)"},
    {"paths", "const MAX(A, B)"},
    {"fog_chain", "const rcp(P.w)"},
    {"fog_chain", "const P * rcp(P.w)"},
    {"fog_chain", "const d2"},
    {"fog_chain", "const rsq(d2)"},
    {"fog_chain", "const d2 * rsq(d2)"},
    {"fog_chain", "attr rcp(P.w)"},
    {"fog_chain", "attr P * rcp(P.w)"},
    {"fog_chain", "attr d2"},
    {"fog_chain", "attr rsq(d2)"},
    {"fog_chain", "attr d2 * rsq(d2)"},
};

bool is_checked(const Expectation& e) {
  for (const auto& pair : kCheckedOps) {
    if (!strcmp(e.group, pair[0]) && !strcmp(e.op, pair[1])) {
      return true;
    }
  }
  return false;
}

// Records whose results come from the hardware's reduced-precision
// transcendental approximations (RSQ, LOGP.z and the LIT overflow edge) and are
// not covered by kApproximateOutputs. The emulator does not match hardware for
// these, and there is too little data to model the approximations, so they are
// disabled until they can be verified against more hardware results.
struct DisabledRecord {
  const char* op;
  const char* case_name;
  uint32_t component;
};

const DisabledRecord kUnverifiedApproximations[] = {
    {"LOGP", "1.5", 2},
    {"LOGP", "FLT_MAX", 2},
    // Hardware gives 0x7F7FF88A for 2^128 but +inf for 4^64 and 0.5^-128, so
    // whether a result at the overflow edge is finite depends on the
    // approximation error.
    {"LIT", "N.L 1, N.H 2, power +inf", 2},
    {"LIT", "N.L 1, N.H 2, power 128", 2},
    {"const rsq(d2)", "(1, 2, 3, 1)", 0},
    {"const d2 * rsq(d2)", "(1, 2, 3, 1)", 0},
    {"attr rsq(d2)", "(1, 2, 3, 1)", 0},
    {"attr d2 * rsq(d2)", "(1, 2, 3, 1)", 0},
};

bool is_unverified_approximation(const Expectation& e) {
  for (const auto& d : kUnverifiedApproximations) {
    if (!strcmp(e.op, d.op) && !strcmp(e.case_name, d.case_name) &&
        e.component == d.component) {
      return true;
    }
  }
  return false;
}

// Outputs that the emulator computes with libm while hardware uses its own
// approximation. A finite, normal result of the same sign as hardware passes
// within `max_relative_error`; zeros, infinities, NaNs, denormals and sign
// differences must still match exactly.
struct ApproximateOutput {
  const char* op;
  uint32_t component;
  double max_relative_error;
};

const ApproximateOutput kApproximateOutputs[] = {
    // The largest measured error is 2.5e-3, at N.H = sqrt(2) with powers near
    // +-128.
    // The largest measured error is 1.14e-4, for fractions just below 1.
    {"EXPP", 2, 1.0 / 4096.0},
    {"LIT", 2, 1.0 / 256.0},
};

bool is_finite_normal(uint32_t bits) {
  uint32_t exponent = bits & 0x7F800000u;
  return exponent != 0 && exponent != 0x7F800000u;
}

bool within_tolerance(const Expectation& e, uint32_t actual,
                      uint32_t expected) {
  if (e.hw_address != kNotAddress || !is_finite_normal(actual) ||
      !is_finite_normal(expected) || (actual ^ expected) & 0x80000000u) {
    return false;
  }
  for (const auto& a : kApproximateOutputs) {
    if (!strcmp(e.op, a.op) && e.component == a.component) {
      float actual_value;
      float expected_value;
      memcpy(&actual_value, &actual, sizeof(actual_value));
      memcpy(&expected_value, &expected, sizeof(expected_value));
      double error = fabs(((double)actual_value - expected_value) /
                          (double)expected_value);
      return error <= a.max_relative_error;
    }
  }
  return false;
}

bool is_skipped(const Expectation& e) {
  return !strcmp(e.group, "paths") && !strncmp(e.op, "attr ", 5);
}

bool run_expectation(const Expectation& e, uint32_t* result) {
  std::string group = e.group;
  if (group == "mac" || group == "ilu") {
    return run_simple(e, result) || run_arl(e, result);
  }
  if (group == "mov") {
    return run_mov(e, result);
  }
  if (group == "paths") {
    return run_path(e, result);
  }
  if (group == "fog_chain") {
    return run_fog_chain(e, result);
  }
  return false;
}

uint32_t expected_bits(const Expectation& e) {
  if (e.hw_address == kNotAddress) {
    return e.hw;
  }
  if (e.hw_address == kAddressOutOfRange) {
    return 0;
  }
  return marker_bits(static_cast<uint32_t>(e.hw_address));
}

std::string hex(uint32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "0x%08X", value);
  return buf;
}

std::string describe_failure(const Expectation& e, uint32_t actual) {
  static const char kComponents[] = "xyzw";
  std::ostringstream msg;
  msg << e.op << " [" << e.case_name << "]." << kComponents[e.component]
      << ": actual " << hex(actual) << ", hw ";
  if (e.hw_address == kNotAddress) {
    msg << hex(e.hw);
  } else if (e.hw_address == kAddressOutOfRange) {
    msg << "out of range (0x00000000)";
  } else {
    msg << "c[" << e.hw_address << "] (" << hex(expected_bits(e)) << ")";
  }
  return msg.str();
}

// Checks every record of the given group and op, returning the number checked.
uint32_t check(const char* group, const char* op) {
  uint32_t count = 0;
  uint32_t failures = 0;
  for (const auto& e : kExpectations) {
    if (strcmp(e.group, group) || strcmp(e.op, op)) {
      continue;
    }
    if (is_unverified_approximation(e)) {
      continue;
    }
    ++count;
    uint32_t actual = 0;
    if (!run_expectation(e, &actual)) {
      ++failures;
      BOOST_ERROR("No program for " << e.group << " " << e.op);
      continue;
    }
    uint32_t expected = expected_bits(e);
    bool passed = actual == expected || within_tolerance(e, actual, expected);
    if (!passed) {
      ++failures;
    }
    BOOST_CHECK_MESSAGE(passed, describe_failure(e, actual));
  }
  BOOST_TEST_MESSAGE(group << " " << op << ": " << (count - failures) << "/"
                           << count << " passed");
  BOOST_TEST(count > 0u, "No records for " << group << " " << op);
  return count;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(exceptional_value_suite)

BOOST_AUTO_TEST_SUITE(mac)
BOOST_AUTO_TEST_CASE(mul) { check("mac", "MUL"); }
BOOST_AUTO_TEST_CASE(add) { check("mac", "ADD"); }
BOOST_AUTO_TEST_CASE(mad) { check("mac", "MAD"); }
BOOST_AUTO_TEST_CASE(dp3) { check("mac", "DP3"); }
BOOST_AUTO_TEST_CASE(dph) { check("mac", "DPH"); }
BOOST_AUTO_TEST_CASE(dp4) { check("mac", "DP4"); }
BOOST_AUTO_TEST_CASE(min) { check("mac", "MIN"); }
BOOST_AUTO_TEST_CASE(max) { check("mac", "MAX"); }
BOOST_AUTO_TEST_CASE(slt) { check("mac", "SLT"); }
BOOST_AUTO_TEST_CASE(sge) { check("mac", "SGE"); }
BOOST_AUTO_TEST_CASE(dst) { check("mac", "DST"); }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(mov)
BOOST_AUTO_TEST_CASE(mov_c) { check("mov", "MOV c"); }
BOOST_AUTO_TEST_CASE(mov_neg_c) { check("mov", "MOV -c"); }
BOOST_AUTO_TEST_CASE(mov_c_wzyx) { check("mov", "MOV c.wzyx"); }
BOOST_AUTO_TEST_CASE(mov_neg_c_yxwz) { check("mov", "MOV -c.yxwz"); }
BOOST_AUTO_TEST_CASE(mov_r0) { check("mov", "MOV r0"); }
BOOST_AUTO_TEST_CASE(mov_neg_r0) { check("mov", "MOV -r0"); }
BOOST_AUTO_TEST_CASE(mov_v1) { check("mov", "MOV v1"); }
BOOST_AUTO_TEST_CASE(mov_neg_v1) { check("mov", "MOV -v1"); }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ilu)
BOOST_AUTO_TEST_CASE(rcp) { check("ilu", "RCP"); }
BOOST_AUTO_TEST_CASE(rcc) { check("ilu", "RCC"); }
BOOST_AUTO_TEST_CASE(rsq) { check("ilu", "RSQ"); }
BOOST_AUTO_TEST_CASE(expp) { check("ilu", "EXPP"); }
BOOST_AUTO_TEST_CASE(logp) { check("ilu", "LOGP"); }
BOOST_AUTO_TEST_CASE(lit) { check("ilu", "LIT"); }
BOOST_AUTO_TEST_CASE(arl) { check("ilu", "ARL"); }
BOOST_AUTO_TEST_SUITE_END()

// The hardware gives identical results for attribute and constant sources, and
// the emulator reads both through the same code, so only the const rows run.
BOOST_AUTO_TEST_SUITE(paths)
BOOST_AUTO_TEST_CASE(mul_ab) { check("paths", "const A * B"); }
BOOST_AUTO_TEST_CASE(mul_ba) { check("paths", "const B * A"); }
BOOST_AUTO_TEST_CASE(mad_ab) { check("paths", "const A * B + C"); }
BOOST_AUTO_TEST_CASE(mad_ba) { check("paths", "const B * A + C"); }
BOOST_AUTO_TEST_CASE(add_ab) { check("paths", "const A + B"); }
BOOST_AUTO_TEST_CASE(dp3_ab) { check("paths", "const DP3(A, B)"); }
BOOST_AUTO_TEST_CASE(dp4_ab) { check("paths", "const DP4(A, B)"); }
BOOST_AUTO_TEST_CASE(dph_ab) { check("paths", "const DPH(A, B)"); }
BOOST_AUTO_TEST_CASE(dph_ba) { check("paths", "const DPH(B, A)"); }
BOOST_AUTO_TEST_CASE(min_ab) { check("paths", "const MIN(A, B)"); }
BOOST_AUTO_TEST_CASE(max_ab) { check("paths", "const MAX(A, B)"); }
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(fog_chain)
BOOST_AUTO_TEST_CASE(const_rcp_w) { check("fog_chain", "const rcp(P.w)"); }
BOOST_AUTO_TEST_CASE(const_p_rcp) { check("fog_chain", "const P * rcp(P.w)"); }
BOOST_AUTO_TEST_CASE(const_d2) { check("fog_chain", "const d2"); }
BOOST_AUTO_TEST_CASE(const_rsq) { check("fog_chain", "const rsq(d2)"); }
BOOST_AUTO_TEST_CASE(const_d) { check("fog_chain", "const d2 * rsq(d2)"); }
BOOST_AUTO_TEST_CASE(attr_rcp_w) { check("fog_chain", "attr rcp(P.w)"); }
BOOST_AUTO_TEST_CASE(attr_p_rcp) { check("fog_chain", "attr P * rcp(P.w)"); }
BOOST_AUTO_TEST_CASE(attr_d2) { check("fog_chain", "attr d2"); }
BOOST_AUTO_TEST_CASE(attr_rsq) { check("fog_chain", "attr rsq(d2)"); }
BOOST_AUTO_TEST_CASE(attr_d) { check("fog_chain", "attr d2 * rsq(d2)"); }
BOOST_AUTO_TEST_SUITE_END()

// Fails if a record in the table has no program or test case, e.g. because of
// a new op or a renamed op string, so that coverage cannot silently shrink.
BOOST_AUTO_TEST_CASE(every_record_is_covered) {
  uint32_t covered = 0;
  uint32_t skipped = 0;
  for (const auto& e : kExpectations) {
    if (is_skipped(e)) {
      ++skipped;
      continue;
    }
    uint32_t actual;
    if (!run_expectation(e, &actual)) {
      BOOST_ERROR("No program for " << e.group << " " << e.op);
    } else if (!is_checked(e)) {
      BOOST_ERROR("No test case for " << e.group << " " << e.op);
    } else {
      ++covered;
    }
  }
  BOOST_TEST(covered + skipped == kNumExpectations);
}

// Fails if a disabled entry no longer matches a record, e.g. after the
// expectations are regenerated, so that stale entries are removed.
BOOST_AUTO_TEST_CASE(every_disabled_record_exists) {
  for (const auto& d : kUnverifiedApproximations) {
    bool found = false;
    for (const auto& e : kExpectations) {
      if (!strcmp(e.op, d.op) && !strcmp(e.case_name, d.case_name) &&
          e.component == d.component) {
        found = true;
        break;
      }
    }
    BOOST_TEST(found,
               "No record for disabled " << d.op << " [" << d.case_name << "]");
  }
}

BOOST_AUTO_TEST_SUITE_END()
