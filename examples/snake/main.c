/**
 * @file
 * Snake on the sub LCD.
 *
 * The main-screen window takes the arrow keys, which only reach the focused
 * window. One loop waits for window messages, touch samples and the next game
 * tick. Quit on either screen ends it. The
 * buzzer beeps for food and at game over, and the sub backlight stays on.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backlight.h"
#include "button.h"
#include "buzzer.h"
#include "draw.h"
#include "touch.h"
#include "window.h"

#define TITLE L"Snake"
#define CELL_SIZE 6
#define STATUS_HEIGHT 12
#define COLUMNS (SUBLCD_WIDTH / CELL_SIZE)
#define ROWS ((SUBLCD_HEIGHT - STATUS_HEIGHT) / CELL_SIZE)
#define MAXIMUM_LENGTH (COLUMNS * ROWS)
#define START_LENGTH 4
#define TICK_MILLISECONDS 150
#define EAT_BEEP_MILLISECONDS 30
#define GAME_OVER_BEEP_MILLISECONDS 400

struct game {
    POINT body[MAXIMUM_LENGTH];
    int length;
    POINT direction;
    POINT next_direction;
    POINT food;
    int score;
    BOOL over;
};

static const struct sublcd_button quit_button = { SUBLCD_WIDTH - 40, 0, 40, STATUS_HEIGHT, "Quit" };

static struct game game;

static BOOL snake_covers(int x, int y, int length)
{
    int index;

    for (index = 0; index < length; index++)
        if (game.body[index].x == x && game.body[index].y == y)
            return TRUE;
    return FALSE;
}

static void place_food(void)
{
    do {
        game.food.x = rand() % COLUMNS;
        game.food.y = rand() % ROWS;
    } while (snake_covers(game.food.x, game.food.y, game.length));
}

static void new_game(void)
{
    int index;

    game.length = START_LENGTH;
    for (index = 0; index < game.length; index++) {
        game.body[index].x = COLUMNS / 2 - index;
        game.body[index].y = ROWS / 2;
    }
    game.direction.x = 1;
    game.direction.y = 0;
    game.next_direction = game.direction;
    game.score = 0;
    game.over = FALSE;
    place_food();
}

static void steer(int x, int y)
{
    if (x == -game.direction.x && y == -game.direction.y)
        return;
    game.next_direction.x = x;
    game.next_direction.y = y;
}

static void step(void)
{
    POINT head;
    BOOL eating;
    int kept;

    game.direction = game.next_direction;
    head.x = game.body[0].x + game.direction.x;
    head.y = game.body[0].y + game.direction.y;
    eating = head.x == game.food.x && head.y == game.food.y;
    kept = eating ? game.length : game.length - 1;
    if (head.x < 0 || head.x >= COLUMNS || head.y < 0 || head.y >= ROWS || snake_covers(head.x, head.y, kept)) {
        game.over = TRUE;
        sublcd_beep(GAME_OVER_BEEP_MILLISECONDS);
        return;
    }
    memmove(&game.body[1], &game.body[0], sizeof(POINT) * kept);
    game.body[0] = head;
    game.length = kept + 1;
    if (eating) {
        sublcd_beep(EAT_BEEP_MILLISECONDS);
        game.score++;
        if (game.length < MAXIMUM_LENGTH)
            place_food();
    }
}

static void draw_cell(POINT cell, BOOL filled)
{
    int x = cell.x * CELL_SIZE, y = STATUS_HEIGHT + cell.y * CELL_SIZE;

    if (filled)
        sublcd_fill_rect(x, y, CELL_SIZE - 1, CELL_SIZE - 1, SUBLCD_BLACK);
    else
        sublcd_rect(x, y, CELL_SIZE - 1, CELL_SIZE - 1, SUBLCD_BLACK);
}

static void draw_game_over(void)
{
    const char *title = "Game over", *hint = "Enter or tap to play again";
    int top = STATUS_HEIGHT + 24;

    sublcd_fill_rect(30, top, SUBLCD_WIDTH - 60, 44, SUBLCD_WHITE);
    sublcd_rect(30, top, SUBLCD_WIDTH - 60, 44, SUBLCD_BLACK);
    sublcd_text((SUBLCD_WIDTH - sublcd_text_width(title, 2)) / 2, top + 8, title, 2, SUBLCD_BLACK);
    sublcd_text((SUBLCD_WIDTH - sublcd_text_width(hint, 1)) / 2, top + 30, hint, 1, SUBLCD_BLACK);
}

static void draw(void)
{
    char score[24];
    int index;

    sublcd_clear();
    snprintf(score, sizeof(score), "Score %d", game.score);
    sublcd_text(2, (STATUS_HEIGHT - SUBLCD_FONT_HEIGHT) / 2, score, 1, SUBLCD_BLACK);
    sublcd_fill_rect(0, STATUS_HEIGHT - 1, SUBLCD_WIDTH, 1, SUBLCD_BLACK);
    sublcd_button_draw(&quit_button, FALSE);
    for (index = 0; index < game.length; index++)
        draw_cell(game.body[index], TRUE);
    draw_cell(game.food, FALSE);
    if (game.over)
        draw_game_over();
    sublcd_update();
}

static void handle_touch(void)
{
    struct sublcd_touch touch;

    while (sublcd_touch_read(&touch)) {
        if (touch.state != SUBLCD_TOUCH_RELEASE)
            continue;
        if (sublcd_button_contains(&quit_button, touch.x, touch.y))
            sublcd_window_quit();
        else if (game.over)
            new_game();
    }
}

static void handle_key(WPARAM key)
{
    switch (key) {
    case VK_UP:
        steer(0, -1);
        break;
    case VK_DOWN:
        steer(0, 1);
        break;
    case VK_LEFT:
        steer(-1, 0);
        break;
    case VK_RIGHT:
        steer(1, 0);
        break;
    case VK_RETURN:
        if (game.over)
            new_game();
        break;
    }
}

static void run(HANDLE touch_event)
{
    DWORD next_tick = GetTickCount();

    while (sublcd_window_pump()) {
        int remaining = (int)(next_tick - GetTickCount());
        DWORD woken;

        sublcd_backlight_keep_on();
        if (remaining <= 0) {
            if (!game.over)
                step();
            draw();
            next_tick += TICK_MILLISECONDS;
            continue;
        }
        woken = MsgWaitForMultipleObjectsEx(1, &touch_event, (DWORD)remaining, QS_ALLINPUT, 0);
        if (woken == WAIT_OBJECT_0)
            handle_touch();
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPWSTR command_line, int show)
{
    HANDLE touch_event;

    if (sublcd_window_close_running(TITLE))
        return 0;
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
    sublcd_window_open(instance, TITLE, L"Snake is on the sub screen. Arrow keys steer; Enter or a tap starts a new game.", handle_key);
    srand(GetTickCount());
    new_game();
    run(touch_event);
    sublcd_touch_close();
    sublcd_clear();
    sublcd_update();
    sublcd_close();
    return 0;
}
