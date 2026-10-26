/**
 * @file
 * Drawing into the sub LCD buffer.
 *
 * Everything is clipped to the screen. Nothing reaches the panel until
 * sublcd_update().
 */
#ifndef SUBLCD_DRAW_H
#define SUBLCD_DRAW_H

#include "sublcd.h"
#include "font.h"

#define SUBLCD_WHITE 0
#define SUBLCD_LIGHT 1
#define SUBLCD_DARK 2
#define SUBLCD_BLACK 3

/**
 * Fills the buffer with white.
 */
void sublcd_clear(void);
/**
 * Sets one pixel.
 *
 * @param x Column, 0 at the left.
 * @param y Row, 0 at the top.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 */
void sublcd_pixel(int x, int y, int colour);
/**
 * Draws a one-pixel line, both ends included.
 *
 * @param x0 Start column.
 * @param y0 Start row.
 * @param x1 End column.
 * @param y1 End row.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 */
void sublcd_line(int x0, int y0, int x1, int y1, int colour);
/**
 * Draws a one-pixel rectangle outline.
 *
 * @param x Left column.
 * @param y Top row.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 */
void sublcd_rect(int x, int y, int width, int height, int colour);
/**
 * Fills a rectangle.
 *
 * @param x Left column.
 * @param y Top row.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 */
void sublcd_fill_rect(int x, int y, int width, int height, int colour);
/**
 * Inverts a rectangle: black and white swap, as do the two greys.
 *
 * @param x Left column.
 * @param y Top row.
 * @param width Width in pixels.
 * @param height Height in pixels.
 */
void sublcd_invert_rect(int x, int y, int width, int height);
/**
 * Draws a one-pixel circle outline.
 *
 * @param centre_x Centre column.
 * @param centre_y Centre row.
 * @param radius Radius in pixels.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 */
void sublcd_circle(int centre_x, int centre_y, int radius, int colour);
/**
 * Draws text in the 5x7 font. Characters outside ' ' to '~' draw as '?'.
 *
 * @param x Left column.
 * @param y Top row.
 * @param text The text.
 * @param scale Pixel size, 1 for 5x7 glyphs, 2 for 10x14 and so on.
 * @param colour SUBLCD_WHITE, SUBLCD_LIGHT, SUBLCD_DARK or SUBLCD_BLACK.
 * @return The column after the last character.
 */
int sublcd_text(int x, int y, const char *text, int scale, int colour);
/**
 * Measures text drawn by sublcd_text().
 *
 * @param text The text.
 * @param scale Pixel size.
 * @return The width in pixels, without trailing space.
 */
int sublcd_text_width(const char *text, int scale);

#endif
