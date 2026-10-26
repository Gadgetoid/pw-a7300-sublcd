/**
 * @file
 * Tamagotchi: TamaLIB's Tamagotchi P1 emulator on the sub LCD.
 *
 * The 32x16 screen is drawn at 6x with the eight icons above and below it.
 * Touching the left, middle or right third of the sub screen presses A, B or
 * C; so do Left, Enter and Right or the A, B and C keys on the main screen's
 * window. The ROM, tama.b, is loaded from the app's folder or
 * \Storage Card\Apps\SubLCD, and the pet is saved to tama.sav beside it on
 * quit. The emulator sleeps between bursts of about a frame, waking early for
 * input. GPL-2.0, as TamaLIB is.
 */
#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "backlight.h"
#include "buzzer.h"
#include "draw.h"
#include "touch.h"
#include "window.h"
#include "tamalib/tamalib.h"

#define TITLE L"Tamagotchi"
#define ROM_NAME L"tama.b"
#define SAVE_NAME L"tama.sav"
#define SAVE_TEMPORARY_NAME L"tama.tmp"
#define FALLBACK_FOLDER L"\\Storage Card\\Apps\\SubLCD\\"
#define ROM_WORDS 6144
#define OPCODE_MASK 0x0FFF

#define TIMESTAMP_FREQUENCY (1 << 20)
#define MAXIMUM_TIMESTAMP_STEP_MS 60000
#define RUN_AHEAD_TICKS (TIMESTAMP_FREQUENCY / 30)
#define RESYNC_TICKS TIMESTAMP_FREQUENCY
#define BACKLIGHT_HOLD_MS 30000
#define FALLBACK_BEEP_MS 40

#define DOT_PITCH 6
#define DOT_SIZE 5
#define MATRIX_LEFT 24
#define MATRIX_TOP 12
#define DOT_ON_COLOUR SUBLCD_BLACK
#define DOT_OFF_COLOUR SUBLCD_LIGHT
#define ICON_SIZE 10
#define ICONS_PER_ROW (ICON_NUM / 2)
#define ICON_TOP_ROW_Y 1
#define ICON_BOTTOM_ROW_Y (SUBLCD_HEIGHT - ICON_SIZE - 1)
#define ICON_ON_COLOUR SUBLCD_BLACK
#define ICON_OFF_COLOUR SUBLCD_LIGHT

#define BUTTON_COUNT 3
#define NO_BUTTON -1

#define SAVE_MAGIC 0x414D4154
#define SAVE_VERSION 1
#define SAVE_SCALARS 23
#define SAVE_SIZE (4 * (2 + SAVE_SCALARS + 3 * INT_SLOT_NUM) + MEM_BUFFER_SIZE)

static const char *const icons[ICON_NUM][ICON_SIZE] = {
    {
        ".#.#...#..",
        ".#.#..##..",
        ".#.#..##..",
        ".###..##..",
        "..#...##..",
        "..#....#..",
        "..#....#..",
        "..#....#..",
        "..#....#..",
        "..#....#..",
    },
    {
        "...####...",
        "..#....#..",
        ".#......#.",
        ".#......#.",
        ".#......#.",
        "..#....#..",
        "...#..#...",
        "...####...",
        "...####...",
        "....##....",
    },
    {
        "..........",
        "...####...",
        "..#.##.#..",
        ".#..##..#.",
        ".########.",
        ".#..##..#.",
        "..#.##.#..",
        "...####...",
        "..........",
        "..........",
    },
    {
        "..........",
        "...####...",
        "...#..#...",
        ".###..###.",
        ".#......#.",
        ".#......#.",
        ".###..###.",
        "...#..#...",
        "...####...",
        "..........",
    },
    {
        ".##.......",
        ".##.......",
        ".##.......",
        ".#######..",
        ".########.",
        "..######..",
        "...####...",
        "...###....",
        "...###....",
        "..#####...",
    },
    {
        "..........",
        ".......##.",
        ".......##.",
        "....##.##.",
        "....##.##.",
        ".##.##.##.",
        ".##.##.##.",
        ".##.##.##.",
        ".##.##.##.",
        "##########",
    },
    {
        "..######..",
        ".#......#.",
        "#.#....#.#",
        "#..#..#..#",
        "#.##..##.#",
        "#........#",
        "#..####..#",
        "#.#....#.#",
        ".#......#.",
        "..######..",
    },
    {
        ".########.",
        "#........#",
        "#...##...#",
        "#...##...#",
        "#...##...#",
        "#........#",
        "#...##...#",
        ".########.",
        "..##......",
        "..#.......",
    },
};

