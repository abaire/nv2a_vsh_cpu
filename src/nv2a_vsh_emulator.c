#include "nv2a_vsh_emulator.h"

#include <assert.h>
#include <string.h>

static inline void set_register(Nv2aVshRegister *out, const Nv2aVshRegister *in,
                                const uint8_t *swizzle, bool negate) {
  float mult = negate ? -1.0f : 1.0f;
  out->reg.x = mult * in->raw[swizzle[0]];
  out->reg.y = mult * in->raw[swizzle[1]];
  out->reg.z = mult * in->raw[swizzle[2]];
  out->reg.w = mult * in->raw[swizzle[3]];
}

static inline void fetch_value(Nv2aVshRegister *out,
                               const Nv2aVshExecutionState *state,
                               const Nv2aVshInput *input) {
  const Nv2aVshRegister *in;

  switch (input->type) {
    default:
      assert(!"Invalid input type");

    case NV2ART_TEMPORARY:
      if (input->index == 12) {
        in = (const Nv2aVshRegister *)state->output_regs;
      } else {
        in = (const Nv2aVshRegister *)(state->temp_regs + input->index * 4);
      }
      break;

    case NV2ART_INPUT:
      in = (const Nv2aVshRegister *)(state->input_regs + input->index * 4);
      break;

    case NV2ART_CONTEXT: {
      uint32_t offset = input->index;
      if (input->is_relative) {
        offset += (int)state->address_reg[0];
      }
      in = (const Nv2aVshRegister *)(state->context_regs + offset * 4);
    } break;
  }

  set_register(out, in, input->swizzle, input->is_negated);
}

static inline void apply_operation(Nv2aVshExecutionState *state,
                                   const Nv2aVshOperation *op,
                                   const Nv2aVshRegister *inputs) {
  Nv2aVshRegister output;

  switch (op->opcode) {
    case NV2AOP_NOP:
      return;

    case NV2AOP_MOV:
      nv2a_vsh_cpu_mov(&output, inputs);
      break;

    case NV2AOP_MUL:
    case NV2AOP_ADD:
    case NV2AOP_MAD:
    case NV2AOP_DP3:
    case NV2AOP_DPH:
    case NV2AOP_DP4:
    case NV2AOP_DST:
    case NV2AOP_MIN:
    case NV2AOP_MAX:
    case NV2AOP_SLT:
    case NV2AOP_SGE:
    case NV2AOP_ARL:
    case NV2AOP_RCP:
    case NV2AOP_RCC:
    case NV2AOP_RSQ:
    case NV2AOP_EXP:
    case NV2AOP_LOG:
    case NV2AOP_LIT:
      break;
  }

  const Nv2aVshOutput *out = op->outputs;
  for (uint32_t i = 0; i < 2; ++i, ++out) {
    Nv2aVshRegister *outreg;
    switch (out->type) {
      case NV2ART_INPUT:
        assert(!"Attempt to write to input register.");

      case NV2ART_NONE:
        continue;

      case NV2ART_OUTPUT:
        assert(out->index < 13 && "Invalid output register target.");
        outreg = (Nv2aVshRegister *)(state->output_regs + out->index * 4);
        break;

      case NV2ART_TEMPORARY:
        assert(out->index < 12 && "Invalid temp register target.");
        outreg = (Nv2aVshRegister *)(state->temp_regs + out->index * 4);
        break;

      case NV2ART_CONTEXT:
        assert(out->index < 192 && "Invalid context register target.");
        outreg = (Nv2aVshRegister *)(state->context_regs + out->index * 4);
        break;

      case NV2ART_ADDRESS:
        outreg = (Nv2aVshRegister *)&state->address_reg;
        break;
    }

    if (out->writemask & NV2AWM_X) {
      outreg->reg.x = output.reg.x;
    }
    if (out->writemask & NV2AWM_Y) {
      outreg->reg.y = output.reg.y;
    }
    if (out->writemask & NV2AWM_Z) {
      outreg->reg.z = output.reg.z;
    }
    if (out->writemask & NV2AWM_W) {
      outreg->reg.w = output.reg.w;
    }
  }
}

static inline void apply(Nv2aVshExecutionState *state,
                         const Nv2aVshStep *step) {
  // Copy the inputs for both operations first to prevent introducing order
  // dependent behavior.
  Nv2aVshRegister mac_inputs[3];
  Nv2aVshRegister ilu_input;
  if (step->mac.opcode) {
    for (uint32_t i = 0; i < 3; ++i) {
      if (step->mac.inputs[i].type == NV2ART_NONE) {
        break;
      }
      fetch_value(&mac_inputs[i], state, &step->mac.inputs[i]);
    }
  }
  if (step->ilu.opcode) {
    fetch_value(&ilu_input, state, &step->ilu.inputs[0]);
  }

  if (step->mac.opcode) {
    apply_operation(state, &step->mac, mac_inputs);
  }
  if (step->ilu.opcode) {
    apply_operation(state, &step->ilu, &ilu_input);
  }
}

void nv2a_vsh_emu_execute(Nv2aVshExecutionState *state,
                          const Nv2aVshProgram *program) {
  assert(state);
  assert(program && program->steps);

  Nv2aVshStep *step = program->steps;
  while (true) {
    apply(state, step);
    if (step->is_final) {
      break;
    }
    ++step;
  }
}

void nv2a_vsh_emu_apply(Nv2aVshExecutionState *state, const Nv2aVshStep *step) {
  apply(state, step);
}
