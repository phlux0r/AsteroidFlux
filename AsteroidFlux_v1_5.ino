#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <Preferences.h>  

// OOP Engine Modules
#include "GameConfig.h"
#include "AudioEngine.h"
#include "PlayerShip.h"
#include "PowerUpManager.h"
#include "AsteroidManager.h"
#include "BackgroundStars.h"

// Binary Memory Assets
#include "splash_image.h"
#include "explosion.h"
#include "gamestart.h"
#include "gameend.h"

// Global Framework Instances
Adafruit_ST7735 tft = Adafruit_ST7735(GameConfig::PIN_TFT_CS, GameConfig::PIN_TFT_DC, GameConfig::PIN_TFT_RST);
GFXcanvas16 canvas(GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);
Preferences preferences;

// Instantiate Game Objects
AudioEngine audio;
PlayerShip ship;
PowerUpManager powerUps;
AsteroidManager asteroids;
BackgroundStars background;

// Shared Game Core State Variables
int score = 0;
int highScore = 0; 
int lives = 3;
int asteroidsPassed = 0;
int nextTargetScore = GameConfig::SCORE_TO_SPAWN;

// Continuous Array Length Extractions
const unsigned int sound_explosion_len = sizeof(explosion_data);
const unsigned int sound_gamestart_len = sizeof(gamestart_data);
const unsigned int sound_gameend_len = sizeof(gameend_data);

// Forward Control Loop Declarations
void initNewGame();
void drawUI();
void runCountdown();
void triggerShipExplosion();
void gameOverSequence();
void waitForPlayerInput();
void showWelcomeSplashScreen();

void setup() {
    Serial.begin(115200);
    
    pinMode(GameConfig::PIN_BLK, OUTPUT);
    digitalWrite(GameConfig::PIN_BLK, HIGH);
    pinMode(GameConfig::PIN_SPEAKER, OUTPUT);
    digitalWrite(GameConfig::PIN_SPEAKER, LOW);

    tft.initR(INITR_BLACKTAB);
    tft.setSPISpeed(27000000);
    SPI.setFrequency(40000000);
    tft.setRotation(1); 

    randomSeed(analogRead(A0) + analogRead(GameConfig::PIN_DIAL));

    // High Score Clear Detection Gate
    if (analogRead(GameConfig::PIN_DIAL) < 100) {
        preferences.begin("game_data", false);
        preferences.clear();
        preferences.end();
        tone(GameConfig::PIN_SPEAKER, 400, 200); delay(200); 
        tone(GameConfig::PIN_SPEAKER, 200, 400); delay(400);
        delay(1000);
    }

    preferences.begin("game_data", false);
    highScore = preferences.getInt("highscore", 0);
    
    showWelcomeSplashScreen();
    initNewGame();
}

void loop() {
    static unsigned long lastFrame = 0;
    while (micros() - lastFrame < 16666) { delayMicroseconds(10); }
    lastFrame = micros();

    bool uiNeedsUpdate = false;
    bool playerHit = false;

    // Run Engine Object Steps
    audio.update();
    background.update();
    ship.updatePosition();
    ship.updateAnimation();
    ship.updateShield();

    // Process Background PowerUp Timeline Logic
    powerUps.update(score, ship, lives, uiNeedsUpdate, audio, asteroids);

    // Track Hazards Engine Math
    asteroids.update(ship, score, asteroidsPassed, nextTargetScore, uiNeedsUpdate, playerHit, audio);

    if (playerHit) {
        triggerShipExplosion();
        lives--;
        delay(300);
        if (lives <= 0) {
            gameOverSequence();
            return;
        } else {
            tft.fillScreen(ST7735_BLACK);
            drawUI();
            asteroids.forceBoardWipe();
            runCountdown();
            return;
        }
    }

    // Refresh Background Buffer Layer
    canvas.fillRect(0, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT - GameConfig::UI_MARGIN_TOP, ST7735_BLACK);

    // Draw Entities onto Virtual Buffer Frame
    background.render(canvas);
    powerUps.render(canvas);
    asteroids.render(canvas);
    ship.render(canvas);

    if (uiNeedsUpdate) {
        drawUI();
    }

    // Output Complete Matrix directly to Hardware Frame Buffer
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);
}

