/**
 * @file
 * Sub LCD touch through TPD2: (EDNA2_TouchSub.dll).
 *
 * The driver queues samples per handle once a queue is created, and signals
 * an event we hand it for each one. ReadFile returns whole 4-byte samples
 * without blocking: x in the low 16 bits and y in the high 16 bits, each
 * shifted left by 2, with the state in the low 2 bits.
 *
 * The driver often drops a touch's first sample, so the first sample after a
 * release is reported as a press whatever its state.
 */
#include "touch.h"
#include "sublcd.h"

#define TOUCH_DEVICE L"TPD2:"
#define IOCTL_TPD_CREATE_QUEUE 0x80002007
#define IOCTL_TPD_SET_MODE 0x8000200B
#define IOCTL_TPD_SET_EVENT 0x8000200F
#define QUEUE_LENGTH 256
#define READ_SAMPLES 32

static HANDLE device = INVALID_HANDLE_VALUE;
static HANDLE sample_event;
static DWORD samples[READ_SAMPLES];
static int sample_count;
static int next_sample;
static int last_x;
static int last_y;
static BOOL touching;

static int clamp(int value, int limit)
{
    return value < 0 ? 0 : value >= limit ? limit - 1 : value;
}

HANDLE sublcd_touch_open(void)
{
    DWORD returned;

    sample_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    device = CreateFileW(TOUCH_DEVICE, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (device == INVALID_HANDLE_VALUE
        || !DeviceIoControl(device, IOCTL_TPD_CREATE_QUEUE, NULL, 0, NULL, QUEUE_LENGTH, &returned, NULL)
        || !DeviceIoControl(device, IOCTL_TPD_SET_MODE, NULL, 1, NULL, 0, &returned, NULL)
        || !DeviceIoControl(device, IOCTL_TPD_SET_EVENT, &sample_event, sizeof(sample_event), NULL, 0, &returned, NULL)) {
        sublcd_touch_close();
        return NULL;
    }
    sample_count = 0;
    next_sample = 0;
    touching = FALSE;
    return sample_event;
}

void sublcd_touch_close(void)
{
    if (device != INVALID_HANDLE_VALUE)
        CloseHandle(device);
    if (sample_event)
        CloseHandle(sample_event);
    device = INVALID_HANDLE_VALUE;
    sample_event = NULL;
}

BOOL sublcd_touch_read(struct sublcd_touch *touch)
{
    DWORD sample, bytes_read;

    if (next_sample == sample_count) {
        if (!ReadFile(device, samples, sizeof(samples), &bytes_read, NULL) || bytes_read < sizeof(DWORD))
            return FALSE;
        sample_count = (int)(bytes_read / sizeof(DWORD));
        next_sample = 0;
    }
    sample = samples[next_sample++];
    touch->state = (enum sublcd_touch_state)(sample & 3);
    if (touch->state == SUBLCD_TOUCH_RELEASE) {
        touching = FALSE;
    } else {
        if (!touching)
            touch->state = SUBLCD_TOUCH_PRESS;
        touching = TRUE;
        last_x = clamp((int)(sample & 0xffff) >> 2, SUBLCD_WIDTH);
        last_y = clamp((int)(sample >> 16) >> 2, SUBLCD_HEIGHT);
    }
    touch->x = last_x;
    touch->y = last_y;
    return TRUE;
}
