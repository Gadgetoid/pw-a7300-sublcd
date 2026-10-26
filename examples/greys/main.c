/**
 * @file
 * Greys: the four grey levels and the backlight brightness, with - and +
 * to dim and brighten it, until Quit is tapped on either screen.
 */
#include <windows.h>
#include <stdio.h>
#include "backlight.h"
#include "button.h"
#include "draw.h"
#include "touch.h"
#include "window.h"

#define TITLE L"Greys"
#define BAR_TOP 4
#define BAR_HEIGHT 64
#define BAR_WIDTH (SUBLCD_WIDTH / 4)
#define CONTROLS_TOP (SUBLCD_HEIGHT - 20)
#define BRIGHTNESS_STEP 10

static const struct sublcd_button dimmer_button = { 4, CONTROLS_TOP, 24, 16, "-" };
static const struct sublcd_button brighter_button = { 100, CONTROLS_TOP, 24, 16, "+" };
static const struct sublcd_button quit_button = { SUBLCD_WIDTH - 44, CONTROLS_TOP, 40, 16, "Quit" };
static const char *const level_names[] = { "White", "Light", "Dark", "Black" };

static void draw_bars(void)
{
    int level;

    for (level = SUBLCD_WHITE; level <= SUBLCD_BLACK; level++) {
        int left = level * BAR_WIDTH;
        int text_colour = level >= SUBLCD_DARK ? SUBLCD_WHITE : SUBLCD_BLACK;

        sublcd_fill_rect(left, BAR_TOP, BAR_WIDTH, BAR_HEIGHT, level);
        sublcd_text(left + (BAR_WIDTH - sublcd_text_width(level_names[level], 1)) / 2, BAR_TOP + BAR_HEIGHT / 2 - 3,
            level_names[level], 1, text_colour);
    }
    sublcd_rect(0, BAR_TOP, SUBLCD_WIDTH, BAR_HEIGHT, SUBLCD_BLACK);
    if (!sublcd_has_grey())
        sublcd_text(4, BAR_TOP + BAR_HEIGHT + 2, "Sharp's driver: black and white", 1, SUBLCD_BLACK);
}

static void draw_brightness(void)
{
    char text[16];
    int level = sublcd_backlight_level();

    if (level)
        sprintf(text, "%d%%", level);
    else
        sprintf(text, "on/off");
    sublcd_fill_rect(32, CONTROLS_TOP, 64, 16, SUBLCD_WHITE);
    sublcd_text(64 - sublcd_text_width(text, 1) / 2, CONTROLS_TOP + 5, text, 1, SUBLCD_BLACK);
}

static void step_brightness(int step)
{
    int level = sublcd_backlight_level();

    if (!level)
        return;
    level = (level + step) / BRIGHTNESS_STEP * BRIGHTNESS_STEP;
    sublcd_set_backlight_level(level < 1 ? 1 : level);
    draw_brightness();
    sublcd_update();
}

static void run(HANDLE touch_event)
{
    struct sublcd_touch touch;

    while (sublcd_window_pump()) {
        sublcd_backlight_keep_on();
        if (MsgWaitForMultipleObjectsEx(1, &touch_event, SUBLCD_BACKLIGHT_KEEP_ON_MS, QS_ALLINPUT, 0) != WAIT_OBJECT_0)
            continue;
        while (sublcd_touch_read(&touch)) {
            if (sublcd_button_tapped(&quit_button, &touch))
                sublcd_window_quit();
            else if (sublcd_button_tapped(&dimmer_button, &touch))
                step_brightness(-BRIGHTNESS_STEP);
            else if (sublcd_button_tapped(&brighter_button, &touch))
                step_brightness(BRIGHTNESS_STEP);
        }
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
    sublcd_window_open(instance, TITLE, L"Greys is on the sub screen.", NULL);
    sublcd_clear();
    draw_bars();
    sublcd_button_draw(&dimmer_button, FALSE);
    sublcd_button_draw(&brighter_button, FALSE);
    sublcd_button_draw(&quit_button, FALSE);
    draw_brightness();
    sublcd_update();
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
