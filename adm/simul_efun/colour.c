#include <simul_efun.h>

string gradient_hex(string hex, float step) {
  int *rgb = COLOUR_D->hexToRgb(hex);
  float *frgb;

  frgb = map(rgb, (: to_float :));
  frgb = map(frgb, (: clamp($1+$(step), 0.0, 255.0) :));

  rgb = map(frgb, (: to_int :));

  return sprintf("{{%02X%02X%02X}}", rgb...);
}