static u12_t program[ROM_WORDS];
static WCHAR save_path[MAX_PATH];
static WCHAR save_temporary_path[MAX_PATH];

static HANDLE touch_event;
static BOOL quitting;
static BOOL input_pending;
static BOOL resync_needed;
static DWORD last_input_ms;

static timestamp_t timestamp;
static DWORD timestamp_last_ms;
static DWORD timestamp_remainder;
static timestamp_t last_deadline;

static bool_t matrix[LCD_HEIGHT][LCD_WIDTH];
static bool_t icon_states[ICON_NUM];
static BOOL screen_dirty;

static u32_t buzzer_frequency;
static BOOL buzzer_playing;
static BOOL tone_supported = TRUE;

static BOOL touch_holds[BUTTON_COUNT];
static BOOL key_holds[BUTTON_COUNT];
static int touch_button = NO_BUTTON;

static void *allocate(u32_t size)
{
    return malloc(size);
}

static void release(void *pointer)
{
    free(pointer);
}

static void halt(void)
{
}

static bool_t is_log_enabled(log_level_t level)
{
    return 0;
}

static void log_message(log_level_t level, char *format, ...)
{
}

static timestamp_t get_timestamp(void)
{
    DWORD now = GetTickCount();
    DWORD elapsed = now - timestamp_last_ms;
    DWORD fraction;

    timestamp_last_ms = now;
    if (elapsed > MAXIMUM_TIMESTAMP_STEP_MS)
        elapsed = MAXIMUM_TIMESTAMP_STEP_MS;
    fraction = elapsed * (TIMESTAMP_FREQUENCY % 1000) + timestamp_remainder;
    timestamp += elapsed * (TIMESTAMP_FREQUENCY / 1000) + fraction / 1000;
    timestamp_remainder = fraction % 1000;
    return timestamp;
}

static DWORD ticks_to_ms(uint32_t ticks)
{
    return ((ticks >> 10) * 1000) >> 10;
}

static void sleep_until(timestamp_t deadline)
{
    int32_t ahead = (int32_t)(deadline - get_timestamp());

    last_deadline = deadline;
    if (ahead < -RESYNC_TICKS)
        resync_needed = TRUE;
    if (ahead <= RUN_AHEAD_TICKS)
        return;
    if (MsgWaitForMultipleObjectsEx(1, &touch_event, ticks_to_ms((uint32_t)ahead), QS_ALLINPUT, 0) != WAIT_TIMEOUT)
        input_pending = TRUE;
}

static void catch_up_with_emulation(void)
{
    int32_t ahead = (int32_t)(last_deadline - get_timestamp());

    if (ahead > 0 && ahead <= 2 * RUN_AHEAD_TICKS)
        Sleep(ticks_to_ms((uint32_t)ahead));
}

static void draw_icon(int icon)
{
    int column = icon % ICONS_PER_ROW;
    int left = SUBLCD_WIDTH * (2 * column + 1) / (2 * ICONS_PER_ROW) - ICON_SIZE / 2;
    int top = icon < ICONS_PER_ROW ? ICON_TOP_ROW_Y : ICON_BOTTOM_ROW_Y;
    int colour = icon_states[icon] ? ICON_ON_COLOUR : ICON_OFF_COLOUR;
    int x, y;

    for (y = 0; y < ICON_SIZE; y++)
        for (x = 0; x < ICON_SIZE; x++)
            sublcd_pixel(left + x, top + y, icons[icon][y][x] == '#' ? colour : SUBLCD_WHITE);
}

