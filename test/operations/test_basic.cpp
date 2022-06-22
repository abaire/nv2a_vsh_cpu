#include <boost/test/unit_test.hpp>

#include "nv2a_vsh_cpu.h"

BOOST_AUTO_TEST_SUITE(basic_operation_suite)

BOOST_AUTO_TEST_CASE(mov) {
  float inputs[] = {0.0f, -1000.0f, 1000.0f, 64.123456f};
  float out[4];
  nv2a_vsh_cpu_mov(out, inputs);

  BOOST_TEST(out[0] == inputs[0]);
  BOOST_TEST(out[1] == inputs[1]);
  BOOST_TEST(out[2] == inputs[2]);
  BOOST_TEST(out[3] == inputs[3]);
}

BOOST_AUTO_TEST_CASE(arl_trivial) {
  float inputs[] = {10.0f, -1000.0f, 1000.0f, 64.123456f};

  float out[4];
  nv2a_vsh_cpu_arl(out, inputs);

  BOOST_TEST(out[0] == inputs[0]);
  BOOST_TEST(out[1] == inputs[0]);
  BOOST_TEST(out[2] == inputs[0]);
  BOOST_TEST(out[3] == inputs[0]);
}

BOOST_AUTO_TEST_CASE(arl_truncate) {
  float inputs[] = {10.12345f, -1000.0f, 1000.0f, 64.123456f};

  float out[4];
  nv2a_vsh_cpu_arl(out, inputs);

  BOOST_TEST(out[0] == 10.0f);
  BOOST_TEST(out[1] == 10.0f);
  BOOST_TEST(out[2] == 10.0f);
  BOOST_TEST(out[3] == 10.0f);
}

BOOST_AUTO_TEST_CASE(arl_biased) {
  float inputs[] = {9.9999999f, -1000.0f, 1000.0f, 64.123456f};

  float out[4];
  nv2a_vsh_cpu_arl(out, inputs);

  BOOST_TEST(out[0] == 10.0f);
  BOOST_TEST(out[1] == 10.0f);
  BOOST_TEST(out[2] == 10.0f);
  BOOST_TEST(out[3] == 10.0f);
}

BOOST_AUTO_TEST_CASE(add_trivial) {
  float inputs[] = {1.0f, 2.0f, 4.0f, 64.0f,
                    10.0f, -10.0f, 100.0f, -100.0f};

  float out[4];
  nv2a_vsh_cpu_add(out, inputs);

  BOOST_TEST(out[0] == 11.0f);
  BOOST_TEST(out[1] == -8.0f);
  BOOST_TEST(out[2] == 104.0f);
  BOOST_TEST(out[3] == -36.0f);
}
BOOST_AUTO_TEST_SUITE_END()
