/**
 * @file
 * Beeps through BZR1: (EDNA2_Buzzer.dll).
 *
 * BZR_IOControl code 0x8000200F queues a sound for the driver's buzzer thread
 * in the same form as its registry events: mode 1 plays alarm pattern
 * `alarm` for `milliseconds`. Pattern 1 has no length limit of its own.
 */
#include "buzzer.h"

#define BUZZER_DEVICE L"BZR1:"
#define IOCTL_BZR_PLAY 0x8000200F
#define BUZZER_MODE_ALARM 1
#define BUZZER_ALARM_CONTINUOUS 1

struct buzzer_request {
    DWORD mode;
    DWORD milliseconds;
    DWORD alarm;
    DWORD reserved[4];
};

BOOL sublcd_beep(DWORD milliseconds)
{
    struct buzzer_request request = { BUZZER_MODE_ALARM, milliseconds, BUZZER_ALARM_CONTINUOUS, { 0 } };
    HANDLE buzzer = CreateFileW(BUZZER_DEVICE, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    DWORD returned;
    BOOL played;

    if (buzzer == INVALID_HANDLE_VALUE)
        return FALSE;
    played = DeviceIoControl(buzzer, IOCTL_BZR_PLAY, &request, sizeof(request), NULL, 0, &returned, NULL);
    CloseHandle(buzzer);
    return played;
}
