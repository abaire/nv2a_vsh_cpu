#ifndef NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_
#define NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

struct nv2a_vsh_register_components {
  float x;
  float y;
  float z;
  float w;
};

typedef union nv2a_vsh_register_ {
  struct nv2a_vsh_register_components reg;
  float raw[4];
} nv2a_vsh_register;

#define OP_1(name) \
  void nv2a_vsh_cpu_##name(nv2a_vsh_register *out, const nv2a_vsh_register *a)
#define OP_2(name)                                                             \
  void nv2a_vsh_cpu_##name(nv2a_vsh_register *out, const nv2a_vsh_register *a, \
                           const nv2a_vsh_register *b)
#define OP_3(name)                                                             \
  void nv2a_vsh_cpu_##name(nv2a_vsh_register *out, const nv2a_vsh_register *a, \
                           const nv2a_vsh_register *b,                         \
                           const nv2a_vsh_register *c)

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
};
#endif

#endif  // NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_
