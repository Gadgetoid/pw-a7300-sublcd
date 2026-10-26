/**
 * @file
 * The sub LCD backlight.
 *
 * Brightness needs pwsh2-boot's sub LCD driver; with Sharp's the backlight is
 * on or off only. See docs/how-it-works.md.
 */
#ifndef SUBLCD_BACKLIGHT_H
#define SUBLCD_BACKLIGHT_H

#include <windows.h>

/**
 * Checks whether the sub LCD backlight is on.
 *
 * @return TRUE if on, FALSE if off or the backlight driver is not running.
 */
BOOL sublcd_backlight(void);
/**
 * Turns the sub LCD backlight on or off.
 *
 * The power manager still turns it off after its idle timeout, and touching
 * the sub screen turns it on.
 *
 * @param on TRUE for on, FALSE for off.
 * @return TRUE on success, FALSE if the backlight driver is not running.
 */
BOOL sublcd_set_backlight(BOOL on);
/**
 * Keeps the sub LCD backlight on while an app runs.
 *
 * Call it at least every SUBLCD_BACKLIGHT_KEEP_ON_MS, for example from the
 * app's main loop. It turns the backlight on at most once per interval, which
 * restarts the power manager's idle timeout. Calling it more often is cheap.
 */
void sublcd_backlight_keep_on(void);
/**
 * Gets the sub LCD backlight brightness.
 *
 * @return 1 to 100, or 0 if the driver cannot dim the backlight.
 */
int sublcd_backlight_level(void);
/**
 * Sets the sub LCD backlight brightness. The driver saves it in the registry
 * and uses it whenever the backlight comes on.
 *
 * @param percent Brightness from 1 to 100, clamped.
 * @return TRUE on success, FALSE if the driver cannot dim the backlight.
 */
BOOL sublcd_set_backlight_level(int percent);

#define SUBLCD_BACKLIGHT_KEEP_ON_MS 2000

#endif
