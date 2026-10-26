/**
 * @file
 * Experiment: does SharpLib's EdSetSubLcdCeAppCompatibleMode stop the
 * dictionary shell drawing its button over the sub LCD?
 *
 * Sets the mode to 1 at start. Mode toggles it between 0 and 1, Quit exits.
 * Each step is logged to \Storage Card\sublcd-compat-mode.log.
 */
#include <windows.h>
#include <stdio.h>
#include "backlight.h"
#include "button.h"
#include "draw.h"
#include "touch.h"

#define LOG_PATH "\\Storage Card\\sublcd-compat-mode.log"

typedef void (*set_mode_function)(int mode);
typedef short (*check_mode_function)(void);

static const struct sublcd_button mode_button = { 4, SUBLCD_HEIGHT - 20, 60, 16, "Mode" };
static const struct sublcd_button quit_button = { SUBLCD_WIDTH - 44, SUBLCD_HEIGHT - 20, 40, 16, "Quit" };

static set_mode_function set_mode;
static check_mode_function check_mode;

static void log_line(const char *text)
{
    FILE *log = fopen(LOG_PATH, "ab");

    if (!log)
        return;
    fprintf(log, "%lu %s\r\n", (unsigned long)GetTickCount(), text);
    fclose(log);
}

static BOOL load_sharplib(void)
{
    HINSTANCE sharplib = LoadLibraryW(L"SharpLib.dll");

    if (!sharplib) {
        log_line("SharpLib.dll did not load");
        return FALSE;
    }
    set_mode = (set_mode_function)GetProcAddressW(sharplib, L"EdSetSubLcdCeAppCompatibleMode");
    check_mode = (check_mode_function)GetProcAddressW(sharplib, L"ChkSubLcdCeAppCompatibleMode");
    if (!set_mode || !check_mode) {
        log_line("compatible mode exports not found");
        return FALSE;
    }
    return TRUE;
}

static void apply_mode(int mode)
{
    char line[64];

    set_mode(mode);
    snprintf(line, sizeof(line), "set %d, check %d", mode, check_mode());
    log_line(line);
}

static void draw(int mode)
{
    char status[32];

    snprintf(status, sizeof(status), "Compatible mode %d", mode);
    sublcd_clear();
    sublcd_rect(0, 0, SUBLCD_WIDTH, SUBLCD_HEIGHT, SUBLCD_BLACK);
    sublcd_text(8, 8, status, 1, SUBLCD_BLACK);
    sublcd_text(8, 24, "Is Sharp's button still here?", 1, SUBLCD_BLACK);
    sublcd_button_draw(&mode_button, FALSE);
    sublcd_button_draw(&quit_button, FALSE);
    sublcd_update();
}

static void run(HANDLE touch_event)
{
    struct sublcd_touch touch;
    int mode = 1;

    apply_mode(mode);
    draw(mode);
    for (;;) {
        sublcd_backlight_keep_on();
        if (WaitForSingleObject(touch_event, SUBLCD_BACKLIGHT_KEEP_ON_MS) != WAIT_OBJECT_0)
            continue;
        while (sublcd_touch_read(&touch)) {
            if (sublcd_button_tapped(&quit_button, &touch))
                return;
            if (sublcd_button_tapped(&mode_button, &touch)) {
                mode = !mode;
                apply_mode(mode);
                draw(mode);
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    HANDLE touch_event;

    log_line("start");
    if (!load_sharplib() || !sublcd_open())
        return 1;
    touch_event = sublcd_touch_open();
    if (!touch_event) {
        log_line("TPD2: did not open");
        sublcd_close();
        return 1;
    }
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    log_line("quit");
    return 0;
}
