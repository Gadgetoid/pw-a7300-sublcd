/**
 * @file
 * A main-screen window for sub LCD apps: a message and a Quit button.
 *
 * It makes quitting obvious, receives keys, and covers the dictionary's
 * "Please wait" message, which stays up in dictionary mode otherwise.
 */
#ifndef SUBLCD_WINDOW_H
#define SUBLCD_WINDOW_H

#include <windows.h>

typedef void (*sublcd_key_function)(WPARAM key);

/**
 * Opens the window. Only one per app.
 *
 * @param instance The app's instance handle.
 * @param title Caption, also used to find a running copy.
 * @param message Text shown above the Quit button.
 * @param on_key Called for each key press, or NULL.
 * @return The window, or NULL if it could not be created.
 */
HWND sublcd_window_open(HINSTANCE instance, const WCHAR *title, const WCHAR *message, sublcd_key_function on_key);
/**
 * Closes a copy of the app that is already running.
 *
 * @param title The caption passed to sublcd_window_open().
 * @return TRUE if a running copy was found and asked to close.
 */
BOOL sublcd_window_close_running(const WCHAR *title);
/**
 * Gets the area between the message and the Quit button, for the app's
 * own controls.
 *
 * @param area Receives the area in client coordinates.
 */
void sublcd_window_free_area(RECT *area);
/**
 * Handles waiting window messages. Call when MsgWaitForMultipleObjectsEx()
 * reports input.
 *
 * @return TRUE while the window is open, FALSE once Quit or close is chosen.
 */
BOOL sublcd_window_pump(void);
/**
 * Closes the window, as if Quit were chosen.
 */
void sublcd_window_quit(void);

#endif
