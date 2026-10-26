/**
 * @file
 * Tiny keyboard: a touch keyboard on the sub LCD that types into the focused
 * window on the main screen.
 *
 * Keys go out through keybd_event as if pressed on the real keyboard, so they
 * reach whichever window has focus. Its own window has a text box to type
 * into, which is the only target in dictionary mode; on the CE desktop, tap
 * another app to type there. Sub LCD taps never move focus. Shift applies to
 * the next key. Quit on either screen, or launching it again, closes it.
 */
#include <windows.h>
#include "backlight.h"
#include "button.h"
#include "draw.h"
#include "touch.h"
#include "window.h"

#define TITLE L"Keyboard"
#define KEY_SIZE 24
#define CHARACTER_ROWS 4
#define MAXIMUM_KEYS 44

#ifndef VK_OEM_COMMA
#define VK_OEM_COMMA 0xbc
#define VK_OEM_MINUS 0xbd
#define VK_OEM_PERIOD 0xbe
#endif

struct key {
    struct sublcd_button button;
    char label[2];
    BYTE virtual_key;
};

static const char *const character_rows[CHARACTER_ROWS] = { "1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm,.-" };

static struct key keys[MAXIMUM_KEYS];
static int key_count;
static BOOL shifted;

static BYTE character_virtual_key(char character)
{
    switch (character) {
    case ',':
        return VK_OEM_COMMA;
    case '.':
        return VK_OEM_PERIOD;
    case '-':
        return VK_OEM_MINUS;
    }
    return (BYTE)(character >= 'a' && character <= 'z' ? character - 'a' + 'A' : character);
}

static void add_key(int column, int row, int width, const char *label, BYTE virtual_key)
{
    struct key *key = &keys[key_count++];

    key->button.x = column * KEY_SIZE;
    key->button.y = row * KEY_SIZE;
    key->button.width = width * KEY_SIZE;
    key->button.height = KEY_SIZE;
    key->button.label = label;
    key->virtual_key = virtual_key;
}

static void build_layout(void)
{
    int row, column;

    for (row = 0; row < CHARACTER_ROWS; row++) {
        for (column = 0; character_rows[row][column]; column++) {
            struct key *key = &keys[key_count];

            key->label[0] = character_rows[row][column];
            add_key(column, row, 1, key->label, character_virtual_key(key->label[0]));
        }
    }
    add_key(9, 2, 1, "BS", VK_BACK);
    add_key(0, 4, 2, "Shift", VK_SHIFT);
    add_key(2, 4, 4, "Space", VK_SPACE);
    add_key(6, 4, 2, "Enter", VK_RETURN);
    add_key(8, 4, 2, "Quit", 0);
}

static void set_shifted(BOOL on)
{
    int index;

    shifted = on;
    for (index = 0; index < key_count; index++) {
        char *label = keys[index].label;

        if (label[0] >= 'a' && label[0] <= 'z' && on)
            label[0] = (char)(label[0] - 'a' + 'A');
        else if (label[0] >= 'A' && label[0] <= 'Z' && !on)
            label[0] = (char)(label[0] - 'A' + 'a');
    }
}

static void draw_keyboard(const struct key *pressed)
{
    int index;

    sublcd_clear();
    for (index = 0; index < key_count; index++) {
        const struct key *key = &keys[index];

        sublcd_button_draw(&key->button, key == pressed || (key->virtual_key == VK_SHIFT && shifted));
    }
    sublcd_update();
}

static struct key *key_at(int x, int y)
{
    int index;

    for (index = 0; index < key_count; index++)
        if (sublcd_button_contains(&keys[index].button, x, y))
            return &keys[index];
    return NULL;
}

static void type_key(BYTE virtual_key)
{
    if (shifted)
        keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event(virtual_key, 0, 0, 0);
    keybd_event(virtual_key, 0, KEYEVENTF_KEYUP, 0);
    if (shifted)
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
}

static BOOL press_key(const struct key *key)
{
    if (!key->virtual_key)
        return FALSE;
    if (key->virtual_key == VK_SHIFT) {
        set_shifted(!shifted);
        return TRUE;
    }
    type_key(key->virtual_key);
    set_shifted(FALSE);
    return TRUE;
}

static void create_text_box(HINSTANCE instance, HWND window)
{
    RECT area;
    HWND text_box;

    sublcd_window_free_area(&area);
    InflateRect(&area, -16, -8);
    text_box = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, area.left, area.top,
        area.right - area.left, area.bottom - area.top, window, NULL, instance, NULL);
    SetFocus(text_box);
}

static void run(HANDLE touch_event)
{
    struct sublcd_touch touch;
    struct key *pressed = NULL;

    draw_keyboard(NULL);
    while (sublcd_window_pump()) {
        sublcd_backlight_keep_on();
        if (MsgWaitForMultipleObjectsEx(1, &touch_event, SUBLCD_BACKLIGHT_KEEP_ON_MS, QS_ALLINPUT, 0) != WAIT_OBJECT_0)
            continue;
        while (sublcd_touch_read(&touch)) {
            if (touch.state == SUBLCD_TOUCH_PRESS) {
                pressed = key_at(touch.x, touch.y);
            } else if (touch.state == SUBLCD_TOUCH_RELEASE) {
                if (pressed && pressed == key_at(touch.x, touch.y) && !press_key(pressed))
                    sublcd_window_quit();
                pressed = NULL;
            }
        }
        draw_keyboard(pressed);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    HANDLE touch_event;
    HWND window;

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
    window = sublcd_window_open(instance, TITLE, L"Type on the sub screen. Keys go to the focused window: this text box, or tap another app.", NULL);
    create_text_box(instance, window);
    build_layout();
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
