/**
 * @file
 * Lines, rectangles, circles and text in the sub LCD buffer.
 *
 * Every shape is built from sublcd_pixel(), which clips and packs 2-bit
 * levels.
 */
#include "draw.h"
#include <stdlib.h>
#include <string.h>

void sublcd_clear(void)
{
    memset(sublcd_buffer(), 0, SUBLCD_BUFFER_SIZE);
}

void sublcd_pixel(int x, int y, int colour)
{
    uint8_t *byte;
    int shift;

    if (x < 0 || x >= SUBLCD_WIDTH || y < 0 || y >= SUBLCD_HEIGHT)
        return;
    byte = &sublcd_buffer()[y * SUBLCD_STRIDE + x / 4];
    shift = 6 - 2 * (x % 4);
    *byte = (uint8_t)((*byte & ~(3 << shift)) | ((colour & 3) << shift));
}

static int pixel_colour(int x, int y)
{
    return (sublcd_buffer()[y * SUBLCD_STRIDE + x / 4] >> (6 - 2 * (x % 4))) & 3;
}

void sublcd_line(int x0, int y0, int x1, int y1, int colour)
{
    int step_x = x0 < x1 ? 1 : -1, step_y = y0 < y1 ? 1 : -1;
    int distance_x = abs(x1 - x0), distance_y = -abs(y1 - y0);
    int error = distance_x + distance_y, doubled_error;

    for (;;) {
        sublcd_pixel(x0, y0, colour);
        if (x0 == x1 && y0 == y1)
            return;
        doubled_error = error * 2;
        if (doubled_error >= distance_y) {
            error += distance_y;
            x0 += step_x;
        }
        if (doubled_error <= distance_x) {
            error += distance_x;
            y0 += step_y;
        }
    }
}

void sublcd_rect(int x, int y, int width, int height, int colour)
{
    sublcd_fill_rect(x, y, width, 1, colour);
    sublcd_fill_rect(x, y + height - 1, width, 1, colour);
    sublcd_fill_rect(x, y, 1, height, colour);
    sublcd_fill_rect(x + width - 1, y, 1, height, colour);
}

void sublcd_fill_rect(int x, int y, int width, int height, int colour)
{
    int row, column;

    for (row = y; row < y + height; row++)
        for (column = x; column < x + width; column++)
            sublcd_pixel(column, row, colour);
}

void sublcd_invert_rect(int x, int y, int width, int height)
{
    int row, column;

    for (row = y; row < y + height; row++)
        for (column = x; column < x + width; column++)
            if (column >= 0 && column < SUBLCD_WIDTH && row >= 0 && row < SUBLCD_HEIGHT)
                sublcd_pixel(column, row, SUBLCD_BLACK - pixel_colour(column, row));
}

void sublcd_circle(int centre_x, int centre_y, int radius, int colour)
{
    int x = radius, y = 0, error = 1 - radius;

    while (x >= y) {
        sublcd_pixel(centre_x + x, centre_y + y, colour);
        sublcd_pixel(centre_x - x, centre_y + y, colour);
        sublcd_pixel(centre_x + x, centre_y - y, colour);
        sublcd_pixel(centre_x - x, centre_y - y, colour);
        sublcd_pixel(centre_x + y, centre_y + x, colour);
        sublcd_pixel(centre_x - y, centre_y + x, colour);
        sublcd_pixel(centre_x + y, centre_y - x, colour);
        sublcd_pixel(centre_x - y, centre_y - x, colour);
        y++;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            x--;
            error += 2 * (y - x) + 1;
        }
    }
}

static void draw_glyph(int x, int y, char character, int scale, int colour)
{
    int glyph = character - SUBLCD_FONT_FIRST, column, row;

    if (glyph < 0 || glyph >= SUBLCD_FONT_GLYPHS)
        glyph = '?' - SUBLCD_FONT_FIRST;
    for (column = 0; column < SUBLCD_FONT_WIDTH; column++)
        for (row = 0; row < SUBLCD_FONT_HEIGHT; row++)
            if (sublcd_font[glyph][column] & (1 << row))
                sublcd_fill_rect(x + column * scale, y + row * scale, scale, scale, colour);
}

int sublcd_text(int x, int y, const char *text, int scale, int colour)
{
    for (; *text; text++) {
        draw_glyph(x, y, *text, scale, colour);
        x += SUBLCD_FONT_ADVANCE * scale;
    }
    return x;
}

int sublcd_text_width(const char *text, int scale)
{
    int length = (int)strlen(text);

    return length ? (length * SUBLCD_FONT_ADVANCE - 1) * scale : 0;
}
