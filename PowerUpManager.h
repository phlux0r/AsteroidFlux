#ifndef POWERUP_MANAGER_H
#define POWERUP_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "GameConfig.h"
#include "PlayerShip.h"
#include "AudioEngine.h"

enum PowerUpType { NONE, EXTRA_LIFE, SHIELD };

class PowerUpManager {
private:
    struct PowerUpData {
        float x;
        float y;
        float vx;
        float radius;
        PowerUpType type;
        bool active;
        uint16_t color;
    };

    PowerUpData _data;
    unsigned long _nextSpawnTime;

public:
    PowerUpManager() {
        _data = {0, 0, 0, 6.0f, NONE, false, 0};
        resetTimeline();
    }

    void resetTimeline() {
        _data.active = false;
        _nextSpawnTime = millis() + random(GameConfig::POWERUP_SPAWN_LOW_MS, GameConfig::POWERUP_SPAWN_HIGH_MS);
    }

    void update(int score, PlayerShip &ship, int &lives, bool &uiUpdate, AudioEngine &audio) {
        if (!_data.active && score >= GameConfig::POWERUP_START_SCORE && millis() >= _nextSpawnTime) {
            PowerUpType rolledType = SHIELD;
            uint16_t rolledColor = ST7735_CYAN;

            int probabilityDice = random(0, 100);
            if (probabilityDice < GameConfig::EXTRA_LIFE_CHANCE) {
                if (score >= GameConfig::MIN_SCORE_FOR_EXTRA_LIFE) {
                    rolledType = EXTRA_LIFE;
                    rolledColor = ST7735_MAGENTA; 
                }
            }

            _data.active = true;
            _data.x = GameConfig::SCREEN_WIDTH + 20;
            _data.y = random(20, GameConfig::SCREEN_HEIGHT - 20);
            _data.vx = -1.2f;
            _data.type = rolledType;
            _data.color = rolledColor;
        }

        if (!_data.active) return;

        _data.x += _data.vx;

        if (_data.x + _data.radius < 0) {
            resetTimeline();
            return;
        }

        // Object Oriented Collision Calculations
        float shipLeft   = ship.getX();
        float shipRight  = ship.getX() + GameConfig::SHIP_WIDTH;
        float shipTop    = (float)ship.getY();
        float shipBottom = (float)(ship.getY() + GameConfig::SHIP_HEIGHT);
        
        float distSqX = max(shipLeft, min(_data.x, shipRight));
        float distSqY = max(shipTop,  min(_data.y, shipBottom));
        
        float deltaX = _data.x - distSqX;
        float deltaY = _data.y - distSqY;
        float distSq = (deltaX * deltaX) + (deltaY * deltaY);
        
        if (distSq < (_data.radius * _data.radius)) {
            if (_data.type == EXTRA_LIFE) {
                lives++;
                uiUpdate = true;
                audio.playSound(1500, 150); 
            } 
            else if (_data.type == SHIELD) {
                ship.activateShield();
                audio.playSound(1000, 250);
            }
            resetTimeline();
        }
    }

    void render(GFXcanvas16 &canvas) {
        if (!_data.active) return;

        int cx = (int)_data.x;
        int cy = (int)_data.y;
        int r = (int)_data.radius;

        if (cx < -20 || cx > GameConfig::SCREEN_WIDTH + 20) return;

        if (_data.type == EXTRA_LIFE) {
            canvas.fillCircle(cx - r/2, cy - r/4, r/2 + 1, _data.color);
            canvas.fillCircle(cx + r/2, cy - r/4, r/2 + 1, _data.color);
            canvas.fillTriangle(cx - r, cy, cx + r, cy, cx, cy + r + 2, _data.color);
        } 
        else if (_data.type == SHIELD) {
            int w = r * 2;
            int h = r * 2;
            canvas.fillRoundRect(cx - r, cy - r, w, h, 3, _data.color);
            canvas.drawFastHLine(cx - r + 3, cy, w - 6, ST7735_BLACK);
            canvas.drawFastVLine(cx, cy - r + 3, h - 6, ST7735_BLACK);
        }
    }
};

#endif