#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro2d.h>

#include <math.h>

#define TOP_SCREEN_WIDTH        400
#define TOP_SCREEN_HEIGHT       240
#define TOP_SCREEN_CENTER_HOR   200
#define TOP_SCREEN_CENTER_VER   120

#define HITLINE_LEFT_MARGIN     50
#define LANE_TOP_MARGIN         60
#define LANE_BOTTOM_MARGIN      60
#define LANE_HEIGHT             40
#define NOTE_MARGIN             5
#define NOTE_RADIUS             (LANE_HEIGHT / 2 - NOTE_MARGIN)

#define DEBUG_DEPTH             1

#define DPAD_ANY                (KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN)

#define C2D_WHITE               C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define C2D_RED                 C2D_Color32(0xFF, 0x00, 0x00, 0xFF)
#define C2D_GREEN               C2D_Color32(0x00, 0xFF, 0x00, 0xFF)
#define C2D_BLUE                C2D_Color32(0x00, 0x00, 0xFF, 0xFF)
#define C2D_BLACK               C2D_Color32(0x00, 0x00, 0x00, 0xFF)
#define C2D_TRANSPARENT         C2D_Color32(0x00, 0x00, 0x00, 0x00)
#define C2D_PURPLE              C2D_Color32(0x80, 0x00, 0x80, 0xFF)
#define C2D_ORANGE              C2D_Color32(0xFF, 0xA5, 0x00, 0xFF)
#define C2D_ORANGERED           C2D_Color32(0xFF, 0x45, 0x00, 0xFF)

inline unsigned long saturated_sub_lu(unsigned long lhs, unsigned long rhs) {
    if (rhs > lhs) {
        return 0;
    } else {
        return lhs - rhs;
    }
}

#endif
