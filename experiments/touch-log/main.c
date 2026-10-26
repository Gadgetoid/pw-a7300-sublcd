/**
 * @file
 * Experiment: why do sub LCD taps go missing in dictionary mode?
 *
 * Logs each driver call's result and every touch sample to
 * \Storage Card\sublcd-touch-log.log, and draws a dot on the sub LCD where
 * each sample lands. A main-screen window shows the sample count and has a
 * Quit button, so it can be closed even if sub LCD touch never arrives.
 */
#include <windows.h>
#include <stdio.h>
#include "backlight.h"
#include "draw.h"
#include "touch.h"

#define LOG_PATH "\\Storage Card\\sublcd-touch-log.log"
#define WINDOW_CLASS L"sublcd-touch-log"
#define QUIT_BUTTON_ID 1

static HWND main_window;
static int sample_count;
static BOOL running = TRUE;

static void log_line(const char *format, ...)
{
    FILE *log = fopen(LOG_PATH, "ab");
    va_list arguments;

    if (!log)
        return;
    fprintf(log, "%lu ", (unsigned long)GetTickCount());
    va_start(arguments, format);
    vfprintf(log, format, arguments);
    va_end(arguments);
    fputs("\r\n", log);
    fclose(log);
}

static void draw_status(void)
{
    char status[32];

    snprintf(status, sizeof(status), "Samples %d", sample_count);
    sublcd_fill_rect(0, 0, SUBLCD_WIDTH, 10, SUBLCD_WHITE);
    sublcd_text(2, 2, status, 1, SUBLCD_BLACK);
    sublcd_update();
    InvalidateRect(main_window, NULL, TRUE);
}

static void handle_touch(void)
{
    struct sublcd_touch touch;

    while (sublcd_touch_read(&touch)) {
        sample_count++;
        log_line("sample state %d at %d,%d", touch.state, touch.x, touch.y);
        sublcd_fill_rect(touch.x - 1, touch.y - 1, 3, 3, SUBLCD_BLACK);
    }
    draw_status();
}

static void paint(HWND window)
{
    PAINTSTRUCT paint_state;
    HDC context = BeginPaint(window, &paint_state);
    WCHAR text[64];
    RECT area;

    GetClientRect(window, &area);
    area.bottom = area.bottom / 2;
    wsprintfW(text, L"Sub LCD touch samples: %d", sample_count);
    DrawTextW(context, text, -1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    EndPaint(window, &paint_state);
}

static LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wparam) == QUIT_BUTTON_ID)
            DestroyWindow(window);
        return 0;
    case WM_PAINT:
        paint(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

static HWND create_window(HINSTANCE instance)
{
    WNDCLASSW window_class = { 0 };
    RECT area;
    HWND window;

    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    window_class.lpszClassName = WINDOW_CLASS;
    RegisterClassW(&window_class);
    window = CreateWindowExW(0, WINDOW_CLASS, L"Touch Log", WS_VISIBLE | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
    GetClientRect(window, &area);
    CreateWindowExW(0, L"BUTTON", L"Quit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, area.right / 2 - 60, area.bottom * 2 / 3 - 30, 120, 60,
        window, (HMENU)QUIT_BUTTON_ID, instance, NULL);
    return window;
}

static void pump_messages(void)
{
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            running = FALSE;
            return;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    HANDLE touch_event;
    DWORD woken;

    log_line("start");
    main_window = create_window(instance);
    log_line("window %p", main_window);
    if (!sublcd_open()) {
        log_line("SLD1: open failed, error %lu", (unsigned long)GetLastError());
        return 1;
    }
    log_line("SLD1: open");
    touch_event = sublcd_touch_open();
    log_line("TPD2: %s, error %lu", touch_event ? "open" : "open failed", (unsigned long)GetLastError());
    sublcd_clear();
    sublcd_text(2, 60, "Tap anywhere", 1, SUBLCD_BLACK);
    log_line("update %d", sublcd_update());
    draw_status();
    while (running) {
        sublcd_backlight_keep_on();
        woken = MsgWaitForMultipleObjectsEx(touch_event ? 1 : 0, &touch_event, SUBLCD_BACKLIGHT_KEEP_ON_MS, QS_ALLINPUT, 0);
        if (touch_event && woken == WAIT_OBJECT_0)
            handle_touch();
        else
            pump_messages();
    }
    log_line("quit after %d samples", sample_count);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
