/**
 * @file
 * The PW-A7300's 240x120 sub LCD, drawn through SLD1:.
 *
 * Draw into the buffer with the functions in draw.h, then call
 * sublcd_update() to send the rows that changed to the panel. With
 * pwsh2-boot's sub LCD driver the panel shows four grey levels; with
 * Sharp's, levels 2 and 3 show black and 0 and 1 white.
 */
#ifndef SUBLCD_H
#define SUBLCD_H

#include <windows.h>
#include <stdint.h>

#define SUBLCD_WIDTH 240
#define SUBLCD_HEIGHT 120
#define SUBLCD_STRIDE (SUBLCD_WIDTH / 4)
#define SUBLCD_BUFFER_SIZE (SUBLCD_STRIDE * SUBLCD_HEIGHT)

/**
 * Opens the sub LCD and turns the panel on.
 *
 * The next sublcd_update() sends the whole buffer.
 *
 * @return TRUE on success, FALSE if SLD1: is not available.
 */
BOOL sublcd_open(void);
/**
 * Closes the sub LCD. The last image stays on the panel.
 */
void sublcd_close(void);
/**
 * Sends the rows that changed since the last update to the panel.
 *
 * @return TRUE on success, or when nothing changed, FALSE if the driver
 *         refused the draw.
 */
BOOL sublcd_update(void);
/**
 * Gets the buffer: 2 bits per pixel, row by row, SUBLCD_STRIDE bytes per row,
 * leftmost pixel in the top two bits. 0 is white, 3 is black.
 *
 * @return The SUBLCD_BUFFER_SIZE-byte buffer.
 */
uint8_t *sublcd_buffer(void);
/**
 * Checks whether the panel shows grey: pwsh2-boot's sub LCD driver is running.
 *
 * @return TRUE for four grey levels, FALSE for black and white.
 */
BOOL sublcd_has_grey(void);
/**
 * Sets the panel contrast. The driver keeps it until restart.
 *
 * @param percent Contrast from 0 to 100, clamped.
 * @return TRUE on success, FALSE otherwise.
 */
BOOL sublcd_set_contrast(int percent);

#endif
