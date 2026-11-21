# nv2a_vsh_cpu

This project provides a CPU-based implementation of the nv2a vertex shader
operations used by the original Microsoft Xbox.

See https://registry.khronos.org/OpenGL/extensions/NV/NV_vertex_program1_1.txt

# Development

## git hooks

This project uses [git hooks](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks)
to automate some aspects of keeping the code base healthy, in particular `clang-format`
invocation.

Please copy the files from the `githooks` subdirectory into `.git/hooks` to
enable them.

## Running the tests

```shell
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

## Updating the exceptional value expectations

`exceptional_value_tests` checks the emulator against results that were measured on Xbox
hardware for various edge cases. The measurements come from the `VSH Exceptional *` tests
in [nxdk_low_level_nv2a_tests](https://github.com/abaire/nxdk_low_level_nv2a_tests).

The `test/exceptional_values` directory contains a snapshot of the `vsh_log.json` file
produced by the low level tester as well as scripts to extract expectations from the log
and transform them into a header file used by the expectation tests in this repo.

To update the expectations:

1. Check out `nxdk_low_level_nv2a_tests` and build its `vsh_test_xiso` target.
1. Run `vsh_test` on hardware, paying attention to the final output indicating where the
   log file is written.
1. Copy the log from the Xbox to `test/exceptional_values/vsh_log.json` in this repository.
1. Regenerate the derived files:

   ```shell
   test/exceptional_values/extract_expectations.py test/exceptional_values/vsh_log.json
   test/exceptional_values/generate_expectations.py
   ```

1. Rebuild and run the tests. A new failure means either that the nv2a emulator needs to
   change, or that a record has to be disabled in `test_exceptional_values.cpp` (see
   `kUnverifiedApproximations`) with a comment explaining why.
1. Commit `vsh_log.json`, `expectations.json` and `expectations.h` together.