void initNewGame() {
    score = 0;
    lives = 3;
    asteroidsPassed = 0;
    nextTargetScore = GameConfig::SCORE_TO_SPAWN;
    ship.reset();
    powerUps.resetTimeline();
    asteroids.initGame();

    tft.fillScreen(ST7735_BLACK);
    drawUI();
    runCountdown();
}

void showWelcomeSplashScreen() {
    tft.fillScreen(ST7735_BLACK);
    for (int i = 0; i < 20480; i++) {
        uint8_t byte1 = splash_bitmap[i * 2];
        uint8_t byte2 = splash_bitmap[i * 2 + 1];
        uint16_t correctedPixel = (byte2 << 8) | byte1;
        tft.drawPixel(i % GameConfig::SCREEN_WIDTH, i / GameConfig::SCREEN_WIDTH, correctedPixel);
    }

    tft.setTextSize(1);
    tft.setCursor(GameConfig::SCREEN_WIDTH / 4, 94);
    tft.setTextColor(GameConfig::COLOR_GREY); 
    tft.print("BEST RECORD: ");
    tft.setTextColor(ST7735_YELLOW);
    tft.print(highScore);

    tft.setCursor(18, 115);
    tft.setTextColor(ST7735_CYAN);
    tft.print("MOVE CONTROL TO START");
    
    audio.playSample(gamestart_data, sound_gamestart_len);
    waitForPlayerInput();
}

void drawUI() {
    canvas.fillRect(0, GameConfig::UI_MARGIN_TOP - 1, GameConfig::SCREEN_WIDTH, 1, ST7735_GREEN);
    canvas.fillRect(0, 0, GameConfig::SCREEN_WIDTH, GameConfig::UI_MARGIN_TOP - 2, ST7735_BLACK);
    canvas.setTextSize(1);
    
    canvas.setTextColor(ST7735_YELLOW);
    canvas.setCursor(4, 1);
    canvas.print("SCORE:"); canvas.print(score);

    canvas.setTextColor(GameConfig::COLOR_GREY); 
    canvas.setCursor(GameConfig::SCREEN_WIDTH / 2 - 10, 1);
    canvas.print("HI:"); canvas.print(highScore);

    canvas.setCursor(GameConfig::SCREEN_WIDTH - 40, 1);
    canvas.setTextColor(ST7735_RED);
    canvas.print(lives); canvas.print(" UP");
}

void runCountdown() {
    canvas.fillRect(0, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT - GameConfig::UI_MARGIN_TOP, ST7735_BLACK);
    for (int i = 3; i > 0; i--) {
        canvas.setCursor(GameConfig::SCREEN_WIDTH / 2 - 6, GameConfig::SCREEN_HEIGHT / 2 - 8);
        canvas.setTextColor(ST7735_CYAN);
        canvas.setTextSize(2);
        canvas.print(i);
        tft.drawRGBBitmap(0, 0, canvas.getBuffer(), GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);
        tone(GameConfig::PIN_SPEAKER, 800, 100); 
        delay(600);
        canvas.fillRect(GameConfig::SCREEN_WIDTH / 2 - 10, GameConfig::SCREEN_HEIGHT / 2 - 12, 20, 24, ST7735_BLACK);
        delay(150);
    }
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);
}

