/* Persistent-mode wrapper around the frozen Lab 2 harness (Part 4). */
#include <png.h>
#define main harness_v2_stdin_main
#include "harness_v2.c"
#undef main
__AFL_FUZZ_INIT();
int main(void) {
  __AFL_INIT();
  unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
  while (__AFL_LOOP(10000)) {
    size_t len = __AFL_FUZZ_TESTCASE_LEN;
    if (len < 10 || png_sig_cmp(buf, 0, 8)) continue;
    unsigned f1 = buf[len - 1], f2 = buf[len - 2];
    lowlevel(buf, len, f1, f2);
    simplified(buf, len, f2);
  }
  return 0;
}
