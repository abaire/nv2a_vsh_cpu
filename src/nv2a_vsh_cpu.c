#include "nv2a_vsh_cpu.h"

#include <math.h>
#include <string.h>

void nv2a_vsh_cpu_mov(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  memcpy(out, inputs, sizeof(*out));
}

void nv2a_vsh_cpu_arl(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float val = floorf(inputs->reg.x + 0.001f);
  out->reg.x = val;
  out->reg.y = val;
  out->reg.z = val;
  out->reg.w = val;
}

void nv2a_vsh_cpu_mul(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = inputs[0].reg.x * inputs[1].reg.x;
  out->reg.y = inputs[0].reg.y * inputs[1].reg.y;
  out->reg.z = inputs[0].reg.z * inputs[1].reg.z;
  out->reg.w = inputs[0].reg.w * inputs[1].reg.w;
}

void nv2a_vsh_cpu_add(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = inputs[0].reg.x + inputs[1].reg.x;
  out->reg.y = inputs[0].reg.y + inputs[1].reg.y;
  out->reg.z = inputs[0].reg.z + inputs[1].reg.z;
  out->reg.w = inputs[0].reg.w + inputs[1].reg.w;
}

void nv2a_vsh_cpu_mad(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = inputs[0].reg.x * inputs[1].reg.x + inputs[2].reg.x;
  out->reg.y = inputs[0].reg.y * inputs[1].reg.y + inputs[2].reg.y;
  out->reg.z = inputs[0].reg.z * inputs[1].reg.z + inputs[2].reg.z;
  out->reg.w = inputs[0].reg.w * inputs[1].reg.w + inputs[2].reg.w;
}

void nv2a_vsh_cpu_dp3(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float result = inputs[0].reg.x * inputs[1].reg.x +
                 inputs[0].reg.y * inputs[1].reg.y +
                 inputs[0].reg.z * inputs[1].reg.z;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dph(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float result = inputs[0].reg.x * inputs[1].reg.x +
                 inputs[0].reg.y * inputs[1].reg.y +
                 inputs[0].reg.z * inputs[1].reg.z + inputs[1].reg.w;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dp4(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float result =
      inputs[0].reg.x * inputs[1].reg.x + inputs[0].reg.y * inputs[1].reg.y +
      inputs[0].reg.z * inputs[1].reg.z + inputs[0].reg.w * inputs[1].reg.w;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dst(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = 1.0f;
  out->reg.y = inputs[0].reg.y * inputs[1].reg.y;
  out->reg.z = inputs[0].reg.z;
  out->reg.w = inputs[1].reg.w;
}

void nv2a_vsh_cpu_min(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x =
      inputs[0].reg.x < inputs[1].reg.x ? inputs[0].reg.x : inputs[1].reg.x;
  out->reg.y =
      inputs[0].reg.y < inputs[1].reg.y ? inputs[0].reg.y : inputs[1].reg.y;
  out->reg.z =
      inputs[0].reg.z < inputs[1].reg.z ? inputs[0].reg.z : inputs[1].reg.z;
  out->reg.w =
      inputs[0].reg.w < inputs[1].reg.w ? inputs[0].reg.w : inputs[1].reg.w;
}

void nv2a_vsh_cpu_max(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x =
      inputs[0].reg.x > inputs[1].reg.x ? inputs[0].reg.x : inputs[1].reg.x;
  out->reg.y =
      inputs[0].reg.y > inputs[1].reg.y ? inputs[0].reg.y : inputs[1].reg.y;
  out->reg.z =
      inputs[0].reg.z > inputs[1].reg.z ? inputs[0].reg.z : inputs[1].reg.z;
  out->reg.w =
      inputs[0].reg.w > inputs[1].reg.w ? inputs[0].reg.w : inputs[1].reg.w;
}

void nv2a_vsh_cpu_slt(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = inputs[0].reg.x < inputs[1].reg.x ? 1.0f : 0.0f;
  out->reg.y = inputs[0].reg.y < inputs[1].reg.y ? 1.0f : 0.0f;
  out->reg.z = inputs[0].reg.z < inputs[1].reg.z ? 1.0f : 0.0f;
  out->reg.w = inputs[0].reg.w < inputs[1].reg.w ? 1.0f : 0.0f;
}

void nv2a_vsh_cpu_sge(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  out->reg.x = inputs[0].reg.x >= inputs[1].reg.x ? 1.0f : 0.0f;
  out->reg.y = inputs[0].reg.y >= inputs[1].reg.y ? 1.0f : 0.0f;
  out->reg.z = inputs[0].reg.z >= inputs[1].reg.z ? 1.0f : 0.0f;
  out->reg.w = inputs[0].reg.w >= inputs[1].reg.w ? 1.0f : 0.0f;
}

void nv2a_vsh_cpu_rcp(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float result =
      (inputs[0].reg.x == 1.0f
           ? 1.0f
           : (inputs[0].reg.x == 0.0f ? INFINITY : 1.0f / inputs[0].reg.x));
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_rcc(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  // TODO: Validate this on HW.
  float result;
  if (inputs[0].reg.x == 1.0f) {
    result = 1.0f;
  } else {
    if (inputs[0].reg.x < -1.84467e19f) {
      result = 1.0f / -1.84467e19f;
    } else if (inputs[0].reg.x > -5.42101e-20f && inputs[0].reg.x < 0.0f) {
      result = 1.0f / -5.42101e-020f;
    } else if (inputs[0].reg.x >= 0 && inputs[0].reg.x < 5.42101e-20f) {
      result = 1.0f / 5.42101e-20f;
    } else if (inputs[0].reg.x > 1.84467e+19f) {
      result = 1.0f / 1.84467e+19f;
    } else {
      result = 1.0f / inputs[0].reg.x;
    }
  }
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_rsq(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float result =
      (inputs->reg.x == 1.0f
           ? 1.0f
           : (inputs->reg.x == 0.0f ? INFINITY : 1.0f / sqrtf(inputs->reg.x)));
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_exp(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  float tmp = floorf(inputs->reg.x);
  out->reg.x = powf(2.0f, tmp);
  out->reg.y = inputs->reg.x - tmp;
  out->reg.z = powf(2.0f, inputs->reg.x);
  out->reg.w = 1.0f;
}

void nv2a_vsh_cpu_log(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  // TODO: Validate this on HW.
  float tmp = fabsf(inputs->reg.x);
  if (tmp == 0.0f) {
    out->reg.x = -INFINITY;
    out->reg.y = 1.0f;
    out->reg.z = -INFINITY;
  } else if (isinf(tmp)) {
    out->reg.x = INFINITY;
    out->reg.y = 1.0f;
    out->reg.z = INFINITY;
  } else {
    int exp = 0;
    out->reg.y = frexpf(tmp, &exp);
    out->reg.x = (float)exp;
    out->reg.z = out->reg.x + log2f(tmp);
  }

  out->reg.w = 1.0f;
}

void nv2a_vsh_cpu_lit(Nv2aVshRegister *out, const Nv2aVshRegister *inputs) {
  static const float kMax = 127.9961f;

  out->reg.x = 1.0f;
  out->reg.y = 0.0f;
  out->reg.z = 0.0f;
  out->reg.w = 1.0f;

  float power = inputs->reg.w < -kMax
                    ? -kMax
                    : (inputs->reg.w > kMax ? kMax : inputs->reg.w);
  if (inputs->reg.x > 0.0f) {
    out->reg.y = inputs->reg.x;
    if (inputs->reg.y > 0.0f) {
      out->reg.z = powf(inputs->reg.y, power);
    }
  }
}
