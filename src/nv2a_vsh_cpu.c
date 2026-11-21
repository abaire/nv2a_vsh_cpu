#include "nv2a_vsh_cpu.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define _X 0
#define _Y 1
#define _Z 2
#define _W 3
#define COMP(array, index, component) (array[(index) * 4 + component])

#define SIGN_MASK 0x80000000u
#define MAGNITUDE_MASK 0x7FFFFFFFu
#define EXPONENT_MASK 0x7F800000u

// NaN results produced by the hardware's arithmetic units.
static const uint32_t kCanonicalNaN = 0x7FFFFFFF;

static inline uint32_t to_bits(float value) {
  uint32_t ret;
  memcpy(&ret, &value, sizeof(ret));
  return ret;
}

static inline float from_bits(uint32_t bits) {
  float ret;
  memcpy(&ret, &bits, sizeof(ret));
  return ret;
}

static inline bool is_nan_bits(uint32_t bits) {
  return (bits & MAGNITUDE_MASK) > EXPONENT_MASK;
}

static inline bool has_sign(float value) { return to_bits(value) & SIGN_MASK; }

static inline void broadcast(float* out, float value) {
  out[0] = value;
  out[1] = value;
  out[2] = value;
  out[3] = value;
}

// nv2a arithmetic treats denormal inputs and results as zero with the same
// sign. Comparisons and MIN/MAX/MOV preserve denormals.
static inline float flush_denormal(float value) {
  uint32_t bits = to_bits(value);
  if (!(bits & EXPONENT_MASK)) {
    return from_bits(bits & SIGN_MASK);
  }
  return value;
}

// nv2a compares values as sign-magnitude integers, so -0 < +0 and positive
// NaNs sort above +inf. Negative NaNs sorting below -inf follows from this
// model but has not been measured on hardware.
static inline int64_t sort_key(float value) {
  uint32_t bits = to_bits(value);
  if (bits & SIGN_MASK) {
    return -(int64_t)(bits & MAGNITUDE_MASK) - 1;
  }
  return bits;
}

// Overflow from finite operands saturates to the largest finite value.
static inline float clamp_finite_overflow(float result, float a, float b) {
  if (isinf(result) && !isinf(a) && !isinf(b)) {
    return from_bits((to_bits(result) & SIGN_MASK) | 0x7F7FFFFF);
  }
  return result;
}

// Zero times anything (including inf and NaN) is +0 on nv2a.
static inline float nv2a_mul(float a, float b) {
  a = flush_denormal(a);
  b = flush_denormal(b);
  if (a == 0.0f || b == 0.0f) {
    return 0.0f;
  }
  if (isnan(a) || isnan(b)) {
    return from_bits(kCanonicalNaN);
  }
  return flush_denormal(clamp_finite_overflow(a * b, a, b));
}

static inline float nv2a_add(float a, float b) {
  a = flush_denormal(a);
  b = flush_denormal(b);
  float result = a + b;
  if (isnan(result)) {
    return from_bits(kCanonicalNaN);
  }
  result = flush_denormal(clamp_finite_overflow(result, a, b));
  if (result == 0.0f) {
    return 0.0f;
  }
  return result;
}

static inline float nv2a_dot3(const float* inputs) {
  float result = nv2a_mul(COMP(inputs, 0, _X), COMP(inputs, 1, _X));
  result = nv2a_add(result, nv2a_mul(COMP(inputs, 0, _Y), COMP(inputs, 1, _Y)));
  return nv2a_add(result, nv2a_mul(COMP(inputs, 0, _Z), COMP(inputs, 1, _Z)));
}

void nv2a_vsh_cpu_mov(float* out, const float* inputs) {
  memcpy(out, inputs, sizeof(float) * 4);
}

void nv2a_vsh_cpu_arl(float* out, const float* inputs) {
  broadcast(out, floorf(COMP(inputs, 0, _X) + 0.001f));
}

