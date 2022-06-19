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

typedef void (*Nv2aVshCpuFunc)(Nv2aVshRegister *out,
                               const Nv2aVshRegister *inputs);

void nv2a_vsh_cpu_mov(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_arl(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_mul(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_add(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_mad(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_dp3(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_dph(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_dp4(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_dst(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_min(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_max(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_slt(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_sge(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_rcp(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_rcc(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_rsq(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_exp(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_log(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);
void nv2a_vsh_cpu_lit(Nv2aVshRegister *out, const Nv2aVshRegister *inputs);

#ifdef __cplusplus
};  // extern "C"
#endif

#endif  // NV2A_VSH_CPU_SRC_NV2A_VSH_CPU_H_
