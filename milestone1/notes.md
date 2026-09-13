# Milestone 1 - setup notes

## Environment

Kali Linux (rolling), Clang/LLVM 21.1.8, AFL++ 5.03c built from source
(LLVM-PCGUARD mode). Kali ships a much newer Clang than the Ubuntu 22.04/24.04
the slides target, which caused two of the demo programs to behave differently.
Notes below in case they're useful to anyone on a similar setup.

Everything below is what I observed on this setup. Where the instructor received
different result on gcc 13, that difference is noted.

## Step 3 - needed a different out-of-bounds index

The slide's `int a[10]; return a[42];` produced a UBSan report but no ASan
`stack-buffer-overflow` on my setup.

ASan detects out-of-bounds access using a poisoned redzone around each object.
Index 42 is 168 bytes past a 40-byte array, so it lands past the redzone in
ordinary stack memory and reads as valid. UBSan caught it because the index is
a compile time constant.

Using a runtime index just past the end (`test_asan.c`) puts the access inside
the redzone and produces the required report:

    int i = argc + 9;   // 10 at runtime
    return a[i];

Compiler difference: the instructor reported that at -O1 the array is
optimized away entirely, so even the corrected version reports nothing, and
that the example needs -O0. That was not reproducible here. On Clang 21 the
corrected version reports `stack-buffer-overflow` at both -O1 and -O0; both
runs are in `step3_sanitizers.txt`. gcc appears to eliminate the array where
Clang 21 does not.

## Step 5 - target needed `volatile`, and AFL++ needed laf-intel

Two separate issues.

First, `printf 'AFL' | ./crash_me` exited 0 at -O1 but segfaulted at -O0.
Writing through a null pointer is undefined behavior, so a compiler is
permitted to treat the branch as unreachable and remove it. Declaring the
pointer `volatile` prevented that, and the crash then reproduced by hand
(ASan SEGV at crash_me.c:9).

Compiler difference: the instructor could not reproduce this - gcc 13
segfaults at -O0, -O1 and -O2, with or without `volatile`. So the deleted
store appears to be Clang-specific rather than something every compiler does.
Being permitted by the standard is not the same as being observed; this was
only observed on Clang 21.

Second, with a crashing target AFL++ still reported 0 crashes after ~80k execs
at ~5000/sec, corpus count 4. At -O1 the three byte comparisons get merged into
one wide comparison, so coverage feedback gives no partial credit for matching
a single byte and the search becomes a 2^24 brute force.

`AFL_LLVM_LAF_ALL=1` (laf-intel) splits wide comparisons back into byte-sized
ones, turning one 1-in-16-million problem into three 1-in-256 problems. AFL++
then found the crash in 4,756 execs, about 9 seconds:

    AFL_LLVM_LAF_ALL=1 AFL_USE_ASAN=1 afl-clang-fast -g -O1 crash_me.c -o crash_me_laf

Crashing input saved as `crash_input_AFL.bin`. The file is 4 bytes: `AFL\0`
(41 46 4c 00), not 3.

## Files

- `test_sanitizers.c` - Step 3 program from the slides
- `test_asan.c` - modified version that triggers ASan
- `crash_me_original.c` - Step 5 program as printed on the slide
- `crash_me.c` - same with `volatile`, the version that was fuzzed
- `crash_input_AFL.bin` - crashing input found by AFL++ (4 bytes, `AFL\0`)
- `step2_toolchain.txt` - clang and llvm-config versions
- `step3_sanitizers.txt` - slide version and corrected version, at -O1 and -O0
- `step4_aflpp.txt` - afl-fuzz and afl-cc version output
- `step5_fuzzing.txt` - by-hand crash, AFL++ crash listing, crashing input
