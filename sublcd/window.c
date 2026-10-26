/**
 * @file
 * Main-screen window with a message and a Quit button.
 *
 * The client area is split in thirds: the message at the top, space for the
 * app's own controls in the middle, and the Quit button at the bottom.
 */
#include "window.h"

#define WINDOW_CLASS L"sublcd-window"
#define QUIT_BUTTON_ID 1
#define QUIT_BUTTON_WIDTH 140
#define QUIT_BUTTON_HEIGHT 56

static HWND window;
static const WCHAR *window_message;
static sublcd_key_function key_function;
static BOOL open;

static void paint(void)
{
    PAINTSTRUCT paint_state;
    HDC context = BeginPaint(window, &paint_state);
    RECT area;

    GetClientRect(window, &area);
    area.bottom /= 3;
    InflateRect(&area, -8, -8);
    DrawTextW(context, window_message, -1, &area, DT_CENTER | DT_WORDBREAK);
    EndPaint(window, &paint_state);
}

static LRESULT CALLBACK window_procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wparam) == QUIT_BUTTON_ID)
            DestroyWindow(handle);
        return 0;
    case WM_KEYDOWN:
        if (key_function)
            key_function(wparam);
        return 0;
    case WM_PAINT:
        paint();
        return 0;
    case WM_DESTROY:
        open = FALSE;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(handle, message, wparam, lparam);
}

HWND sublcd_window_open(HINSTANCE instance, const WCHAR *title, const WCHAR *message, sublcd_key_function on_key)
{
    WNDCLASSW window_class = { 0 };
    RECT area;

    window_message = message;
    key_function = on_key;
    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    window_class.lpszClassName = WINDOW_CLASS;
    RegisterClassW(&window_class);
    window = CreateWindowExW(0, WINDOW_CLASS, title, WS_VISIBLE | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, instance, NULL);
    if (!window)
        return NULL;
    GetClientRect(window, &area);
    CreateWindowExW(0, L"BUTTON", L"Quit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (area.right - QUIT_BUTTON_WIDTH) / 2,
        area.bottom * 5 / 6 - QUIT_BUTTON_HEIGHT / 2, QUIT_BUTTON_WIDTH, QUIT_BUTTON_HEIGHT, window, (HMENU)QUIT_BUTTON_ID, instance, NULL);
    SetFocus(window);
    open = TRUE;
    return window;
}

BOOL sublcd_window_close_running(const WCHAR *title)
{
    HWND running = FindWindowW(WINDOW_CLASS, title);

    if (!running)
        return FALSE;
    PostMessageW(running, WM_CLOSE, 0, 0);
    return TRUE;
}

void sublcd_window_free_area(RECT *area)
{
    GetClientRect(window, area);
    area->top = area->bottom / 3;
    area->bottom = area->bottom * 2 / 3;
}

BOOL sublcd_window_pump(void)
{
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT)
            return FALSE;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return open;
}

void sublcd_window_quit(void)
{
    if (open)
        DestroyWindow(window);
}
