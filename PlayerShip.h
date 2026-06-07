#ifndef PLAYER_SHIP_H
#define PLAYER_SHIP_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "GameConfig.h"

// --- 16x11 SIDE-VIEW SHIP BITMAP (16-bit RGB565 Colors) ---
#define W ST7735_WHITE
#define B ST7735_BLACK
#define C ST7735_CYAN
#define L ST7735_BLUE
#define O ST7735_ORANGE
#define R ST7735_RED
#define M ST7735_MAGENTA

class PlayerShip {
private:
    float _x;
    int _y;
    float _dialFiltered;
    int _currentFrame;
    unsigned long _nextFrameTime;
    bool _shieldActive;
    unsigned long _shieldEndTime;

    // --- EMBED THE PROGMEM ANIMATION BITMAP DATA HERE ---
    // This is the 3-frame animation data pulled from your original code
    const uint16_t _frames[3][160] = {
        {
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            O,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            O,O,R,L,L,L,L,M,M,M,W,W,W,W,W,B,
            O,O,R,L,L,L,L,M,M,M,W,W,W,W,W,B,
            O,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B
        },
        {
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            B,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            R,O,O,L,L,L,L,M,M,M,W,W,W,W,W,B,
            R,O,O,L,L,L,L,M,M,M,W,W,W,W,W,B,
            B,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B
        },
        {
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            R,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            O,R,B,L,L,L,L,M,M,M,W,W,W,W,W,B,
            O,R,B,L,L,L,L,M,M,M,W,W,W,W,W,B,
            R,B,B,W,W,W,W,W,W,W,W,W,W,B,B,B,
            B,B,B,B,W,W,C,C,L,L,L,W,B,B,B,B,
            B,B,B,B,B,B,W,W,L,L,W,B,B,B,B,B,
            B,B,B,B,B,B,B,B,W,W,B,B,B,B,B,B
        }
    };

public:
    PlayerShip() : _x(15.0f), _y(64), _dialFiltered(2048.0f), _currentFrame(0), 
                   _nextFrameTime(0), _shieldActive(false), _shieldEndTime(0) {}

    void reset() {
        _y = GameConfig::SCREEN_HEIGHT / 2;
        _dialFiltered = 2048.0f;
        _shieldActive = false;
    }

    void updatePosition() {
        int rawDial = analogRead(GameConfig::PIN_DIAL);
        _dialFiltered = (_dialFiltered * 0.80f) + (rawDial * 0.20f); 
        _y = map((int)_dialFiltered, 0, 4095, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_HEIGHT - GameConfig::SHIP_HEIGHT - 1);
    }

    void updateAnimation() {
        if (millis() >= _nextFrameTime) {
            _currentFrame = (_currentFrame + 1) % 3;
            _nextFrameTime = millis() + GameConfig::SHIP_ANIM_SPEED_MS;
        }
    }

    void activateShield() {
        _shieldActive = true;
        _shieldEndTime = millis() + GameConfig::SHIELD_DURATION_MS;
    }

    void updateShield() {
        if (_shieldActive && millis() >= _shieldEndTime) {
            _shieldActive = false;
        }
    }

    float getX() const { return _x; }
    int getY() const { return _y; }
    bool isShieldActive() const { return _shieldActive; }
    void deactivateShield() { _shieldActive = false; }

    // --- RENDER DIRECTLY FROM THE INTERNAL ARRAY ---
    void render(GFXcanvas16 &canvas) {
        // Draw Shield Bubble if active
        if (_shieldActive) {
            int cx = (int)_x + (GameConfig::SHIP_WIDTH / 2);
            int cy = _y + (GameConfig::SHIP_HEIGHT / 2);
            int shieldRadius = 14;

            // Establish baseline safety colors
            uint16_t primaryColor = ST7735_CYAN;
            uint16_t secondaryColor = GameConfig::COLOR_ION_BLUE;

            // Check if shield has entered the final 2-second timeout window
            unsigned long timeLeft = (_shieldEndTime > millis()) ? (_shieldEndTime - millis()) : 0;
            
            if (timeLeft < 2000) {
                // Flash rapidly between neon colors and critical RED warning states every 50ms
                if ((millis() / 50) % 2 == 0) {
                    primaryColor = ST7735_RED;
                    secondaryColor = ST7735_ORANGE;
                }
            }

            // Draw the double-ring animated energy shield bubble
            if ((millis() / 80) % 2 == 0) {
                canvas.drawCircle(cx, cy, shieldRadius, primaryColor);
                canvas.drawCircle(cx, cy, shieldRadius - 1, secondaryColor); 
            }
        }
        
        // Draw Base Ship Asset automatically using the private _frames matrix
        canvas.drawRGBBitmap((int)_x, _y, _frames[_currentFrame], GameConfig::SHIP_WIDTH, GameConfig::SHIP_HEIGHT);
    }
};

#endif