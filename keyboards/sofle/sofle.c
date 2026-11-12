// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#include "quantum.h"

#ifdef SWAP_HANDS_ENABLE

__attribute__ ((weak))
const keypos_t PROGMEM hand_swap_config[MATRIX_ROWS][MATRIX_COLS] =
    // The LAYOUT macro could work for this, but it was harder to figure out the
    // identity using it.

    // This is the identity layout.
/*
{
    { {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0} },
    { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1} },
    { {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2} },
    { {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3} },
    { {0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4} },

    { {0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5} },
    { {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6} },
    { {0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7} },
    { {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8} },
    { {0, 9}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {5, 9} },
};
*/

    // This is the mirror, q <-> p, w <-> o, etc...
{
 { {0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5} },
 { {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6} },
 { {0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7} },
 { {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8} },
 { {0, 9}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {5, 9} },

 { {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0} },
 { {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1} },
 { {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2} },
 { {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3} },
 { {0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4} },
};

#    ifdef ENCODER_MAP_ENABLE
const uint8_t PROGMEM encoder_hand_swap_config[NUM_ENCODERS] = {1, 0};
#    endif

#endif

#ifdef ENCODER_ENABLE
bool encoder_update_kb(uint8_t index, bool clockwise) {
    if (!encoder_update_user(index, clockwise)) {
        return false;
    }

    if (index == 1) {
        tap_code(clockwise ? KC_VOLD : KC_VOLU);
    }

    return true;
}
#endif

#ifdef OLED_ENABLE
oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    if (is_keyboard_master()) {
        return OLED_ROTATION_270;
    }
    if (!is_keyboard_master()) {
        return OLED_ROTATION_270;
    }
    return rotation;
}

/* QMK logo
static void render_logo(void) {

    static const char PROGMEM qmk_logo[] = {
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0
    };
    oled_write_P(qmk_logo, false);

}
*/

static void draw_speedometer_pixel(uint8_t x, uint8_t y, bool on) {
    oled_write_pixel(x, y, on);
    oled_write_pixel(x, y + 1, on);
}

