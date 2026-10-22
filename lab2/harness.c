#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct src { const unsigned char *p; size_t len, off; };

static void read_cb(png_structp png, png_bytep out, png_size_t n) {
  struct src *s = png_get_io_ptr(png);
  if (n > s->len - s->off) png_error(png, "EOF");
  memcpy(out, s->p + s->off, n); s->off += n;
}

int main(void) {
  static unsigned char buf[1 << 20];
  size_t len = 0; ssize_t r;
  while (len < sizeof buf && (r = read(0, buf + len, sizeof buf - len)) > 0) len += r;
  if (len < 8 || png_sig_cmp(buf, 0, 8)) return 0;

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png ? png_create_info_struct(png) : NULL;
  if (!info) { png_destroy_read_struct(&png, NULL, NULL); return 0; }

  png_bytep volatile row = NULL;
  if (setjmp(png_jmpbuf(png))) {
    free((void *)row); png_destroy_read_struct(&png, &info, NULL); return 0;
  }
  png_set_user_limits(png, 4096, 4096);
  png_set_chunk_malloc_max(png, 8000000);
  png_set_crc_action(png, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
#ifdef PNG_IGNORE_ADLER32
  png_set_option(png, PNG_IGNORE_ADLER32, PNG_OPTION_ON);
#endif
  struct src s = { buf, len, 0 };
  png_set_read_fn(png, &s, read_cb);

  png_read_info(png, info);
  int passes = png_set_interlace_handling(png);
  png_read_update_info(png, info);
  row = malloc(png_get_rowbytes(png, info));
  for (int p = 0; p < passes; p++)
    for (png_uint_32 y = 0; y < png_get_image_height(png, info); y++)
      png_read_row(png, row, NULL);
  png_read_end(png, info);

  free((void *)row); png_destroy_read_struct(&png, &info, NULL);
  return 0;
}
