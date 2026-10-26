/**
 * @file
 * Labelled touch buttons on the sub LCD.
 */
#ifndef SUBLCD_BUTTON_H
#define SUBLCD_BUTTON_H

#include <windows.h>
#include "touch.h"

struct sublcd_button {
    int x;
    int y;
    int width;
    int height;
    const char *label;
};

/**
 * Draws a button: an outline with its label centred, inverted when pressed.
 *
 * @param button The button.
 * @param pressed TRUE to draw it pressed.
 */
void sublcd_button_draw(const struct sublcd_button *button, BOOL pressed);
/**
 * Checks whether a point is on a button.
 *
 * @param button The button.
 * @param x Column.
 * @param y Row.
 * @return TRUE if the point is inside the button.
 */
BOOL sublcd_button_contains(const struct sublcd_button *button, int x, int y);
/**
 * Checks whether a touch sample completes a tap on a button: a release
 * inside it.
 *
 * @param button The button.
 * @param touch A sample from sublcd_touch_read().
 * @return TRUE if the sample is a release on the button.
 */
BOOL sublcd_button_tapped(const struct sublcd_button *button, const struct sublcd_touch *touch);

#endif