void nv2a_vsh_cpu_mul(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    out[i] = nv2a_mul(COMP(inputs, 0, i), COMP(inputs, 1, i));
  }
}

void nv2a_vsh_cpu_add(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    out[i] = nv2a_add(COMP(inputs, 0, i), COMP(inputs, 1, i));
  }
}

void nv2a_vsh_cpu_mad(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    out[i] = nv2a_add(nv2a_mul(COMP(inputs, 0, i), COMP(inputs, 1, i)),
                      COMP(inputs, 2, i));
  }
}

void nv2a_vsh_cpu_dp3(float* out, const float* inputs) {
  broadcast(out, nv2a_dot3(inputs));
}

void nv2a_vsh_cpu_dph(float* out, const float* inputs) {
  broadcast(out, nv2a_add(nv2a_dot3(inputs), COMP(inputs, 1, _W)));
}

void nv2a_vsh_cpu_dp4(float* out, const float* inputs) {
  broadcast(out, nv2a_add(nv2a_dot3(inputs),
                          nv2a_mul(COMP(inputs, 0, _W), COMP(inputs, 1, _W))));
}

void nv2a_vsh_cpu_dst(float* out, const float* inputs) {
  out[0] = 1.0f;
  out[1] = nv2a_mul(COMP(inputs, 0, _Y), COMP(inputs, 1, _Y));
  out[2] = COMP(inputs, 0, _Z);
  out[3] = COMP(inputs, 1, _W);
}

void nv2a_vsh_cpu_min(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    float a = COMP(inputs, 0, i);
    float b = COMP(inputs, 1, i);
    out[i] = sort_key(a) < sort_key(b) ? a : b;
  }
}

void nv2a_vsh_cpu_max(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    float a = COMP(inputs, 0, i);
    float b = COMP(inputs, 1, i);
    out[i] = sort_key(a) > sort_key(b) ? a : b;
  }
}

void nv2a_vsh_cpu_slt(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    bool less = sort_key(COMP(inputs, 0, i)) < sort_key(COMP(inputs, 1, i));
    out[i] = less ? 1.0f : 0.0f;
  }
}

void nv2a_vsh_cpu_sge(float* out, const float* inputs) {
  for (uint32_t i = 0; i < 4; ++i) {
    bool less = sort_key(COMP(inputs, 0, i)) < sort_key(COMP(inputs, 1, i));
    out[i] = less ? 0.0f : 1.0f;
  }
}

// Computes 1/in with nv2a handling of NaN, signed zero and denormals.
static inline float nv2a_reciprocal(float in) {
  in = flush_denormal(in);
  if (isnan(in)) {
    return from_bits(kCanonicalNaN);
  }
  if (in == 1.0f) {
    return 1.0f;
  }
  if (in == 0.0f) {
    return has_sign(in) ? -INFINITY : INFINITY;
  }
  return flush_denormal(1.0f / in);
}

void nv2a_vsh_cpu_rcp(float* out, const float* inputs) {
  broadcast(out, nv2a_reciprocal(COMP(inputs, 0, _X)));
}

static const uint32_t kRCCMin = 0x1F800000;  // 2^-64
static const uint32_t kRCCMax = 0x5F800000;  // 2^64

void nv2a_vsh_cpu_rcc(float* out, const float* inputs) {
  float result = nv2a_reciprocal(COMP(inputs, 0, _X));
  if (!isnan(result)) {
    uint32_t bits = to_bits(result);
    uint32_t magnitude = bits & MAGNITUDE_MASK;
    if (magnitude < kRCCMin) {
      magnitude = kRCCMin;
    } else if (magnitude > kRCCMax) {
      magnitude = kRCCMax;
    }
    result = from_bits((bits & SIGN_MASK) | magnitude);
  }
  broadcast(out, result);
}

