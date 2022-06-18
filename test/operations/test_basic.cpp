#include <boost/test/unit_test.hpp>

#include "nv2a_vsh_cpu.h"

BOOST_AUTO_TEST_SUITE(basic_operation_suite)

BOOST_AUTO_TEST_CASE(mov) {
  nv2a_vsh_register a = {0.0f, -1000.0f, 1000.0f, 64.123456f};

  nv2a_vsh_register out;
  nv2a_vsh_cpu_mov(&out, &a);

  BOOST_TEST(out.reg.x == a.reg.x);
  BOOST_TEST(out.reg.y == a.reg.y);
  BOOST_TEST(out.reg.z == a.reg.z);
  BOOST_TEST(out.reg.w == a.reg.w);
}

BOOST_AUTO_TEST_CASE(arl_trivial) {
  nv2a_vsh_register a = {10.0f, -1000.0f, 1000.0f, 64.123456f};

  nv2a_vsh_register out;
  nv2a_vsh_cpu_arl(&out, &a);

  BOOST_TEST(out.reg.x == a.reg.x);
  BOOST_TEST(out.reg.y == a.reg.x);
  BOOST_TEST(out.reg.z == a.reg.x);
  BOOST_TEST(out.reg.w == a.reg.x);
}

BOOST_AUTO_TEST_CASE(arl_truncate) {
  nv2a_vsh_register a = {10.12345f, -1000.0f, 1000.0f, 64.123456f};

  nv2a_vsh_register out;
  nv2a_vsh_cpu_arl(&out, &a);

  BOOST_TEST(out.reg.x == 10.0f);
  BOOST_TEST(out.reg.y == 10.0f);
  BOOST_TEST(out.reg.z == 10.0f);
  BOOST_TEST(out.reg.w == 10.0f);
}

BOOST_AUTO_TEST_CASE(arl_biased) {
  nv2a_vsh_register a = {9.9999999f, -1000.0f, 1000.0f, 64.123456f};

  nv2a_vsh_register out;
  nv2a_vsh_cpu_arl(&out, &a);

  BOOST_TEST(out.reg.x == 10.0f);
  BOOST_TEST(out.reg.y == 10.0f);
  BOOST_TEST(out.reg.z == 10.0f);
  BOOST_TEST(out.reg.w == 10.0f);
}

BOOST_AUTO_TEST_SUITE_END()
