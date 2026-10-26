/**
 * @file
 * Backlight: toggles the sub LCD backlight, sets it with "on" or "off", or
 * sets its brightness with a number from 1 to 100 on the command line.
 */
#include <windows.h>
#include "backlight.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    BOOL on = !sublcd_backlight();
    int level = _wtoi(command_line);

    if (level > 0) {
        if (!sublcd_set_backlight_level(level)) {
            MessageBoxW(NULL, L"This sub LCD driver cannot dim the backlight.", L"Backlight", MB_OK | MB_ICONERROR);
            return 1;
        }
        on = TRUE;
    } else if (!lstrcmpiW(command_line, L"on"))
        on = TRUE;
    else if (!lstrcmpiW(command_line, L"off"))
        on = FALSE;
    if (!sublcd_set_backlight(on)) {
        MessageBoxW(NULL, L"The sub backlight event is not available.", L"Backlight", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
