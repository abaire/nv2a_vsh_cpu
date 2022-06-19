#ifndef NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_
#define NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Nv2aVshRegisterComponents {
  float x;
  float y;
  float z;
  float w;
};

typedef union Nv2aVshRegister_ {
  struct Nv2aVshRegisterComponents reg;
  float raw[4];
} Nv2aVshRegister;

#define OP_1(name) \
  void nv2a_vsh_cpu_##name(Nv2aVshRegister *out, const Nv2aVshRegister *a)
#define OP_2(name)                                                         \
  void nv2a_vsh_cpu_##name(Nv2aVshRegister *out, const Nv2aVshRegister *a, \
                           const Nv2aVshRegister *b)
#define OP_3(name)                                                         \
  void nv2a_vsh_cpu_##name(Nv2aVshRegister *out, const Nv2aVshRegister *a, \
                           const Nv2aVshRegister *b, const Nv2aVshRegister *c)

OP_1(mov);
OP_1(arl);
OP_2(mul);
OP_2(add);
OP_3(mad);
OP_2(dp3);
OP_2(dph);
OP_2(dp4);
OP_2(dst);
OP_2(min);
OP_2(max);
OP_2(slt);
OP_2(sge);
OP_1(rcp);
OP_1(rcc);
OP_1(rsq);
OP_1(exp);
OP_1(log);
OP_1(lit);

#undef OP_1
#undef OP_2
#undef OP_3

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_
