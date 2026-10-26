/**
 * @file
 * Drawing and hit testing for sub LCD touch buttons.
 */
#include "button.h"
#include "draw.h"

void sublcd_button_draw(const struct sublcd_button *button, BOOL pressed)
{
    int label_x = button->x + (button->width - sublcd_text_width(button->label, 1)) / 2;
    int label_y = button->y + (button->height - SUBLCD_FONT_HEIGHT) / 2;

    sublcd_fill_rect(button->x, button->y, button->width, button->height, SUBLCD_WHITE);
    sublcd_rect(button->x, button->y, button->width, button->height, SUBLCD_BLACK);
    sublcd_text(label_x, label_y, button->label, 1, SUBLCD_BLACK);
    if (pressed)
        sublcd_invert_rect(button->x + 1, button->y + 1, button->width - 2, button->height - 2);
}

BOOL sublcd_button_contains(const struct sublcd_button *button, int x, int y)
{
    return x >= button->x && x < button->x + button->width && y >= button->y && y < button->y + button->height;
}

BOOL sublcd_button_tapped(const struct sublcd_button *button, const struct sublcd_touch *touch)
{
    return touch->state == SUBLCD_TOUCH_RELEASE && sublcd_button_contains(button, touch->x, touch->y);
}
