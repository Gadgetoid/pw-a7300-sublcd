/**
 * @file
 * The buzzer that clicks for key presses and screen taps.
 *
 * It has one pitch; only the length of a beep can be chosen.
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

#endif