static void draw_screen(void)
{
    int x, y, icon;

    for (y = 0; y < LCD_HEIGHT; y++)
        for (x = 0; x < LCD_WIDTH; x++)
            sublcd_fill_rect(MATRIX_LEFT + x * DOT_PITCH, MATRIX_TOP + y * DOT_PITCH, DOT_SIZE, DOT_SIZE,
                matrix[y][x] ? DOT_ON_COLOUR : DOT_OFF_COLOUR);
    for (icon = 0; icon < ICON_NUM; icon++)
        draw_icon(icon);
    sublcd_update();
}

static void set_lcd_matrix(u8_t x, u8_t y, bool_t value)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || matrix[y][x] == value)
        return;
    matrix[y][x] = value;
    screen_dirty = TRUE;
}

static void set_lcd_icon(u8_t icon, bool_t value)
{
    if (icon >= ICON_NUM || icon_states[icon] == value)
        return;
    icon_states[icon] = value;
    screen_dirty = TRUE;
}

static void set_frequency(u32_t frequency)
{
    buzzer_frequency = frequency;
    if (buzzer_playing && tone_supported) {
        catch_up_with_emulation();
        sublcd_tone(buzzer_frequency / 10);
    }
}

static void play_frequency(bool_t enabled)
{
    if ((BOOL)enabled == buzzer_playing)
        return;
    buzzer_playing = enabled;
    catch_up_with_emulation();
    if (tone_supported) {
        if (sublcd_tone(enabled ? buzzer_frequency / 10 : 0))
            return;
        tone_supported = FALSE;
    }
    if (enabled)
        sublcd_beep(FALLBACK_BEEP_MS);
}

static void apply_button(int button)
{
    BOOL pressed = touch_holds[button] || key_holds[button];

    tamalib_set_button((button_t)button, pressed ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
}

static int button_for_key(WPARAM key)
{
    switch (key) {
    case VK_LEFT:
    case 'A':
        return BTN_LEFT;
    case VK_RETURN:
    case 'B':
        return BTN_MIDDLE;
    case VK_RIGHT:
    case 'C':
        return BTN_RIGHT;
    }
    return NO_BUTTON;
}

static void handle_key(WPARAM key, BOOL pressed)
{
    int button = button_for_key(key);

    last_input_ms = GetTickCount();
    if (key == VK_ESCAPE && pressed) {
        sublcd_window_quit();
        return;
    }
    if (button == NO_BUTTON)
        return;
    key_holds[button] = pressed;
    apply_button(button);
}

static void pump_messages(void)
{
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            quitting = TRUE;
            return;
        }
        if (message.message == WM_KEYDOWN)
            handle_key(message.wParam, TRUE);
        else if (message.message == WM_KEYUP)
            handle_key(message.wParam, FALSE);
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

static void handle_touch(void)
{
    struct sublcd_touch touch;

    while (sublcd_touch_read(&touch)) {
        last_input_ms = GetTickCount();
        if (touch.state == SUBLCD_TOUCH_PRESS && touch_button == NO_BUTTON) {
            touch_button = touch.x * BUTTON_COUNT / SUBLCD_WIDTH;
            if (touch_button >= BUTTON_COUNT)
                touch_button = BUTTON_COUNT - 1;
            if (touch_button < 0)
                touch_button = 0;
            touch_holds[touch_button] = TRUE;
            apply_button(touch_button);
        } else if (touch.state == SUBLCD_TOUCH_RELEASE && touch_button != NO_BUTTON) {
            touch_holds[touch_button] = FALSE;
            apply_button(touch_button);
            touch_button = NO_BUTTON;
        }
    }
}

static void service_input(void)
{
    input_pending = FALSE;
    handle_touch();
    pump_messages();
}

static void update_screen(void)
{
    service_input();
    if (GetTickCount() - last_input_ms < BACKLIGHT_HOLD_MS)
        sublcd_backlight_keep_on();
    if (!screen_dirty)
        return;
    screen_dirty = FALSE;
    draw_screen();
}

static int handler(void)
{
    if (input_pending)
        service_input();
    if (resync_needed) {
        resync_needed = FALSE;
        tamalib_set_exec_mode(EXEC_MODE_RUN);
    }
    return quitting;
}

static hal_t hal = {
    .malloc = allocate,
    .free = release,
    .halt = halt,
    .is_log_enabled = is_log_enabled,
    .log = log_message,
    .sleep_until = sleep_until,
    .get_timestamp = get_timestamp,
    .update_screen = update_screen,
    .set_lcd_matrix = set_lcd_matrix,
    .set_lcd_icon = set_lcd_icon,
    .set_frequency = set_frequency,
    .play_frequency = play_frequency,
    .handler = handler,
};

static BOOL read_whole_file(const WCHAR *path, void *buffer, DWORD size)
{
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD read = 0;
    BOOL complete;

    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    complete = GetFileSize(file, NULL) == size && ReadFile(file, buffer, size, &read, NULL) && read == size;
    CloseHandle(file);
    return complete;
}

static BOOL write_whole_file(const WCHAR *path, const void *buffer, DWORD size)
{
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    BOOL complete;

    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    complete = WriteFile(file, buffer, size, &written, NULL) && written == size;
    CloseHandle(file);
    return complete;
}

static BOOL load_rom_from(const WCHAR *folder, DWORD folder_length)
{
    WCHAR rom_path[MAX_PATH];
    int word;

    if (folder_length + wcslen(SAVE_TEMPORARY_NAME) >= MAX_PATH)
        return FALSE;
    memcpy(rom_path, folder, folder_length * sizeof(WCHAR));
    wcscpy(rom_path + folder_length, ROM_NAME);
    if (!read_whole_file(rom_path, program, sizeof(program)))
        return FALSE;
    for (word = 0; word < ROM_WORDS; word++)
        program[word] &= OPCODE_MASK;
    memcpy(save_path, folder, folder_length * sizeof(WCHAR));
    wcscpy(save_path + folder_length, SAVE_NAME);
    memcpy(save_temporary_path, folder, folder_length * sizeof(WCHAR));
    wcscpy(save_temporary_path + folder_length, SAVE_TEMPORARY_NAME);
    return TRUE;
}

static BOOL load_rom(void)
{
    WCHAR module_path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, module_path, MAX_PATH);
    WCHAR *last_separator;

    if (length > 0 && length < MAX_PATH) {
        last_separator = wcsrchr(module_path, L'\\');
        if (last_separator && load_rom_from(module_path, (DWORD)(last_separator - module_path + 1)))
            return TRUE;
    }
    return load_rom_from(FALLBACK_FOLDER, wcslen(FALLBACK_FOLDER));
}

