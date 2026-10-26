/**
 * @file
 * Sub LCD buffer and transfer through SLD1:.
 *
 * SLD1: is EDNA2_slcd_ST7586.dll. pwsh2-boot replaces Sharp's with one that
 * answers SLD_IOCTL_GET_INFO and draws 2-bit greyscale (SLD_IOCTL_DRAW_GREY).
 * Sharp's only draws 1-bit rectangles (code 8), so the buffer is reduced to
 * black and white for it. Either way only the changed rows are sent.
 *
 * The codes and structures match ce600/drivers/sublcd-a7300/sublcd_ioctl.h.
 */
#include "sublcd.h"
#include <string.h>

#define SLD_DEVICE L"SLD1:"
#define SLD_IOCTL_ON 1
#define SLD_IOCTL_CONTRAST 14
#define SLD_IOCTL_DRAW 8
#define SLD_IOCTL_WINDOW 12
#define SLD_IOCTL_GET_INFO 0x100
#define SLD_IOCTL_DRAW_GREY 0x101
#define SLD_INFO_MAGIC 0x32444C53
#define SLD_CAPS_GREY 0x00000001
#define MONO_COLUMN_GROUPS (SUBLCD_WIDTH / 8)

struct sld_draw {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    const uint8_t *bits;
    uint16_t flags;
    uint16_t reserved;
};

struct sld_grey_draw {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t pixels[SUBLCD_BUFFER_SIZE];
};

struct sld_info {
    DWORD magic;
    DWORD version;
    DWORD capabilities;
};

static HANDLE device = INVALID_HANDLE_VALUE;
static struct sld_grey_draw grey_draw;
static uint8_t shown[SUBLCD_BUFFER_SIZE];
static uint8_t mono[MONO_COLUMN_GROUPS * SUBLCD_HEIGHT];
static BOOL shown_valid;
static BOOL grey;

static BOOL control(DWORD code, void *input, DWORD input_size)
{
    DWORD returned;

    return DeviceIoControl(device, code, input, input_size, NULL, 0, &returned, NULL);
}

static BOOL driver_has_grey(void)
{
    struct sld_info info = { 0, 0, 0 };
    DWORD returned = 0;

    if (!DeviceIoControl(device, SLD_IOCTL_GET_INFO, NULL, 0, &info, sizeof(info), &returned, NULL))
        return FALSE;
    return returned >= sizeof(info) && info.magic == SLD_INFO_MAGIC && (info.capabilities & SLD_CAPS_GREY);
}

static BOOL row_changed(int y)
{
    return memcmp(&grey_draw.pixels[y * SUBLCD_STRIDE], &shown[y * SUBLCD_STRIDE], SUBLCD_STRIDE) != 0;
}

static void reduce_to_mono(int top, int bottom)
{
    int x, y;

    for (y = top; y <= bottom; y++)
        for (x = 0; x < SUBLCD_WIDTH; x++) {
            uint8_t *byte = &mono[(x / 8) * SUBLCD_HEIGHT + y];
            uint8_t mask = (uint8_t)(0x80 >> (x % 8));
            int level = (grey_draw.pixels[y * SUBLCD_STRIDE + x / 4] >> (6 - 2 * (x % 4))) & 3;

            if (level >= 2)
                *byte |= mask;
            else
                *byte &= (uint8_t)~mask;
        }
}

static BOOL send_rows(int top, int bottom)
{
    struct sld_draw draw = { 0, 0, SUBLCD_WIDTH, 0, mono, 0, 0 };

    if (grey) {
        grey_draw.x = 0;
        grey_draw.y = (uint16_t)top;
        grey_draw.width = SUBLCD_WIDTH;
        grey_draw.height = (uint16_t)(bottom - top + 1);
        return control(SLD_IOCTL_DRAW_GREY, &grey_draw, sizeof(grey_draw));
    }
    reduce_to_mono(top, bottom);
    draw.y = (uint16_t)top;
    draw.height = (uint16_t)(bottom - top + 1);
    return control(SLD_IOCTL_DRAW, &draw, sizeof(draw));
}

BOOL sublcd_open(void)
{
    device = CreateFileW(SLD_DEVICE, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (device == INVALID_HANDLE_VALUE)
        return FALSE;
    control(SLD_IOCTL_ON, NULL, 0);
    control(SLD_IOCTL_WINDOW, NULL, 0);
    grey = driver_has_grey();
    shown_valid = FALSE;
    return TRUE;
}

void sublcd_close(void)
{
    if (device == INVALID_HANDLE_VALUE)
        return;
    CloseHandle(device);
    device = INVALID_HANDLE_VALUE;
}

BOOL sublcd_update(void)
{
    int top = 0, bottom = SUBLCD_HEIGHT - 1;

    if (shown_valid) {
        while (top < SUBLCD_HEIGHT && !row_changed(top))
            top++;
        if (top == SUBLCD_HEIGHT)
            return TRUE;
        while (!row_changed(bottom))
            bottom--;
    }
    if (!send_rows(top, bottom))
        return FALSE;
    memcpy(shown, grey_draw.pixels, sizeof(shown));
    shown_valid = TRUE;
    return TRUE;
}

uint8_t *sublcd_buffer(void)
{
    return grey_draw.pixels;
}

BOOL sublcd_has_grey(void)
{
    return grey;
}

BOOL sublcd_set_contrast(int percent)
{
    uint8_t level = (uint8_t)(percent < 0 ? 0 : percent > 100 ? 100 : percent);

    return control(SLD_IOCTL_CONTRAST, &level, sizeof(level));
}
