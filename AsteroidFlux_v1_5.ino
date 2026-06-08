#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <Preferences.h>  
#include <Fonts/TomThumb.h>

// OOP Engine Modules
#include "GameConfig.h"
#include "AudioEngine.h"
#include "PlayerShip.h"
#include "PowerUpManager.h"
#include "AsteroidManager.h"
#include "ParticleManager.h"
#include "BackgroundStars.h"
#include "NebulaManager.h"

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
NebulaManager nebula;
ParticleManager particles;

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

enum AttractScreen { SCREEN_SPLASH, SCREEN_INFO };
AttractScreen currentAttractScreen = SCREEN_SPLASH;

unsigned long lastScreenSwitchTime = 0;
const unsigned long ATTRACT_SLIDE_DURATION = 8000; // Switch screens every 8 seconds (8000ms)

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
    waitForPlayerInput();
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
    nebula.update();
    background.update();
    particles.update();
    ship.updatePosition();
    ship.updateAnimation();
    ship.updateShield();

    // Process Background PowerUp Timeline Logic
    powerUps.update(score, ship, lives, uiNeedsUpdate, audio, asteroids);

    // Track Hazards Engine Math
    asteroids.update(ship, score, asteroidsPassed, nextTargetScore, uiNeedsUpdate, playerHit, audio, particles);

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
            particles.clearAll();
            runCountdown();
            return;
        }
    }

    // Refresh Background Buffer Layer
    canvas.fillRect(0, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT - GameConfig::UI_MARGIN_TOP, ST7735_BLACK);

    // Draw Entities onto Virtual Buffer Frame
    nebula.render(canvas);
    background.render(canvas);
    particles.render(canvas);
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
    particles.clearAll();
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
    tft.setCursor(GameConfig::SCREEN_WIDTH / 4 - 4, 94);
    tft.setTextColor(GameConfig::COLOR_GREY); 
    tft.print("BEST RECORD: ");
    tft.setTextColor(ST7735_YELLOW);
    tft.print(highScore);

    tft.setCursor(18, 115);
    tft.setTextColor(ST7735_CYAN);
    tft.print("MOVE CONTROL TO START");
}

