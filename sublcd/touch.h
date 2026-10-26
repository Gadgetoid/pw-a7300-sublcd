/**
 * @file
 * The sub LCD's resistive touch panel, through Sharp's TPD2: driver.
 *
 * Only the most recently opened handle receives samples, so opening touch
 * takes it from any other app using it (such as Touchpad).
 */
#ifndef SUBLCD_TOUCH_H
#define SUBLCD_TOUCH_H

#include <windows.h>

enum sublcd_touch_state {
    SUBLCD_TOUCH_MOVE = 1,
    SUBLCD_TOUCH_RELEASE = 2,
    SUBLCD_TOUCH_PRESS = 3,
};

struct sublcd_touch {
    int x;
    int y;
    enum sublcd_touch_state state;
};

/**
 * Starts receiving touch samples.
 *
 * @return An event that is signalled when samples are waiting, for use with
 *         WaitForSingleObject() or MsgWaitForMultipleObjectsEx(), or NULL if
 *         TPD2: is not available. Owned by the library.
 */
HANDLE sublcd_touch_open(void);
/**
 * Stops receiving touch samples.
 */
void sublcd_touch_close(void);
/**
 * Takes the next waiting sample. Does not block.
 *
 * Coordinates are sub LCD pixels. Every touch starts with a press, even when
 * the driver drops its first sample. A release reports the position of the
 * last sample before it.
 *
 * @param touch Receives the sample.
 * @return TRUE if a sample was taken, FALSE if none are waiting.
 */
BOOL sublcd_touch_read(struct sublcd_touch *touch);

#endif
