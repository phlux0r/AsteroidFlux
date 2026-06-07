#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>

struct GameConfig {
    // Hardware Pins
    static const int PIN_TFT_CS     = 10;
    static const int PIN_TFT_DC     = 8;
    static const int PIN_TFT_RST    = 9;
    static const int PIN_BLK        = 7;  
    static const int PIN_DIAL       = 1;  
    static const int PIN_SPEAKER    = 5;

    // Screen Parameters
    static const int SCREEN_WIDTH   = 160;
    static const int SCREEN_HEIGHT  = 128;
    static const int UI_MARGIN_TOP  = 11;

    // Difficulty Balancing
    static const int MAX_ASTEROIDS           = 6;
    static const int SCORE_TO_SPAWN          = 10;
    static constexpr float BASE_SPEED        = 1.1f;
    static constexpr float SPEED_STEP        = 0.07f;
    static constexpr float COMET_SPEED_CAP   = 6.0f;
    static const int COMET_BONUS_SCORE       = 15;

    // Power-Up Parameters
    static const int POWERUP_START_SCORE     = 100;
    static const int MIN_SCORE_FOR_EXTRA_LIFE= 600;
    static const int SHIELD_DURATION_MS      = 10000;
    static const int EXTRA_LIFE_CHANCE       = 35; // Out of 100
    static const int POWERUP_SPAWN_LOW_MS    = 15000;
    static const int POWERUP_SPAWN_HIGH_MS   = 40000;
    static const int SLOW_SPEED_CHANCE       = 15; // Out of 100
    static const uint16_t COLOR_SHIELD       = 0x07E0; // Pure Green
    static const uint16_t COLOR_HEALTH       = ST7735_MAGENTA;
    static const uint16_t COLOR_SLOW         = ST7735_CYAN;
    static const int SPEED_STEPS_TO_REDUCE   = 3;

    // Background stars
    static const int MAX_STARS              = 16;
    static constexpr float STAR_SCROLL_SPEED = 0.25f; // Moves much slower than your BASE_SPEED (1.1f)

    // Asset Animations
    static const int SHIP_ANIM_SPEED_MS      = 80;
    static const int SHIP_WIDTH              = 16;
    static const int SHIP_HEIGHT             = 10;

    // UI Colors (RGB565)
    static const uint16_t COLOR_GREY        = 0x7BEF;
    static const uint16_t COLOR_ION_BLUE     = 0xCE7F;
};

#endif