#ifndef PARTICLE_MANAGER_H
#define PARTICLE_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "GameConfig.h"

class ParticleManager {
private:
    struct Particle {
        float x, y;
        float vx, vy;
        uint16_t color;
        unsigned long expireTime;
        bool active;
    };

    Particle _pool[GameConfig::MAX_PARTICLES];

public:
    ParticleManager() {
        for (int i = 0; i < GameConfig::MAX_PARTICLES; i++) {
            _pool[i].active = false;
        }
    }

    // Call this from anywhere to burst a cluster of debris
    void spawnExplosion(float centerX, float centerY, uint16_t color, int count) {
        int spawned = 0;
        
        // Search through the fixed pool for inactive slots
        for (int i = 0; i < GameConfig::MAX_PARTICLES; i++) {
            if (_pool[i].active) continue;

            _pool[i].active = true;
            _pool[i].x = centerX;
            _pool[i].y = centerY;
            
            // Generate circular, high-energy outward vectors
            float angle = random(0, 360) * (PI / 180.0f);
            float speed = (random(50, 250) / 100.0f); // Velocity speeds between 0.5 and 2.5
            
            _pool[i].vx = cos(angle) * speed;
            _pool[i].vy = sin(angle) * speed;
            _pool[i].color = color;
            _pool[i].expireTime = millis() + random(GameConfig::PARTICLE_LIFESPAN_MS / 2, GameConfig::PARTICLE_LIFESPAN_MS);

            spawned++;
            if (spawned >= count) break; // Stop once requested particle density is met
        }
    }

    void update() {
        unsigned long now = millis();
        for (int i = 0; i < GameConfig::MAX_PARTICLES; i++) {
            if (!_pool[i].active) continue;

            // Garbage collect particles that exceed their lifespan
            if (now >= _pool[i].expireTime) {
                _pool[i].active = false;
                continue;
            }

            // Apply inertia physics
            _pool[i].x += _pool[i].vx;
            _pool[i].y += _pool[i].vy;

            // Optional structural detail: apply cosmic resistance to slow them down over time
            _pool[i].vx *= 0.96f;
            _pool[i].vy *= 0.96f;
        }
    }

    void render(GFXcanvas16 &canvas) {
        unsigned long now = millis();
        for (int i = 0; i < GameConfig::MAX_PARTICLES; i++) {
            if (!_pool[i].active) continue;

            int cx = (int)_pool[i].x;
            int cy = (int)_pool[i].y;

            // Clip boundaries to ensure we don't bleed rendering into UI text margins
            if (cx < 0 || cx >= GameConfig::SCREEN_WIDTH || cy < GameConfig::UI_MARGIN_TOP || cy >= GameConfig::SCREEN_HEIGHT) {
                continue;
            }

            // High impact flashing effect: particles turn white right before burning out
            uint16_t currentColor = _pool[i].color;
            if (_pool[i].expireTime - now < 80) {
                currentColor = ST7735_WHITE;
            }

            canvas.drawPixel(cx, cy, currentColor);
        }
    }
    
    void clearAll() {
        for (int i = 0; i < GameConfig::MAX_PARTICLES; i++) {
            _pool[i].active = false;
        }
    }
};

#endif