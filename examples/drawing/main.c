/**
 * @file
 * Drawing: one panel each for lines, rectangles, circles and text, until Quit
 * is tapped on either screen.
 */
#include <windows.h>
#include "backlight.h"
#include "button.h"
#include "draw.h"
#include "touch.h"
#include "window.h"

#define TITLE L"Drawing"
#define PANEL_WIDTH (SUBLCD_WIDTH / 2)
#define PANEL_HEIGHT (SUBLCD_HEIGHT / 2)

static const struct sublcd_button quit_button = { SUBLCD_WIDTH - 44, SUBLCD_HEIGHT - 20, 40, 16, "Quit" };

static void draw_lines(int left, int top)
{
    int step;

    for (step = 0; step <= 8; step++) {
        sublcd_line(left + 4, top + PANEL_HEIGHT - 4, left + 4 + step * 13, top + 4, SUBLCD_BLACK);
        sublcd_line(left + 4, top + PANEL_HEIGHT - 4, left + PANEL_WIDTH - 4, top + 4 + step * 6, SUBLCD_BLACK);
    }
}

static void draw_rectangles(int left, int top)
{
    int inset;

    for (inset = 4; inset < 24; inset += 5)
        sublcd_rect(left + inset, top + inset, PANEL_WIDTH / 2 - inset * 2 + 24, PANEL_HEIGHT - inset * 2, SUBLCD_BLACK);
    sublcd_fill_rect(left + 84, top + 10, 28, 40, SUBLCD_BLACK);
    sublcd_fill_rect(left + 90, top + 16, 16, 28, SUBLCD_WHITE);
}

static void draw_circles(int left, int top)
{
    int radius;

    for (radius = 4; radius <= 24; radius += 5)
        sublcd_circle(left + 32, top + PANEL_HEIGHT / 2, radius, SUBLCD_BLACK);
    sublcd_circle(left + 88, top + PANEL_HEIGHT / 2, 20, SUBLCD_BLACK);
    sublcd_invert_rect(left + 68, top + 10, 20, 40);
}

static void draw_text(int left, int top)
{
    sublcd_text(left + 6, top + 6, "Text 1x", 1, SUBLCD_BLACK);
    sublcd_text(left + 6, top + 18, "Text 2x", 2, SUBLCD_BLACK);
    sublcd_fill_rect(left + 4, top + 40, 64, 16, SUBLCD_BLACK);
    sublcd_text(left + 8, top + 45, "Inverted", 1, SUBLCD_WHITE);
}

static void run(HANDLE touch_event)
{
    struct sublcd_touch touch;

    while (sublcd_window_pump()) {
        sublcd_backlight_keep_on();
        if (MsgWaitForMultipleObjectsEx(1, &touch_event, SUBLCD_BACKLIGHT_KEEP_ON_MS, QS_ALLINPUT, 0) != WAIT_OBJECT_0)
            continue;
        while (sublcd_touch_read(&touch))
            if (sublcd_button_tapped(&quit_button, &touch))
                sublcd_window_quit();
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    HANDLE touch_event;

    if (sublcd_window_close_running(TITLE))
        return 0;
    if (!sublcd_open()) {
        MessageBoxW(NULL, L"The sub LCD (SLD1:) is not available.", TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    touch_event = sublcd_touch_open();
    if (!touch_event) {
        MessageBoxW(NULL, L"The sub touch panel (TPD2:) is not available.", TITLE, MB_OK | MB_ICONERROR);
        sublcd_close();
        return 1;
    }
    sublcd_window_open(instance, TITLE, L"Drawing is on the sub screen.", NULL);
    sublcd_clear();
    draw_lines(0, 0);
    draw_rectangles(PANEL_WIDTH, 0);
    draw_circles(0, PANEL_HEIGHT);
    draw_text(PANEL_WIDTH, PANEL_HEIGHT);
    sublcd_line(PANEL_WIDTH, 0, PANEL_WIDTH, SUBLCD_HEIGHT - 1, SUBLCD_BLACK);
    sublcd_line(0, PANEL_HEIGHT, SUBLCD_WIDTH - 1, PANEL_HEIGHT, SUBLCD_BLACK);
    sublcd_button_draw(&quit_button, FALSE);
    sublcd_update();
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