void nv2a_vsh_cpu_rsq(float* out, const float* inputs) {
  float in = fabsf(flush_denormal(COMP(inputs, 0, _X)));
  float result;
  if (isnan(in)) {
    result = from_bits(kCanonicalNaN);
  } else if (in == 1.0f) {
    result = 1.0f;
  } else if (in == 0.0f) {
    result = INFINITY;
  } else {
    result = flush_denormal(1.0f / sqrtf(in));
  }
  broadcast(out, result);
}

void nv2a_vsh_cpu_exp(float* out, const float* inputs) {
  float in = COMP(inputs, 0, _X);
  out[3] = 1.0f;
  if (isnan(in)) {
    out[0] = INFINITY;
    out[1] = in;
    out[2] = from_bits(kCanonicalNaN);
    return;
  }

  in = flush_denormal(in);
  if (isinf(in)) {
    out[0] = in > 0.0f ? INFINITY : 0.0f;
    out[1] = 0.0f;
    out[2] = out[0];
    return;
  }

  // A negative input is rounded toward -inf to a multiple of 2^-23 before the
  // fraction is taken, except that one smaller in magnitude than 2^-23 gives
  // (1, 1, 1, 1). Positive inputs are not rounded.
  static const float kFractionScale = 8388608.0f;  // 2^23
  if (in < 0.0f && in > -kFractionScale) {
    if (in > -1.0f / kFractionScale) {
      broadcast(out, 1.0f);
      return;
    }
    in = floorf(in * kFractionScale) / kFractionScale;
  }

  float tmp = floorf(in);
  out[0] = flush_denormal(powf(2.0f, tmp));
  out[1] = in - tmp;
  out[2] = flush_denormal(powf(2.0f, in));
}

void nv2a_vsh_cpu_log(float* out, const float* inputs) {
  float tmp = fabsf(flush_denormal(COMP(inputs, 0, _X)));
  out[3] = 1.0f;
  if (isnan(tmp)) {
    out[0] = tmp;
    out[1] = tmp;
    out[2] = tmp;
  } else if (tmp == 0.0f) {
    out[0] = -INFINITY;
    out[1] = 1.0f;
    out[2] = -INFINITY;
  } else if (isinf(tmp)) {
    out[0] = INFINITY;
    out[1] = 1.0f;
    out[2] = INFINITY;
  } else {
    // frexpf returns values that do not match nv2a, so the exponent is
    // extracted manually.
    uint32_t tmp_int = to_bits(tmp);
    int32_t exponent = (int32_t)((tmp_int >> 23) & 0xFF) - 127;
    uint32_t mantissa = (tmp_int & 0x7FFFFF) | 0x3F800000;

    out[0] = (float)exponent;
    out[1] = from_bits(mantissa);
    out[2] = log2f(tmp);
  }
}

void nv2a_vsh_cpu_lit(float* out, const float* inputs) {
  static const float kMax = 128.0f;

  out[0] = 1.0f;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 1.0f;

  float n_dot_l = inputs[0];
  if (sort_key(n_dot_l) <= sort_key(0.0f)) {
    return;
  }
  // Y passes N.L through unflushed, but a denormal N.L does not enable Z.
  out[1] = n_dot_l;
  if (sort_key(flush_denormal(n_dot_l)) <= sort_key(0.0f)) {
    return;
  }

  // Negative N.H (including -0) is treated as +0, but NaN N.H propagates
  // regardless of sign.
  float n_dot_h = flush_denormal(inputs[1]);
  if (!isnan(n_dot_h) && has_sign(n_dot_h)) {
    n_dot_h = 0.0f;
  }
  float power = flush_denormal(inputs[3]);
  if (isnan(power)) {
    out[2] = from_bits(kCanonicalNaN);
    return;
  }
  power = power < -kMax ? -kMax : (power > kMax ? kMax : power);

  float result = powf(n_dot_h, power);
  out[2] = isnan(result) ? from_bits(kCanonicalNaN) : flush_denormal(result);
}
