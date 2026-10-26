/**
 * @file
 * The buzzer that clicks for key presses and screen taps.
 *
 * Through Sharp's buzzer driver it has one pitch; only the length of a beep
 * can be chosen. pwsh2-boot's sub LCD driver adds sublcd_tone() for any
 * pitch.
 */
#ifndef SUBLCD_BUZZER_H
#define SUBLCD_BUZZER_H

#include <windows.h>

/**
 * Starts a beep and returns without waiting for it to finish.
 *
 * @param milliseconds Length of the beep.
 * @return TRUE on success, FALSE if the buzzer driver refused it.
 */
BOOL sublcd_beep(DWORD milliseconds);
/**
 * Plays a continuous tone until the next call changes or stops it.
 *
 * @param frequency Pitch in hertz, or 0 to stop.
 * @return TRUE on success, FALSE if the driver cannot play tones (use
 *         sublcd_beep() instead).
 */
BOOL sublcd_tone(DWORD frequency);

#endif
