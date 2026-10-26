/**
 * @file
 * A 5x7 font for printable ASCII, from the classic glcdfont.
 *
 * Each glyph is five columns, top pixel in bit 0.
 */
#ifndef SUBLCD_FONT_H
#define SUBLCD_FONT_H

#include <stdint.h>

#define SUBLCD_FONT_FIRST ' '
#define SUBLCD_FONT_GLYPHS 95
#define SUBLCD_FONT_WIDTH 5
#define SUBLCD_FONT_HEIGHT 7
#define SUBLCD_FONT_ADVANCE (SUBLCD_FONT_WIDTH + 1)

/**
 * Glyphs for ' ' to '~'.
 */
extern const uint8_t sublcd_font[SUBLCD_FONT_GLYPHS][SUBLCD_FONT_WIDTH];

#endif
