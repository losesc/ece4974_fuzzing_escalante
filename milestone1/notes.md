# Milestone 1 - setup notes

## Environment

Kali Linux, Clang/LLVM 21.1.8, AFL++ 5.03c built from source
(LLVM-PCGUARD mode). Kali ships a much newer Clang than the Ubuntu 22.04/24.04
the slides target, which caused two of the demo programs to behave differently.
Notes below in case they're useful. 

## Step 3 - needed a different out-of-bounds index

The slide's `int a[10]; return a[42];` produced a UBSan report but no ASan
`stack-buffer-overflow` on my setup.

ASan detects out-of-bounds access using a poisoned redzone around each object.
Index 42 is 168 bytes past a 40-byte array, so it lands past the redzone in
ordinary stack memory and reads as valid. UBSan caught it since the index is
a compile-time constant.

Using a runtime index just past the end (`test_asan.c`) puts the access inside
the redzone and produces the required report with both sanitizers firing:

    int i = argc + 9;   // 10 at runtime
    return a[i];

## Step 5 - target needed `volatile`, and AFL++ needed laf-intel

Two separate issues.

First, `printf 'AFL' | ./crash_me` exited 0 at -O1 but segfaulted at -O0.
Writing through a null pointer is undefined behavior, so the compiler may treat
the branch as unreachable and remove it. Declaring the pointer volatile
prevents that, and the crash reproduces by hand (ASan SEGV crash_me.c:9).

Second, with a crashing target AFL++ still reported 0 crashes after ~80k execs
at ~5000/sec, corpus count 4. At -O1 the three byte comparisons get merged into
one wide comparison, so coverage feedback gives no partialt for matching
a single byte and the search becomes a brute force.

`AFL_LLVM_LAF_ALL=1` (laf-intel) splits wide comparisons back into byte-sized
ones. AFL++ then found the crash in 4,756 execs, about 9 seconds:

    AFL_LLVM_LAF_ALL=1 AFL_USE_ASAN=1 afl-clang-fast -g -O1 crash_me.c -o crash_me_laf

Crashing input saved as `crash_input_AFL.bin` (contents: `AFL`).

## Files

- `test_sanitizers.c`- Step 3 program from the slides
- `test_asan.c` - modified version that triggers ASan
- `crash_me_original.c` - Step 5 program as printed
- `crash_me.c` — same with `volatile`, the version that was fuzzed
- `crash_input_AFL.bin` - crashing input found by AFL++
- `step2`–`step5` txt files - terminal transcripts
