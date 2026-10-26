/**
 * @file
 * Beeps through BZR1: (EDNA2_Buzzer.dll).
 *
 * BZR_IOControl code 0x8000200F queues a sound for the driver's buzzer thread
 * in the same form as its registry events: mode 1 plays alarm pattern
 * `alarm` for `milliseconds`. Pattern 1 has no length limit of its own.
 *
 * Tones go to pwsh2-boot's SLD1: driver (code 0x104), which drives the
 * buzzer's PWM directly. Sharp's SLD1: returns TRUE for codes it does not
 * know, so SLD_IOCTL_GET_INFO's capabilities say whether tones work.
 */
#include "buzzer.h"

#define BUZZER_DEVICE L"BZR1:"
#define IOCTL_BZR_PLAY 0x8000200F
#define BUZZER_MODE_ALARM 1
#define BUZZER_ALARM_CONTINUOUS 1

#define SLD_DEVICE L"SLD1:"
#define SLD_IOCTL_GET_INFO 0x100
#define SLD_IOCTL_SET_TONE 0x104
#define SLD_INFO_MAGIC 0x32444C53
#define SLD_CAPS_TONE 0x00000004

struct sld_info {
    DWORD magic;
    DWORD version;
    DWORD capabilities;
};

static HANDLE tone_device = INVALID_HANDLE_VALUE;
static BOOL tone_checked;

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

static BOOL open_tone_device(void)
{
    struct sld_info info = { 0, 0, 0 };
    DWORD returned = 0;

    if (tone_checked)
        return tone_device != INVALID_HANDLE_VALUE;
    tone_checked = TRUE;
    tone_device = CreateFileW(SLD_DEVICE, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (tone_device == INVALID_HANDLE_VALUE)
        return FALSE;
    if (DeviceIoControl(tone_device, SLD_IOCTL_GET_INFO, NULL, 0, &info, sizeof(info), &returned, NULL)
        && returned >= sizeof(info) && info.magic == SLD_INFO_MAGIC && (info.capabilities & SLD_CAPS_TONE))
        return TRUE;
    CloseHandle(tone_device);
    tone_device = INVALID_HANDLE_VALUE;
    return FALSE;
}

BOOL sublcd_tone(DWORD frequency)
{
    DWORD returned;

    if (!open_tone_device())
        return FALSE;
    return DeviceIoControl(tone_device, SLD_IOCTL_SET_TONE, &frequency, sizeof(frequency), NULL, 0, &returned, NULL);
}
