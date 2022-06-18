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

void nv2a_vsh_cpu_mul(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x * b->reg.x;
  out->reg.y = a->reg.y * b->reg.y;
  out->reg.z = a->reg.z * b->reg.z;
  out->reg.w = a->reg.w * b->reg.w;
}

void nv2a_vsh_cpu_add(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x + b->reg.x;
  out->reg.y = a->reg.y + b->reg.y;
  out->reg.z = a->reg.z + b->reg.z;
  out->reg.w = a->reg.w + b->reg.w;
}

void nv2a_vsh_cpu_mad(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b, const nv2a_vsh_register *c) {
  out->reg.x = a->reg.x * b->reg.x + c->reg.x;
  out->reg.y = a->reg.y * b->reg.y + c->reg.y;
  out->reg.z = a->reg.z * b->reg.z + c->reg.z;
  out->reg.w = a->reg.w * b->reg.w + c->reg.w;
}

void nv2a_vsh_cpu_dp3(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  float result =
      a->reg.x * b->reg.x + a->reg.y * b->reg.y + a->reg.z * b->reg.z;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dph(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  float result = a->reg.x * b->reg.x + a->reg.y * b->reg.y +
                 a->reg.z * b->reg.z + b->reg.w;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dp4(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  float result = a->reg.x * b->reg.x + a->reg.y * b->reg.y +
                 a->reg.z * b->reg.z + a->reg.w * b->reg.w;
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_dst(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = 1.0f;
  out->reg.y = a->reg.y * b->reg.y;
  out->reg.z = a->reg.z;
  out->reg.w = b->reg.w;
}

void nv2a_vsh_cpu_min(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x < b->reg.x ? a->reg.x : b->reg.x;
  out->reg.y = a->reg.y < b->reg.y ? a->reg.y : b->reg.y;
  out->reg.z = a->reg.z < b->reg.z ? a->reg.z : b->reg.z;
  out->reg.w = a->reg.w < b->reg.w ? a->reg.w : b->reg.w;
}

void nv2a_vsh_cpu_max(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x > b->reg.x ? a->reg.x : b->reg.x;
  out->reg.y = a->reg.y > b->reg.y ? a->reg.y : b->reg.y;
  out->reg.z = a->reg.z > b->reg.z ? a->reg.z : b->reg.z;
  out->reg.w = a->reg.w > b->reg.w ? a->reg.w : b->reg.w;
}

void nv2a_vsh_cpu_slt(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x < b->reg.x ? 1.0f : 0.0f;
  out->reg.y = a->reg.y < b->reg.y ? 1.0f : 0.0f;
  out->reg.z = a->reg.z < b->reg.z ? 1.0f : 0.0f;
  out->reg.w = a->reg.w < b->reg.w ? 1.0f : 0.0f;
}

void nv2a_vsh_cpu_sge(nv2a_vsh_register *out, const nv2a_vsh_register *a,
                      const nv2a_vsh_register *b) {
  out->reg.x = a->reg.x >= b->reg.x ? 1.0f : 0.0f;
  out->reg.y = a->reg.y >= b->reg.y ? 1.0f : 0.0f;
  out->reg.z = a->reg.z >= b->reg.z ? 1.0f : 0.0f;
  out->reg.w = a->reg.w >= b->reg.w ? 1.0f : 0.0f;
}

void nv2a_vsh_cpu_rcp(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  float result =
      (a->reg.x == 1.0f ? 1.0f
                        : (a->reg.x == 0.0f ? INFINITY : 1.0f / a->reg.x));
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_rcc(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  // TODO: Validate this on HW.
  float result;
  if (a->reg.x == 1.0f) {
    result = 1.0f;
  } else {
    if (a->reg.x < -1.84467e19f) {
      result = 1.0f / -1.84467e19f;
    } else if (a->reg.x > -5.42101e-20f && a->reg.x < 0.0f) {
      result = 1.0f / -5.42101e-020f;
    } else if (a->reg.x >= 0 && a->reg.x < 5.42101e-20f) {
      result = 1.0f / 5.42101e-20f;
    } else if (a->reg.x > 1.84467e+19f) {
      result = 1.0f / 1.84467e+19f;
    } else {
      result = 1.0f / a->reg.x;
    }
  }
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_rsq(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  float result = (a->reg.x == 1.0f
                      ? 1.0f
                      : (a->reg.x == 0.0f ? INFINITY : 1.0f / sqrtf(a->reg.x)));
  out->reg.x = result;
  out->reg.y = result;
  out->reg.z = result;
  out->reg.w = result;
}

void nv2a_vsh_cpu_exp(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  float tmp = floorf(a->reg.x);
  out->reg.x = powf(2.0f, tmp);
  out->reg.y = a->reg.x - tmp;
  out->reg.z = powf(2.0f, a->reg.x);
  out->reg.w = 1.0f;
}

void nv2a_vsh_cpu_log(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  // TODO: Validate this on HW.
  float tmp = fabsf(a->reg.x);
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

void nv2a_vsh_cpu_lit(nv2a_vsh_register *out, const nv2a_vsh_register *a) {
  static const float kMax = 127.9961f;

  out->reg.x = 1.0f;
  out->reg.y = 0.0f;
  out->reg.z = 0.0f;
  out->reg.w = 1.0f;

  float power = a->reg.w < -kMax ? -kMax : (a->reg.w > kMax ? kMax : a->reg.w);
  if (a->reg.x > 0.0f) {
    out->reg.y = a->reg.x;
    if (a->reg.y > 0.0f) {
      out->reg.z = powf(a->reg.y, power);
    }
  }
}
