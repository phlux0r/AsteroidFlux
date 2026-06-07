#ifndef BACKGROUND_STARS_H
#define BACKGROUND_STARS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "GameConfig.h"

class BackgroundStars {
private:
    struct StarData {
        float x;
        int y;
    };

    StarData _stars[GameConfig::MAX_STARS];

public:
    BackgroundStars() {
        // Distribute stars evenly across the initial screen buffer canvas width on boot
        for (int i = 0; i < GameConfig::MAX_STARS; i++) {
            _stars[i].x = random(0, GameConfig::SCREEN_WIDTH);
            _stars[i].y = random(GameConfig::UI_MARGIN_TOP + 2, GameConfig::SCREEN_HEIGHT - 2);
        }
    }

    void update() {
        for (int i = 0; i < GameConfig::MAX_STARS; i++) {
            // Apply slow parallax scroll physics vector
            _stars[i].x -= GameConfig::STAR_SCROLL_SPEED;

            // Recycle star to the right edge when it slips out of view
            if (_stars[i].x < 0) {
                _stars[i].x = GameConfig::SCREEN_WIDTH;
                _stars[i].y = random(GameConfig::UI_MARGIN_TOP + 2, GameConfig::SCREEN_HEIGHT - 2);
            }
        }
    }

    void render(GFXcanvas16 &canvas) {
        for (int i = 0; i < GameConfig::MAX_STARS; i++) {
            // Render basic 1-pixel stars
            canvas.drawPixel((int)_stars[i].x, _stars[i].y, ST7735_WHITE);
        }
    }
};

#endif