static void put_value(uint8_t **cursor, uint32_t value)
{
    int byte;

    for (byte = 0; byte < 4; byte++)
        *(*cursor)++ = (uint8_t)(value >> (8 * byte));
}

static uint32_t get_value(const uint8_t **cursor)
{
    uint32_t value = 0;
    int byte;

    for (byte = 0; byte < 4; byte++)
        value |= (uint32_t)*(*cursor)++ << (8 * byte);
    return value;
}

static void save_state(void)
{
    static uint8_t data[SAVE_SIZE];
    state_t *state = tamalib_get_state();
    uint8_t *cursor = data;
    int slot;

    put_value(&cursor, SAVE_MAGIC);
    put_value(&cursor, SAVE_VERSION);
    put_value(&cursor, *state->pc);
    put_value(&cursor, *state->x);
    put_value(&cursor, *state->y);
    put_value(&cursor, *state->a);
    put_value(&cursor, *state->b);
    put_value(&cursor, *state->np);
    put_value(&cursor, *state->sp);
    put_value(&cursor, *state->flags);
    put_value(&cursor, *state->tick_counter);
    put_value(&cursor, *state->clk_timer_2hz_timestamp);
    put_value(&cursor, *state->clk_timer_4hz_timestamp);
    put_value(&cursor, *state->clk_timer_8hz_timestamp);
    put_value(&cursor, *state->clk_timer_16hz_timestamp);
    put_value(&cursor, *state->clk_timer_32hz_timestamp);
    put_value(&cursor, *state->clk_timer_64hz_timestamp);
    put_value(&cursor, *state->clk_timer_128hz_timestamp);
    put_value(&cursor, *state->clk_timer_256hz_timestamp);
    put_value(&cursor, *state->prog_timer_timestamp);
    put_value(&cursor, *state->prog_timer_enabled);
    put_value(&cursor, *state->prog_timer_data);
    put_value(&cursor, *state->prog_timer_rld);
    put_value(&cursor, *state->call_depth);
    put_value(&cursor, *state->cpu_halted);
    for (slot = 0; slot < INT_SLOT_NUM; slot++) {
        put_value(&cursor, state->interrupts[slot].factor_flag_reg);
        put_value(&cursor, state->interrupts[slot].mask_reg);
        put_value(&cursor, state->interrupts[slot].triggered);
    }
    memcpy(cursor, state->memory, MEM_BUFFER_SIZE);
    if (!write_whole_file(save_temporary_path, data, SAVE_SIZE))
        return;
    DeleteFileW(save_path);
    MoveFileW(save_temporary_path, save_path);
}

