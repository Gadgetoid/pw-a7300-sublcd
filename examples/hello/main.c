/**
 * @file
 * Hello world: a greeting on the sub LCD until Quit is tapped on either
 * screen.
 */
#include <windows.h>
#include "backlight.h"
#include "button.h"
#include "draw.h"
#include "touch.h"
#include "window.h"

#define TITLE L"Hello"

static const struct sublcd_button quit_button = { SUBLCD_WIDTH - 44, SUBLCD_HEIGHT - 20, 40, 16, "Quit" };

static void draw_greeting(void)
{
    const char *greeting = "Hello, world!";
    int scale = 2;

    sublcd_clear();
    sublcd_rect(0, 0, SUBLCD_WIDTH, SUBLCD_HEIGHT, SUBLCD_BLACK);
    sublcd_text((SUBLCD_WIDTH - sublcd_text_width(greeting, scale)) / 2, (SUBLCD_HEIGHT - SUBLCD_FONT_HEIGHT * scale) / 2,
        greeting, scale, SUBLCD_BLACK);
    sublcd_button_draw(&quit_button, FALSE);
    sublcd_update();
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
    sublcd_window_open(instance, TITLE, L"Hello world is on the sub screen.", NULL);
    draw_greeting();
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
