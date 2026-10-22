#include <png.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct src { const unsigned char *p; size_t len, off; };
static void read_cb(png_structp png, png_bytep out, size_t n) {
  struct src *s = png_get_io_ptr(png);
  if (n > s->len - s->off) png_error(png, "EOF");
  memcpy(out, s->p + s->off, n); s->off += n;
}
static void lowlevel(const unsigned char *buf, size_t len, unsigned f1, unsigned f2) {
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png ? png_create_info_struct(png) : NULL;
  png_bytep volatile row = NULL;
  struct src s = { buf, len, 0 };
  if (!info || setjmp(png_jmpbuf(png))) goto out;
  png_set_user_limits(png, 4096, 4096);
  png_set_chunk_malloc_max(png, 8000000);
  png_set_crc_action(png, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
  png_set_read_fn(png, &s, read_cb);
  png_read_info(png, info);
#ifdef PNG_READ_QUANTIZE_SUPPORTED
  /* Reach png_do_quantize (CVE-2025-64505). Palette images only; use the
     image's own PLTE. f2 varies maximum_colors to hit both paths. */
  if ((f2 & 1) && (png_get_color_type(png, info) & PNG_COLOR_MASK_PALETTE)) {
    png_colorp pal; int np;
    if (png_get_PLTE(png, info, &pal, &np) && np > 0) {
      int maxc = (f2 & 2) ? 2 : ((f2 & 4) ? (np > 16 ? 16 : np) : np);
      png_set_quantize(png, pal, np, maxc, NULL, 1);
    }
  }
#endif
  if (f1 & 1)   png_set_expand(png);
  if (f1 & 2)   png_set_strip_16(png);
  if (f1 & 4)   png_set_gray_to_rgb(png);
  if (f1 & 8)   png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
  if (f1 & 16)  png_set_rgb_to_gray_fixed(png, 1, -1, -1);
  if (f1 & 32)  png_set_gamma(png, 2.2, 0.45455);
  if (f1 & 64)  png_set_bgr(png);
  if (f1 & 128) png_set_packing(png);
  int passes = png_set_interlace_handling(png);
  png_read_update_info(png, info);
  row = malloc(png_get_rowbytes(png, info));
  for (int p = 0; p < passes; p++)
    for (png_uint_32 y = 0; y < png_get_image_height(png, info); y++)
      png_read_row(png, row, NULL);
  png_read_end(png, info);
out:
  free((void *)row); png_destroy_read_struct(&png, &info, NULL);
}
static void simplified(const unsigned char *buf, size_t len, unsigned f2) {
  png_image img; memset(&img, 0, sizeof img); img.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_memory(&img, buf, len)) return;
  img.format = f2 & 0x3f;
  png_color bg = { (png_byte)f2, 0x80, 0x40 };
  void *out = NULL, *cmap = NULL;
  if (img.width <= 4096 && img.height <= 4096) {
    out  = malloc(PNG_IMAGE_SIZE(img));
    cmap = malloc(PNG_IMAGE_SAMPLE_SIZE(img.format) * 256);
    if (out && cmap) png_image_finish_read(&img, (f2 & 0x40) ? &bg : NULL, out, 0, cmap);
  }
  png_image_free(&img); free(out); free(cmap);
}
int main(void) {
  static unsigned char buf[1 << 20];
  size_t len = 0; ssize_t r;
  while (len < sizeof buf && (r = read(0, buf + len, sizeof buf - len)) > 0) len += r;
  if (len < 10 || png_sig_cmp(buf, 0, 8)) return 0;
  unsigned f1 = buf[len - 1], f2 = buf[len - 2];
  lowlevel(buf, len, f1, f2);
  simplified(buf, len, f2);
  return 0;
}