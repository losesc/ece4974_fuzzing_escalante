# ECE 4974 - Software Fuzzing for Detecting Security Vulnerabilities

Coursework repository for the independent study on software fuzzing.
Los Escalante, Fall 2026.

## Environment

- Kali Linux 
- Clang / LLVM 21.1.8
- AFL++ 5.03c, built from source, LLVM-PCGUARD mode
- Sanitizers: AddressSanitizer, UndefinedBehaviorSanitizer

## Contents

### `milestone1/`- Toolchain ready

Setup and verification of the fuzzing environmen, Linux environment,
Clang/LLVM and build tools, ASan/UBSan verification, AFL++ build and install,
and an end-to-end fuzzing run against a demo target.

Terminal transcripts for each step are in step2-step5 text files.
`NOTES.md` documents two places where the demo programs behaved differently
on Clang 21 than on the Clang 14/18 the course slides target, and how each
was resolved.

## Milestone status

- Milestone 1 - toolchain ready