void drawInfoScreen() {
    tft.fillScreen(ST7735_BLACK);
    
    // 1. Activate the tiny custom font
    tft.setFont(&TomThumb);

    // Header text
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(30, 10);
    tft.print("---== GAME REWARDS ==---");

    // 1. HAZARDS & SCORING BREAKDOWN
    tft.setTextColor(ST7735_YELLOW);
    tft.setCursor(15, 25);  tft.print("TINY:       +1 PTS");
    
    tft.setTextColor(ST7735_CYAN);
    tft.setCursor(95, 25);  tft.print("SMALL: +2 PTS");
    
    tft.setTextColor(ST7735_ORANGE);
    tft.setCursor(15, 35);  tft.print("MEDIUM:   +3 PTS");
    
    tft.setTextColor(ST7735_RED);
    tft.setCursor(95, 35);  tft.print("LARGE: +4 PTS");
    
    tft.setTextColor(ST7735_BLUE);
    tft.setCursor(15, 45);  tft.print("MASSIVE: +5 PTS");

    tft.setTextColor(ST7735_MAGENTA);
    tft.setCursor(95, 45);  tft.print("COMET: +15 PTS");

    tft.drawFastHLine(0, 55, GameConfig::SCREEN_WIDTH, 0x4208); // Subtle dark divider line

    // 2. POWER-UPS MATRIX
    tft.setCursor(40, 70);
    tft.setTextColor(ST7735_WHITE);
    tft.print("---== POWERUPS ==---");

    // Cyan Clock
    tft.setTextColor(ST7735_CYAN);
    tft.setCursor(5, 85);   tft.print("CLOCK:    ");
    tft.setTextColor(ST7735_WHITE);
    tft.print("SLOWS DOWN SECTOR");

    // Green Shield
    tft.setTextColor(ST7735_GREEN);
    tft.setCursor(5, 95);  tft.print("SHIELD:  ");
    tft.setTextColor(ST7735_WHITE);
    tft.print("ABSORBS 1 COLLISION - ON 10 SEC");

    // Red Health
    tft.setTextColor(ST7735_RED);
    tft.setCursor(5, 105);  tft.print("HEART:    ");
    tft.setTextColor(ST7735_WHITE);
    tft.print("EXTRA LIFE!! - AFTER 600 PTS");

    tft.setFont();

    // Footer Prompts
    tft.setTextColor(0x5AEB); // Retro arcade dimmed green
    tft.setCursor(14, 120);
    tft.print("MOVE CONTROLLER TO FLY");
    // Restore the default font before drawing your main UI or Title screen
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
    // 1. Calculate the exact center boundary of the ship
    float centerX = ship.getX() + (GameConfig::SHIP_WIDTH / 2.0f);
    float centerY = (float)ship.getY() + (GameConfig::SHIP_HEIGHT / 2.0f);
    unsigned int audioPointer = 44; // Skip the WAV header safely

    // 2. Trigger a massive particle dispersion matching the ship's ion-blue theme
    // We break the pool limits slightly here or fill it to max capacity
    particles.spawnExplosion(centerX, centerY, GameConfig::COLOR_ION_BLUE, 20);
    particles.spawnExplosion(centerX, centerY, ST7735_YELLOW, 10);

    sigmaDeltaAttach(GameConfig::PIN_SPEAKER, 312500);

    // Run a 6-frame rendering sequence matching your original animation timing
    for (int frame = 1; frame <= 12; frame++) {
        
        // 3. CRITICAL ENGINE STEP: Advance the particle physics and erase the canvas background
        particles.update();
        canvas.fillRect(0, GameConfig::UI_MARGIN_TOP, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT - GameConfig::UI_MARGIN_TOP, ST7735_BLACK);
        
        // 4. Render the flying ship debris onto the virtual buffer canvas
        particles.render(canvas);

        // Output the updated canvas frame to the hardware display screen
        tft.drawRGBBitmap(0, 0, canvas.getBuffer(), GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT);

        // 5. Keep the precise timing block for raw PCM audio data streaming
        // This loops 320 times per frame to maintain the correct sample rate pitch
        for (int sampleCount = 0; sampleCount < 260; sampleCount++) {
            if (audioPointer < sound_explosion_len) {
                uint8_t audioSample = pgm_read_byte(&explosion_data[audioPointer++]);
                sigmaDeltaWrite(GameConfig::PIN_SPEAKER, audioSample); 
            } else {
                sigmaDeltaWrite(GameConfig::PIN_SPEAKER, 128); // Static baseline balance
            }
            delayMicroseconds(125); // 8kHz clock interval constraint
        }
    }

    // 6. Safe Hardware Shutdown
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

    tft.setCursor(GameConfig::SCREEN_WIDTH / 4 - 12, 15);
    tft.setTextColor(ST7735_RED); 
    tft.setTextSize(2);
    tft.print("GAME OVER");

    //audio.playSample(gameend_data, sound_gameend_len);

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

    tft.setCursor(GameConfig::SCREEN_WIDTH / 4 - 28, 95);
    tft.setTextColor(ST7735_CYAN);
    tft.print("> TWIST DIAL TO RESET <");

    audio.playGameOverMelody();

    waitForPlayerInput();
    showWelcomeSplashScreen();
    initNewGame();
}

void waitForPlayerInput() {
    // 1. Capture the initial position of the dial right at bootup
    int startingValue = analogRead(GameConfig::PIN_DIAL);
    const int threshold = 150; 

    unsigned long lastScreenSwitchTime = millis();
    const unsigned long ATTRACT_SLIDE_DURATION = 8000; // 8 seconds per slide
    currentAttractScreen = SCREEN_SPLASH;

    // 2. Play the crisp startup fanfare exactly once right when the boot menu mounts
    audio.playStartupMelody();

    while (true) {
        // Keep the audio engine updated if any background alerts are ticking
        audio.update();

        // 3. INPUT INTERCEPTOR: Read the dial's current position
        int currentValue = analogRead(GameConfig::PIN_DIAL);
        
        // If the player spins the dial past your threshold, break out and start the game!
        if (abs(currentValue - startingValue) > threshold) {
            break;
        }

        // 4. SLIDE SLIDESHOW TRANSITION TIMER
        if (millis() - lastScreenSwitchTime >= ATTRACT_SLIDE_DURATION) {
            lastScreenSwitchTime = millis(); // Reset stopwatch

            if (currentAttractScreen == SCREEN_SPLASH) {
                currentAttractScreen = SCREEN_INFO;
                drawInfoScreen(); // Draws the point matrix and systems gear layout
            } else {
                currentAttractScreen = SCREEN_SPLASH;
                showWelcomeSplashScreen(); // Redraws your custom bitmap asset
            }
            
            // CRITICAL DIAL ADJUSTMENT: Re-sample the dial baseline right after a screen swap.
            // This prevents a slow drift or past movement from instantly triggering a false game start
            // the exact millisecond a new slide loads.
            startingValue = analogRead(GameConfig::PIN_DIAL);
        }

        // Maintained your 16ms frame-pacing delay to match original timing
        delay(16); 
    }

    // Cleanly mute everything before entering the core game loop
    audio.mute();
}