static void load_state(void)
{
    static uint8_t data[SAVE_SIZE];
    state_t *state = tamalib_get_state();
    const uint8_t *cursor = data;
    int slot;

    if (!read_whole_file(save_path, data, SAVE_SIZE))
        return;
    if (get_value(&cursor) != SAVE_MAGIC || get_value(&cursor) != SAVE_VERSION)
        return;
    *state->pc = get_value(&cursor);
    *state->x = get_value(&cursor);
    *state->y = get_value(&cursor);
    *state->a = get_value(&cursor);
    *state->b = get_value(&cursor);
    *state->np = get_value(&cursor);
    *state->sp = get_value(&cursor);
    *state->flags = get_value(&cursor);
    *state->tick_counter = get_value(&cursor);
    *state->clk_timer_2hz_timestamp = get_value(&cursor);
    *state->clk_timer_4hz_timestamp = get_value(&cursor);
    *state->clk_timer_8hz_timestamp = get_value(&cursor);
    *state->clk_timer_16hz_timestamp = get_value(&cursor);
    *state->clk_timer_32hz_timestamp = get_value(&cursor);
    *state->clk_timer_64hz_timestamp = get_value(&cursor);
    *state->clk_timer_128hz_timestamp = get_value(&cursor);
    *state->clk_timer_256hz_timestamp = get_value(&cursor);
    *state->prog_timer_timestamp = get_value(&cursor);
    *state->prog_timer_enabled = get_value(&cursor);
    *state->prog_timer_data = get_value(&cursor);
    *state->prog_timer_rld = get_value(&cursor);
    *state->call_depth = get_value(&cursor);
    *state->cpu_halted = get_value(&cursor);
    for (slot = 0; slot < INT_SLOT_NUM; slot++) {
        state->interrupts[slot].factor_flag_reg = get_value(&cursor);
        state->interrupts[slot].mask_reg = get_value(&cursor);
        state->interrupts[slot].triggered = get_value(&cursor);
    }
    memcpy(state->memory, cursor, MEM_BUFFER_SIZE);
    tamalib_refresh_hw();
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    if (sublcd_window_close_running(TITLE))
        return 0;
    if (!load_rom()) {
        MessageBoxW(NULL,
            L"The Tamagotchi P1 ROM was not found. Copy tama.b (12288 bytes) to \\Storage Card\\Apps\\SubLCD\\ or next to this app.",
            TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!sublcd_open()) {
        MessageBoxW(NULL, L"The sub LCD (SLD1:) is not available.", TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    touch_event = sublcd_touch_open();
    if (!touch_event) {
        MessageBoxW(NULL, L"The sub touch panel (TPD2:) is not available.", TITLE, MB_OK | MB_ICONERROR);
        sublcd_close();
        return 1;
    }
    sublcd_window_open(instance, TITLE,
        L"Tamagotchi is on the sub screen. Tap its left, middle or right third for A, B or C, or use Left, Enter and "
        L"Right. Escape or Quit saves and exits.",
        NULL);
    timestamp_last_ms = GetTickCount();
    last_input_ms = timestamp_last_ms;
    sublcd_clear();
    screen_dirty = TRUE;
    tamalib_register_hal(&hal);
    tamalib_init(program, NULL, TIMESTAMP_FREQUENCY);
    load_state();
    tamalib_mainloop();
    save_state();
    if (buzzer_playing && tone_supported)
        sublcd_tone(0);
    tamalib_release();
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
