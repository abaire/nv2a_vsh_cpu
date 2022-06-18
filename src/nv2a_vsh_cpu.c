#include "nv2a_vsh_cpu.h"

#include <math.h>
#include <string.h>

void nv2a_vsh_cpu_mov(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  memcpy(out, a, sizeof(*out));
}

void nv2a_vsh_cpu_arl(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  float val = floorf(a->reg.x + 0.001f);
  out->reg.x = val;
  out->reg.y = val;
  out->reg.z = val;
  out->reg.w = val;
}

/*

def _arl(inst: dict, input: Context, output: Context):
    # TODO: Validate this behavior on HW.
    val = input.get(inst["inputs"][0])[0]
    val = int(math.floor(val + 0.001))
    output.set(inst["output"], (val, val, val, val))


def _mov(inst: dict, input: Context, output: Context):
    for reg in inst["outputs"]:
        output.set(reg, input.get(inst["inputs"][0]))


def _mac_mul(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val * b_val for a_val, b_val in zip(a, b)]
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_add(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val + b_val for a_val, b_val in zip(a, b)]
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_mad(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val * b_val for a_val, b_val in zip(a, b)]
    c = input.get(inst["inputs"][2])
    result = [a_val + b_val for a_val, b_val in zip(result, c)]
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_dp3(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val * b_val for a_val, b_val in zip(a[:3], b[:3])]

    val = functools.reduce(lambda x, y: x + y, result)
    result = [val] * 4
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_dph(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val * b_val for a_val, b_val in zip(a[:3], b[:3])]

    val = functools.reduce(lambda x, y: x + y, result)
    val += b[4]
    result = [val] * 4
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_dp4(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val * b_val for a_val, b_val in zip(a[:4], b[:4])]

    val = functools.reduce(lambda x, y: x + y, result)
    result = [val] * 4
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_dst(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = (1.0, a[1] * b[1], a[2], b[3])
    for reg in inst["outputs"]:
        output.set(reg, tuple(result))


def _mac_min(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val if a_val < b_val else b_val for a_val, b_val in zip(a[:4],
b[:4])] for reg in inst["outputs"]: output.set(reg, tuple(result))


def _mac_max(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [a_val if a_val >= b_val else b_val for a_val, b_val in zip(a[:4],
b[:4])] for reg in inst["outputs"]: output.set(reg, tuple(result))


def _mac_slt(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [1.0 if a_val < b_val else 0.0 for a_val, b_val in zip(a[:4],
b[:4])] for reg in inst["outputs"]: output.set(reg, tuple(result))


def _mac_sge(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    b = input.get(inst["inputs"][1])
    result = [1.0 if a_val >= b_val else 0.0 for a_val, b_val in zip(a[:4],
b[:4])] for reg in inst["outputs"]: output.set(reg, tuple(result))


_MAC_HANDLERS = {
    nv2avsh.vsh_instruction.MAC.MAC_MOV: _mov,
    nv2avsh.vsh_instruction.MAC.MAC_MUL: _mac_mul,
    nv2avsh.vsh_instruction.MAC.MAC_ADD: _mac_add,
    nv2avsh.vsh_instruction.MAC.MAC_MAD: _mac_mad,
    nv2avsh.vsh_instruction.MAC.MAC_DP3: _mac_dp3,
    nv2avsh.vsh_instruction.MAC.MAC_DPH: _mac_dph,
    nv2avsh.vsh_instruction.MAC.MAC_DP4: _mac_dp4,
    nv2avsh.vsh_instruction.MAC.MAC_DST: _mac_dst,
    nv2avsh.vsh_instruction.MAC.MAC_MIN: _mac_min,
    nv2avsh.vsh_instruction.MAC.MAC_MAX: _mac_max,
    nv2avsh.vsh_instruction.MAC.MAC_SLT: _mac_slt,
    nv2avsh.vsh_instruction.MAC.MAC_SGE: _mac_sge,
    nv2avsh.vsh_instruction.MAC.MAC_ARL: _arl,
}


def _ilu_rcp(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])

    def compute(val):
        if val == 1.0:
            return 1.0

        if val == 0.0:
            return math.inf

        return 1.0 / val

    result = [compute(val) for val in a[:4]]
    for reg in inst["outputs"]:
        output.set(reg, (result[0], result[1], result[2], result[3]))


def _ilu_rcc(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])

    def compute(input):
        if input < -1.84467e19:
            input = -1.84467e19
        elif input > -5.42101e-20 and input < 0:
            input = -5.42101e-020
        elif input >= 0 and input < 5.42101e-20:
            input = 5.42101e-20
        elif input > 1.84467e19:
            input = 1.84467e19

        if input == 1.0:
            return 1.0

        return 1.0 / input

    result = [compute(val) for val in a[:4]]
    for reg in inst["outputs"]:
        output.set(reg, (result[0], result[1], result[2], result[3]))


def _ilu_rsq(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])

    def compute(input):
        if input == 1.0:
            return 1.0

        if input == 0:
            return math.inf

        return 1.0 / math.sqrt(input)

    result = [compute(abs(val)) for val in a[:4]]
    for reg in inst["outputs"]:
        output.set(reg, (result[0], result[1], result[2], result[3]))


def _ilu_exp(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])

    tmp = math.floor(a[0])
    x = math.pow(2, tmp)
    y = a[0] - tmp
    z = math.pow(2, a[0])
    w = 1.0

    for reg in inst["outputs"]:
        output.set(reg, (x, y, z, w))


def _ilu_log(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])

    tmp = math.floor(a[0])
    if tmp == 0.0:
        x = -math.inf
        y = 1.0
        z = -math.inf
        w = 1.0
    else:
        x = math.floor(math.log2(tmp))
        y = tmp / math.pow(2, math.floor(math.log2(tmp)))
        z = math.log2(tmp)
        w = 1.0

    for reg in inst["outputs"]:
        output.set(reg, (x, y, z, w))


def _clamp(val, min_val, max_val):
    return max(min(val, max_val), min_val)


def _ilu_lit(inst: dict, input: Context, output: Context):
    a = input.get(inst["inputs"][0])
    epsilon = 1.0 / 256.0

    sx = max(a[0], 0.0)
    sy = max(a[1], 0.0)
    sw = _clamp(a[3], -(128 - epsilon), 128 - epsilon)

    x = 1.0
    y = sx
    z = 0.0
    if sx > 0:
        z = math.pow(2, sw * math.log2(sy))
    w = 1.0

    output.set(inst["output"], (x, y, z, w))


_ILU_HANDLERS = {
    nv2avsh.vsh_instruction.ILU.ILU_MOV: _mov,
    nv2avsh.vsh_instruction.ILU.ILU_RCP: _ilu_rcp,
    nv2avsh.vsh_instruction.ILU.ILU_RCC: _ilu_rcc,
    nv2avsh.vsh_instruction.ILU.ILU_RSQ: _ilu_rsq,
    nv2avsh.vsh_instruction.ILU.ILU_EXP: _ilu_exp,
    nv2avsh.vsh_instruction.ILU.ILU_LOG: _ilu_log,
    nv2avsh.vsh_instruction.ILU.ILU_LIT: _ilu_lit,
}

 */