void triggerShipExplosion() {
    int centerX = (int)ship.getX() + (GameConfig::SHIP_WIDTH / 2);
    int centerY = ship.getY() + (GameConfig::SHIP_HEIGHT / 2);
    unsigned int audioPointer = 44;

    sigmaDeltaAttach(GameConfig::PIN_SPEAKER, 312500);

    for (int frame = 1; frame <= 6; frame++) {
        int innerRadius = frame * 3;
        int outerRadius = frame * 7;
        uint16_t primaryColor = (frame % 2 == 0) ? ST7735_WHITE : ST7735_YELLOW;
        uint16_t secondaryColor = (frame % 2 == 0) ? ST7735_YELLOW : ST7735_WHITE;

        canvas.drawLine(centerX, centerY - innerRadius, centerX, centerY - outerRadius, primaryColor);
        canvas.drawLine(centerX, centerY + innerRadius, centerX, centerY + outerRadius, primaryColor);
        canvas.drawLine(centerX - innerRadius, centerY, centerX - outerRadius, centerY, primaryColor);
        canvas.drawLine(centerX + innerRadius, centerY, centerX + outerRadius, centerY, primaryColor);

        int diagInner = (int)(innerRadius * 0.71f);
        int diagOuter = (int)(outerRadius * 0.71f);
        canvas.drawLine(centerX - diagInner, centerY - diagInner, centerX - diagOuter, centerY - diagOuter, secondaryColor);
        canvas.drawLine(centerX + diagInner, centerY - diagInner, centerX + diagOuter, centerY - diagOuter, secondaryColor);
        canvas.drawLine(centerX - diagInner, centerY + diagInner, centerX - diagOuter, centerY + diagOuter, secondaryColor);
        canvas.drawLine(centerX + diagInner, centerY + diagInner, centerX + diagOuter, centerY + diagOuter, secondaryColor);

        tft.drawRGBBitmap(0, 0, canvas.getBuffer(), GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);

        for (int sampleCount = 0; sampleCount < 320; sampleCount++) {
            if (audioPointer < sound_explosion_len) {
                uint8_t audioSample = pgm_read_byte(&explosion_data[audioPointer++]);
                sigmaDeltaWrite(GameConfig::PIN_SPEAKER, audioSample); 
            } else {
                sigmaDeltaWrite(GameConfig::PIN_SPEAKER, 128);
            }
            delayMicroseconds(125); 
        }

        canvas.fillRect(0, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT - GameConfig::UI_MARGIN_TOP, ST7735_BLACK);
    }

    sigmaDeltaWrite(GameConfig::PIN_SPEAKER, 0);
    sigmaDeltaDetach(GameConfig::PIN_SPEAKER);
    pinMode(GameConfig::PIN_SPEAKER, OUTPUT);
    digitalWrite(GameConfig::PIN_SPEAKER, LOW);
    audio.mute();
}

void gameOverSequence() {
    tft.fillScreen(ST7735_BLACK);
    bool newRecord = false;
    if (score > highScore) {
        highScore = score;
        newRecord = true;
        preferences.putInt("highscore", highScore);
    }

    tft.setCursor(GameConfig::SCREEN_WIDTH / 4 - 5, 15);
    tft.setTextColor(ST7735_RED); 
    tft.setTextSize(2);
    tft.print("GAME OVER");

    audio.playSample(gameend_data, sound_gameend_len);

    tft.setTextSize(1);
    tft.setCursor(GameConfig::SCREEN_WIDTH / 4, 45);
    tft.setTextColor(ST7735_WHITE);
    tft.print("YOUR SCORE: ");
    tft.setTextColor(ST7735_YELLOW);
    tft.print(score);

    tft.setCursor(GameConfig::SCREEN_WIDTH / 4, 65);
    if (newRecord) {
        tft.setTextColor(ST7735_GREEN);
        tft.print("NEW HIGH SCORE!! ");
    } else {
        tft.setTextColor(GameConfig::COLOR_GREY); 
        tft.print("HIGH SCORE: ");
        tft.print(highScore);
    }

    tft.setCursor(GameConfig::SCREEN_WIDTH / 4 - 20, 95);
    tft.setTextColor(ST7735_CYAN);
    tft.print("> TWIST DIAL TO RESET <");

    waitForPlayerInput();
    showWelcomeSplashScreen();
    initNewGame();
}

void waitForPlayerInput() {
    int startingValue = analogRead(GameConfig::PIN_DIAL);
    const int threshold = 150; 
    
    while (true) {
        audio.update();
        delay(1);
        int currentValue = analogRead(GameConfig::PIN_DIAL);
        if (abs(currentValue - startingValue) > threshold) break;
        delay(16); 
    }
    audio.mute();
}