static void draw_speedometer_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool on) {
    int16_t dx  = x1 > x0 ? x1 - x0 : x0 - x1;
    int16_t sx  = x0 < x1 ? 1 : -1;
    int16_t dy  = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int16_t sy  = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;

    while (true) {
        draw_speedometer_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        int16_t twice_err = 2 * err;
        if (twice_err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (twice_err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static uint8_t speedometer_arc_height(uint8_t x) {
    if (x <= 15) {
        int16_t dx = 15 - x;
        return 14 * (196 - dx * dx) / 196;
    }

    uint8_t dx = x - 15;
    return dx >= 7 ? 13 : 14 - (dx * dx) / 49;
}

static void draw_wpm_speedometer(uint8_t wpm) {
    enum {
        CENTER_X = 15,
        CENTER_Y = 100,
        MAX_WPM  = 120,
    };
    static bool    initialized;
    static uint8_t previous_tip_x;
    static uint8_t previous_tip_y;

    if (wpm > MAX_WPM) {
        wpm = MAX_WPM;
    }

    uint8_t tip_x = 1 + (wpm * 28) / MAX_WPM;
    uint8_t tip_y = CENTER_Y - speedometer_arc_height(tip_x);

    if (initialized) {
        draw_speedometer_line(CENTER_X, CENTER_Y, previous_tip_x, previous_tip_y, false);
    }

    for (uint8_t x = 1; x <= 29; x++) {
        draw_speedometer_pixel(x, CENTER_Y - speedometer_arc_height(x), true);
    }

    static const uint8_t tick_x[]  = {1, 8, 15, 22, 29};
    static const int8_t  tick_dx[] = {-1, -1, 0, 1, 1};
    static const int8_t  tick_dy[] = {-1, -2, -3, -2, -1};
    for (uint8_t i = 0; i < ARRAY_SIZE(tick_x); i++) {
        uint8_t x = tick_x[i];
        uint8_t y = CENTER_Y - speedometer_arc_height(x);
        draw_speedometer_line(x, y, x + tick_dx[i], y + tick_dy[i], true);
    }

    draw_speedometer_line(CENTER_X, CENTER_Y, tip_x, tip_y, true);
    oled_write_pixel(CENTER_X, CENTER_Y, true);
    oled_write_pixel(CENTER_X - 1, CENTER_Y, true);
    oled_write_pixel(CENTER_X + 1, CENTER_Y, true);
    oled_write_pixel(CENTER_X, CENTER_Y - 1, true);

    previous_tip_x = tip_x;
    previous_tip_y = tip_y;
    initialized    = true;
}

static void print_slave_status_narrow(void) {
    // LINE COST: this panel is 5 chars wide, and a word that exactly
    // fills a line eats an extra blank line under oled_write_ln_P() --
    // the cursor wraps to the next line, and the driver's page-advance
    // then clears it as the "rest of the current" line. So every name
    // below is either 5 chars or ends in an explicit '\n'. Break this
    // and the screen comes up one line short: text below shifts up and
    // the last line stays stale (the framebuffer is only cleared at init).
    oled_write_ln_P(PSTR("LYOUT"), false);
    switch (get_highest_layer(layer_state|default_layer_state)) {
        case 0:
            oled_write_ln_P(PSTR("Qwrty"), false);
            break;
        case 1:
            oled_write_ln_P(PSTR("Clmk\n"), false);
            break;
        case 2:
            oled_write_ln_P(PSTR("Game\n"), false);
            break;
        default:
            oled_write_ln_P(PSTR("Mod\n"), false);
            break;
    }
    oled_write_ln_P(PSTR("LAYER"), false);
    switch (get_highest_layer(layer_state)) {
        case 0:
        case 1:
            oled_write_ln_P(PSTR("Base\n"), false);
            break;
        case 2:
            // Unreachable today (Game is a PDF default layer, never a
            // transient one), but keep the 2-line cost like the others.
            oled_write_ln_P(PSTR("Base\n"), false);
            break;
        case 3:
            oled_write_ln_P(PSTR("Symbl"), false);
            break;
        case 4:
            // 3-char word, so this '\n' is load-bearing (see LINE COST)
            oled_write_ln_P(PSTR("Mod\n"), false);
            break;
        case 5:
            oled_write_ln_P(PSTR("Adjst"), false);
            break;
        case 6:
            oled_write_ln_P(PSTR("Mouse"), false);
            break;
        default:
            oled_write_ln_P(PSTR("Undef"), false);
    }
    oled_write_ln_P(PSTR("\n"), false);

    uint8_t wpm = get_current_wpm();
    char    wpm_str[4];
    if (wpm > 120) {
        wpm = 120;
    }
    wpm_str[0] = '0' + wpm / 100;
    wpm_str[1] = '0' + (wpm / 10) % 10;
    wpm_str[2] = '0' + wpm % 10;
    wpm_str[3] = '\0';

    draw_wpm_speedometer(wpm);
    oled_set_cursor(1, 13);
    oled_write(wpm_str, false);
}

void print_status_narrow(void) {
    //oled_write_P(PSTR("\n\n"), false);

    oled_write_P(PSTR("\n\n"), false);
    led_t led_usb_state = host_keyboard_led_state();
    oled_write_ln_P(PSTR("CPSLK"), led_usb_state.caps_lock);
    oled_write_ln_P(PSTR("CRRCT"), autocorrect_is_enabled());
    oled_write_ln_P(PSTR("SHIFT"), get_autoshift_state());
    oled_write_ln_P(PSTR("MAC"), keymap_config.swap_lctl_lgui);
}

// This function is called for every OLED update, and decides which display to render.
bool oled_task_kb(void) {
    //if (!oled_task_user()) {
    //    return false;
    //}
    if (is_keyboard_master()) {
        print_status_narrow();
    } else {
        //render_logo();
        print_slave_status_narrow();
    }
    return true;
}
#endif
