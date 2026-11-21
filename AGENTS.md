# Agent guidelines

## Hardware behavior claims need hardware evidence

This library emulates the nv2a vertex shader, so "correct" means "matches the
hardware", not "matches IEEE 754" or "matches what seems sensible".

Do not report emulator behavior as a bug, or propose a change to it, because it
differs from IEEE 754, from D3D documentation, from another emulator, or from
what the hardware "probably" does. Every such suggestion must be backed by a
hardware measurement:

- Cite the record in `test/exceptional_values/expectations.json` (group, op,
  case and component) that the current code fails to match, or
- Name the test in
  [nxdk_low_level_nv2a_tests](https://github.com/abaire/nxdk_low_level_nv2a_tests)
  that measures the behavior and the result it produced on hardware.

If no hardware data covers the case, say so plainly. Do not present it as a
bug. Instead, propose a hardware test that would settle the question, with the
operands to try.

This rule covers the emulated hardware's behavior only. Bugs in the host-side
code (build files, test harness, scripts, portability of the C code) can be
reported as usual.
