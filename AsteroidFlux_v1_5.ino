#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h> 
#include <SPI.h>
#include <Preferences.h>  
#include "splash_image.h"
#include "explosion.h"
#include "gamestart.h"
#include "gameend.h"

// --- PIN DEFINITIONS ---
#define TFT_CS     10
#define TFT_DC      8
#define TFT_RST     9
#define BLK_PIN     7  
#define DIAL_PIN    1  // Grove Yellow Wire (Analog In)
#define SPEAKER_PIN 5

// --- GAME BALANCING CONSTANTS ---
const int MAX_ASTEROIDS = 6;      
const int SCORE_TO_SPAWN = 10;    
const float BASE_SPEED = 1.1;
const float SPEED_STEP = 0.07;
const int POWERUP_START_SCORE = 100;
const int MIN_SCORE_FOR_EXTRA_LIFE = 600;
const int SHIELD_DURATION = 10000;
const int EXTRA_LIFE_CHANCE = 25;
const int POWERUP_RND_SPWN_LOW = 15000;
const int POWERUP_RND_SPWN_HIGH = 40000;
// --- NOTE FREQUENCY DEFINITIONS ---
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_REST 0    // Use 0 Hz to represent a silent pause between notes
// The notes of our melody played in sequence
const int startupMelody[] = {
  NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5, NOTE_B4, NOTE_B4
};
// The duration of each note in milliseconds (200ms = short, fast note)
const int startupDurations[] = {
  100, 100, 100, 100, 300, 150, 150
};
// --- SONG 2: GAME OVER MELODY (4 Notes) ---
const int gameOverMelody[] = { NOTE_G4, NOTE_E4, NOTE_D4, NOTE_C4 };
const int gameOverDurations[] = { 250,  250,  250,  600 };

// Sound length
const unsigned int sound_explosion_len = sizeof(explosion_data);
const unsigned int sound_gamestart_len = sizeof(gamestart_data);
const unsigned int sound_gameend_len = sizeof(gameend_data);

// --- 16x11 SIDE-VIEW SHIP BITMAP (16-bit RGB565 Colors) ---
#define W ST7735_WHITE
#define B ST7735_BLACK
#define C ST7735_CYAN
#define L ST7735_BLUE
#define O ST7735_ORANGE  // Orange Engine Flare
#define R ST7735_RED
#define M ST7735_MAGENTA

const uint16_t ship_frames[3][160] PROGMEM = {
  // --- FRAME 0: Long Thrust Flare ---
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
  // --- FRAME 1: Shifted/Shorter Core ---
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
  // --- FRAME 2: Split Fire Burst ---
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
// Initialize display and flash storage
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Create a 16-bit color canvas matching your 160x128 resolution
GFXcanvas16 canvas(160, 128);
Preferences preferences; 

// --- GAME STRUCTURES ---
struct Asteroid {
  float x;
  float y;
  int sizeClass; 
  float radius;
  uint16_t color;
  bool active;
  
  float xOffsets[8];
  float yOffsets[8];
  
  // Comet Trait flags
  bool isComet;
  float speedMultiplier;

  // --- ADD THESE PHYSICS VARIABLES ---
  float vx;          // Horizontal velocity vector
  float vy;          // Vertical velocity vector (up/down drift)
  float angle;       // Current tumbling angle (0 to 360 degrees)
  float spinSpeed;   // How fast it rotates per frame
};

// --- POWER-UP CONFIGURATION ---
enum PowerUpType { NONE, EXTRA_LIFE, SHIELD };

struct PowerUp {
  float x;
  float y;
  float vx;
  float radius;
  PowerUpType type;
  bool active;
  uint16_t color;
};

// Global Power-Up Instances
PowerUp activePowerUp = { 0, 0, 0, 6.0, NONE, false, 0 };
unsigned long nextPowerUpSpawnTime = random(POWERUP_RND_SPWN_LOW, POWERUP_RND_SPWN_HIGH);

// Shield System Timers
bool shieldActive = false;
unsigned long shieldEndTime = 0;
// --- GLOBAL STATE VARIABLES ---
int screenWidth, screenHeight;
int score = 0;
int highScore = 0; 
int lives = 3;
int asteroidsPassed = 0;
int nextTargetScore = SCORE_TO_SPAWN; // Tracks the next milestone for difficulty scaling
// Melody setups
const int* currentMelody = nullptr;    // Points to the active note array
const int* currentDurations = nullptr; // Points to the active duration array
int totalNotes = 0;                     // Tracks the size of the active song
int currentNoteIndex = -1;    // -1 means no melody is currently playing
unsigned long nextNoteTime = 0; // Tracks when the current note should finish
bool melodyPlaying = false;

// Audio state variables
unsigned long soundEndTime = 0;
bool soundActive = false;

// Ship configuration
const int shipWidth = 16;
const int shipHeight = 10;
float shipX = 15; 
int shipY, oldShipY;
// --- SHIP ANIMATION CONFIG ---
int shipCurrentFrame = 0;
unsigned long nextShipFrameTime = 0;
const int SHIP_ANIM_SPEED = 80; // Time in milliseconds per frame (80ms = snappy flicker)
const int COMET_BONUS_SCORE = 15;

// Object pooling
Asteroid pool[MAX_ASTEROIDS];
int currentMaxActive = 1; 
float currentSpeed = BASE_SPEED;

// Analog filter
float dialFiltered = 2048;
bool cometOnScreen = false; // Tracks if a comet is currently active on screen

// Forward Declarations
void resetAsteroid(int index, bool startOffScreen);
void drawJaggedAsteroid(Asteroid& ast, uint16_t colorOverride);
void drawCometTeardrop(Asteroid& ast, uint16_t colorOverride); // New Render Feature
void runCountdown();
void drawUI();
void gameOverSequence();
void waitForPlayerInput(); 
void showWelcomeSplashScreen(); 

void setup() {
  Serial.begin(115200);
  
  pinMode(BLK_PIN, OUTPUT);
  digitalWrite(BLK_PIN, HIGH);

  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);

  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(27000000);
  SPI.setFrequency(40000000); 

  tft.setRotation(1); 
  screenWidth = tft.width();
  screenHeight = tft.height();

  randomSeed(analogRead(A0) + analogRead(DIAL_PIN));
  // --- HIGH SCORE WIPE CHECK ---
  // If the dial is twisted fully counter-clockwise (near 0) when booting:
  if (analogRead(DIAL_PIN) < 100) {
    // 1. Open preferences in read/write mode ("game_data" must match your usual namespace)
    preferences.begin("game_data", false);
    
    // 2. Safely wipe the specific highscore value or clear the whole namespace
    preferences.clear(); // Wipes all keys (lives, scores, etc.) in "game_data"
    // OR just preferences.putInt("highscore", 0); to only wipe the score
    
    preferences.end();
    
    // 3. Optional: Play a "wiped" sound effect
    tone(SPEAKER_PIN, 400, 200); delay(200); 
    tone(SPEAKER_PIN, 200, 400); delay(400);

    // Give the user time to let go of the dial before moving on
    delay(1000); 
  }
  preferences.begin("game_data", false);
  highScore = preferences.getInt("highscore", 0);
  
  showWelcomeSplashScreen();
  initNewGame();
}

void initNewGame() {
  score = 0;
  lives = 3;
  asteroidsPassed = 0;
  currentMaxActive = 1;
  currentSpeed = BASE_SPEED;
  cometOnScreen = false; // <-- Clear flag on a brand new game
  nextTargetScore = SCORE_TO_SPAWN; // Reset progression threshold back to 10

  shieldActive = false;
  activePowerUp.active = false;
  // CHANGED: Randomize the startup timer for every new life session
  nextPowerUpSpawnTime = millis() + random(POWERUP_RND_SPWN_LOW, POWERUP_RND_SPWN_HIGH);

  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    pool[i].active = (i < currentMaxActive);
    resetAsteroid(i, true); 
  }

  tft.fillScreen(ST7735_BLACK);
  drawUI();
  runCountdown();
}

void resetAsteroid(int index, bool startOffScreen) {

  // THE FIX: Explicitly check IF it was a comet, don't just assume.
  if (pool[index].active && pool[index].isComet) {
    cometOnScreen = false;
  }
  pool[index].isComet = false;
  pool[index].speedMultiplier = 1.0;

  // Roll a hidden dice for a random Comet spawn
  // Condition: Must have 3+ active slots, roll a 0, AND no other comet can be on screen!
  if (currentMaxActive >= 3 && !cometOnScreen && random(0, 20) == 0) { 
    pool[index].isComet = true;
    cometOnScreen = true;           // Lock the global slot so no other asteroid can become a comet
    pool[index].sizeClass = 1;      
    pool[index].radius = 2.5;       
    pool[index].color = ST7735_MAGENTA; 
    pool[index].speedMultiplier = 1.0; 
  }
  else {
    // Generate a standard rocky asteroid
    pool[index].sizeClass = random(1, 6); 
    switch (pool[index].sizeClass) {
      case 1: pool[index].radius = 3.0; pool[index].color = ST7735_YELLOW; break;
      case 2: pool[index].radius = 5.0; pool[index].color = ST7735_CYAN;  break;
      case 3: pool[index].radius = 7.0; pool[index].color = ST7735_ORANGE; break; 
      case 4: pool[index].radius = 9.0; pool[index].color = ST7735_RED;   break;
      case 5: pool[index].radius = 10.0; pool[index].color = ST7735_BLUE;   break;
    }

    for (int i = 0; i < 8; i++) {
      float angle = i * (PI / 4.0);
      float irregularRadius = pool[index].radius * (random(70, 131) / 100.0);
      pool[index].xOffsets[i] = cos(angle) * irregularRadius;
      pool[index].yOffsets[i] = sin(angle) * irregularRadius;
    }

    // --- PHYSICS ENGINE INITIALIZATION ---
    // 1. Calculate a absolute positive speed value first
    float calculatedSpeed = currentSpeed;
  
    // Apply comet multiplier if applicable
    if (pool[index].isComet) {
      calculatedSpeed = currentSpeed * 1.5f;
      if (calculatedSpeed > 6.0f) calculatedSpeed = 6.0f + (currentSpeed/10); // Speed ceiling cap
    }

    // 2. FORCE the direction to be strictly leftward by applying a negative sign here
    pool[index].vx = -abs(calculatedSpeed); 

    // 3. Keep the up/down vertical drift gentle (0.2 to 0.4 max pixels per frame)
    // If they drift too fast vertically, they look like chaotic pinballs
    pool[index].vy = (random(-30, 31) / 100.0f); 

    // 4. Rotational initialization
    pool[index].angle = random(0, 360);
    pool[index].spinSpeed = (random(-3, 4)); // Gentle tumble between -3° and +3°
  }

  if (startOffScreen) {
    pool[index].x = screenWidth + random(15, 80) + (index * 35); 
  } else {
    // Comets spawn slightly deeper offscreen to allow warning time
    pool[index].x = screenWidth + (pool[index].isComet ? random(40, 90) : random(15, 45));
  }
  
  pool[index].y = random(15 + pool[index].radius, screenHeight - pool[index].radius - 6);
}

void drawJaggedAsteroid(Asteroid& ast, uint16_t colorOverride) {
// Convert current rotation angle to radians
  float rad = ast.angle * (PI / 180.0f);
  float cosA = cos(rad);
  float sinA = sin(rad);

  int rotatedX[8];
  int rotatedY[8];

  // Rotate each vertex around the asteroid's center point (x, y)
  for (int i = 0; i < 8; i++) {
    float rawX = ast.xOffsets[i];
    float rawY = ast.yOffsets[i];

    // Standard 2D rotation matrix math
    rotatedX[i] = (int)(ast.x + (rawX * cosA - rawY * sinA));
    rotatedY[i] = (int)(ast.y + (rawX * sinA + rawY * cosA));
  }

  // Direct rendering with no individual clipping checks needed anymore!
  for (int i = 0; i < 8; i++) {
    int nextIndex = (i + 1) % 8;
    canvas.drawLine(rotatedX[i], rotatedY[i], rotatedX[nextIndex], rotatedY[nextIndex], colorOverride);
  }
}

// Custom Vector Engine to render the Teardrop Comet
void drawCometTeardrop(Asteroid& ast, uint16_t colorOverride) {
  int cx = (int)ast.x;
  int cy = (int)ast.y;
  int r = (int)ast.radius;

  // Draw the bright glowing comet core head
  canvas.drawCircle(cx, cy, r, colorOverride);

  // Only draw the trailing tail if we aren't using ST7735_BLACK to erase
  if (colorOverride != ST7735_BLACK) {
    
    // Create a fast 2-frame toggle timer (changes every 60ms)
    bool alternateFrame = (millis() / 60) % 2 == 0;

    if (alternateFrame) {
      // --- TRAIL FRAME A: Sleek, Long Energy Beam ---
      // Center stream extends far back (11 pixels)
      canvas.drawLine(cx + r, cy, cx + r + 11, cy, ST7735_WHITE);               
      // Tight upper and lower flank lines (light blue/cyan ion trail)
      canvas.drawLine(cx, cy - r, cx + r + 8, cy - 1, 0xCE7F);                  
      // Inner accent spark
      canvas.drawLine(cx, cy + r, cx + r + 8, cy + 1, 0xCE7F);                  
    } 
    else {
      // --- TRAIL FRAME B: Widened, Flared Plasma Burst ---
      // Center stream shortens slightly (7 pixels) but thickens
      canvas.drawLine(cx + r, cy, cx + r + 7, cy, ST7735_WHITE);
      canvas.drawLine(cx + r, cy - 1, cx + r + 6, cy, ST7735_WHITE);
      canvas.drawLine(cx + r, cy + 1, cx + r + 6, cy, ST7735_WHITE);
      
      // Flank lines flare outward further at the tips to simulate gas expansion
      canvas.drawLine(cx, cy - r, cx + r + 9, cy - 3, 0xCE7F);                  
      canvas.drawLine(cx, cy + r, cx + r + 9, cy + 3, 0xCE7F);                  
    }
  }
}
// --- Powerup UI ---
void drawPowerUp(PowerUp& p) {
  if (!p.active) return;

  int cx = (int)p.x;
  int cy = (int)p.y;
  int r = (int)p.radius;

  if (cx < -20 || cx > screenWidth + 20) return;

  if (p.type == EXTRA_LIFE) {
    // --- FILLED ARCADE HEART ---
    // Draw two solid circles for the top rounded lobes of the heart
    canvas.fillCircle(cx - r/2, cy - r/4, r/2 + 1, p.color);
    canvas.fillCircle(cx + r/2, cy - r/4, r/2 + 1, p.color);
    
    // Draw a solid downward triangle to form the sharp bottom point
    // Arguments: (x0, y0, x1, y1, x2, y2, color)
    canvas.fillTriangle(cx - r, cy, cx + r, cy, cx, cy + r + 2, p.color);
  } 
  else if (p.type == SHIELD) {
    // --- FILLED TACTICAL SHIELD POD ---
    // Draw a solid rounded rectangle for a clean, chunky look
    // Arguments: (x, y, width, height, corner_radius, color)
    int width = r * 2;
    int height = r * 2;
    canvas.fillRoundRect(cx - r, cy - r, width, height, 3, p.color);
    
    // Give it an inner technological detail by stamping a black core crosshair
    canvas.drawFastHLine(cx - r + 3, cy, (r * 2) - 6, ST7735_BLACK);
    canvas.drawFastVLine(cx, cy - r + 3, (r * 2) - 6, ST7735_BLACK);
  }
}

// Visual indicator for when the ship has an active shield bubble wrapped around it
void drawShieldBubble(int sx, int sy) {
  int cx = sx + (shipWidth / 2);
  int cy = sy + (shipHeight / 2);
  int shieldRadius = 14;

  // Pulse effect: toggles every 80ms
  if ((millis() / 80) % 2 == 0) {
    // Draw two nested circles to create a thick, glowing 2-pixel wall
    canvas.drawCircle(cx, cy, shieldRadius, ST7735_CYAN);
    canvas.drawCircle(cx, cy, shieldRadius - 1, 0xCE7F); // Light blue/cyan mix
  }
}

// --- DYNAMIC BACKGROUND PWM AUDIO PLAYER ---
void playSample(const unsigned char* sound_data, unsigned int sound_data_len) {
  // CORE 3.0 FIX: Use sigmaDeltaAttach with only 2 arguments (Pin, Frequency)
  sigmaDeltaAttach(SPEAKER_PIN, 312500); 
  sigmaDeltaWrite(SPEAKER_PIN, 128); // Set initial silent bias

  // Stream our audio bytes cleanly (skipping the 44-byte WAV header)
  for (unsigned int i = 0; i < sound_data_len; i++) {
    uint8_t audioSample = pgm_read_byte(&sound_data[i]);
    
    // CORE 3.0 FIX: In 3.x, write directly to the Pin instead of a channel index
    sigmaDeltaWrite(SPEAKER_PIN, audioSample);
    
    delayMicroseconds(125); // 8000Hz pacing
  }
  
  // Tear down using the new 3.x detach function
  sigmaDeltaWrite(SPEAKER_PIN, 0);
  sigmaDeltaDetach(SPEAKER_PIN);
  
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  noTone(SPEAKER_PIN); 
}

void showWelcomeSplashScreen() {
  tft.fillScreen(ST7735_BLACK);
  // 1. Manually swap and draw the big-endian bytes pixel by pixel
  // Total pixels = 160 * 128 = 20480
  for (int i = 0; i < 20480; i++) {
    // Grab the two 8-bit halves for this pixel
    uint8_t byte1 = splash_bitmap[i * 2];
    uint8_t byte2 = splash_bitmap[i * 2 + 1];

    // Swap their positions to create a correct 16-bit Little-Endian pixel
    uint16_t correctedPixel = (byte2 << 8) | byte1;

    // Calculate the X and Y screen coordinates for this pixel index
    int x = i % screenWidth;
    int y = i / screenWidth;

    // Push the single corrected pixel to the screen
    tft.drawPixel(x, y, correctedPixel);
  }
  // write the record and welcome
  tft.setTextSize(1);
  tft.setCursor(screenWidth / 4, 94);
  tft.setTextColor(0x7BEF); 
  tft.print("BEST RECORD: ");
  tft.setTextColor(ST7735_YELLOW);
  tft.print(highScore);

  tft.setCursor(18, 115);
  tft.setTextColor(ST7735_CYAN);
  tft.print("MOVE CONTROL TO START");
  
  //int notes = sizeof(startupMelody) / sizeof(startupMelody[0]);
  //playMelody(startupMelody, startupDurations, notes);
  playSample(gamestart_data, sound_gamestart_len);

  waitForPlayerInput();
}

void drawUI() {
  canvas.fillRect(0, 10, screenWidth, 1, ST7735_GREEN);
  canvas.fillRect(0, 0, screenWidth, 9, ST7735_BLACK);
  canvas.setTextSize(1);
  
  canvas.setTextColor(ST7735_YELLOW);
  canvas.setCursor(4, 1);
  canvas.print("SCORE:"); canvas.print(score);

  canvas.setTextColor(0x7BEF); 
  canvas.setCursor(screenWidth / 2 - 10, 1);
  canvas.print("HI:"); canvas.print(highScore);

  canvas.setCursor(screenWidth - 40, 1);
  canvas.setTextColor(ST7735_RED);
  canvas.print(lives); canvas.print(" UP");
}

void runCountdown() {
  canvas.fillRect(0, 11, screenWidth, screenHeight - 11, ST7735_BLACK);
  for (int i = 3; i > 0; i--) {
    canvas.setCursor(screenWidth / 2 - 6, screenHeight / 2 - 8);
    canvas.setTextColor(ST7735_CYAN);
    canvas.setTextSize(2);
    canvas.print(i);
    // Push the countdown number to the screen
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
    tone(SPEAKER_PIN, 800, 100); // Sharp warning beep
    delay(600);
    canvas.fillRect(screenWidth / 2 - 10, screenHeight / 2 - 12, 20, 24, ST7735_BLACK);
    delay(150);
  }
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
}

// Triggers a sound effect that stops automatically after durationMs
void playSound(int frequency, int durationMs) {
  tone(SPEAKER_PIN, frequency);
  soundEndTime = millis() + durationMs;
  soundActive = true;
}

// Call this function and pass the specific song you want to play
void playMelody(const int* melodyArray, const int* durationArray, int noteCount) {
  currentMelody = melodyArray;
  currentDurations = durationArray;
  totalNotes = noteCount;
  
  currentNoteIndex = 0;
  melodyPlaying = true;
  nextNoteTime = 0; // Force immediate start
}

// THE UNIFIED ENGINE: Call this EXACTLY once per frame in loop()
void runAudioEngine() {
  // PRIORITY 1: Handle standalone Sound Effects (Blips, Explosions)
  if (soundActive) {
    if (millis() >= soundEndTime) {
      noTone(SPEAKER_PIN);
      soundActive = false;
      // If a melody was playing underneath, force it to refresh its timer 
      // so it doesn't instantly jump notes when the sound effect ends
      if (melodyPlaying) nextNoteTime = millis(); 
    }
    return; // Block the melody logic while a sound effect is physically playing
  }

  // PRIORITY 2: Handle Melodies if no sound effect is active
  if (melodyPlaying) {
    if (millis() >= nextNoteTime) {
      if (currentNoteIndex >= totalNotes) {
        noTone(SPEAKER_PIN);
        melodyPlaying = false;
        currentNoteIndex = -1;
        return;
      }

      // Grab frequencies and durations via our dynamic pointers
      int currentFreq = currentMelody[currentNoteIndex];
      int currentDuration = currentDurations[currentNoteIndex];

      if (currentFreq == NOTE_REST) {
        noTone(SPEAKER_PIN);
      } else {
        tone(SPEAKER_PIN, currentFreq);
      }

      nextNoteTime = millis() + (currentDuration * 1.30);
      currentNoteIndex++;
    }
  }
}

// --- SELF-CONTAINED RETRO STARBURST EXPLOSION ---
void triggerShipExplosion() {
  int centerX = (int)shipX + (shipWidth / 2);
  int centerY = shipY + (shipHeight / 2);
  
  unsigned int audioPointer = 44;

  // CORE 3.0 FIX: Open the hardware stream channel using the new 3.x API
  sigmaDeltaAttach(SPEAKER_PIN, 312500); 

  // Animate the explosion outward over 6 expanding vector frames
  for (int frame = 1; frame <= 6; frame++) {
    int innerRadius = frame * 3;
    int outerRadius = frame * 7;

    uint16_t primaryColor = (frame % 2 == 0) ? ST7735_WHITE : ST7735_YELLOW;
    uint16_t secondaryColor = (frame % 2 == 0) ? ST7735_YELLOW : ST7735_WHITE;

    // Draw the 8 shrapnel vectors
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

    // Push the graphics frame to the screen
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
    
    // --- INTEGRATED AUDIO PIPELINE ---
    for (int sampleCount = 0; sampleCount < 320; sampleCount++) {
      if (audioPointer < sound_explosion_len) {
        uint8_t audioSample = pgm_read_byte(&explosion_data[audioPointer++]);
        sigmaDeltaWrite(SPEAKER_PIN, audioSample); // CORE 3.0 FIX: Write to the pin directly
      } else {
        sigmaDeltaWrite(SPEAKER_PIN, 128); 
      }
      delayMicroseconds(125); 
    }

    // Clean up our canvas frame buffer vectors
    canvas.drawLine(centerX, centerY - innerRadius, centerX, centerY - outerRadius, ST7735_BLACK);
    canvas.drawLine(centerX, centerY + innerRadius, centerX, centerY + outerRadius, ST7735_BLACK);
    canvas.drawLine(centerX - innerRadius, centerY, centerX - outerRadius, centerY, ST7735_BLACK);
    canvas.drawLine(centerX + innerRadius, centerY, centerX + outerRadius, centerY, ST7735_BLACK);
    canvas.drawLine(centerX - diagInner, centerY - diagInner, centerX - diagOuter, centerY - diagOuter, ST7735_BLACK);
    canvas.drawLine(centerX + diagInner, centerY - diagInner, centerX + diagOuter, centerY - diagOuter, ST7735_BLACK);
    canvas.drawLine(centerX - diagInner, centerY + diagInner, centerX - diagOuter, centerY + diagOuter, ST7735_BLACK);
    canvas.drawLine(centerX + diagInner, centerY + diagInner, centerX + diagOuter, centerY + diagOuter, ST7735_BLACK);
  }

  // CORE 3.0 FIX: Detach the hardware safely using the pin number
  sigmaDeltaWrite(SPEAKER_PIN, 0);
  sigmaDeltaDetach(SPEAKER_PIN);
  
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
  noTone(SPEAKER_PIN); 
}

void loop() {
  // 60 FPS Engine Lock
  static unsigned long lastFrame = 0;
  while (micros() - lastFrame < 16666) { delayMicroseconds(10); }
  lastFrame = micros();
  bool uiNeedsUpdate = false;

  runAudioEngine();

  // --- 1. UPDATE SHIP POSITION ---
  oldShipY = shipY;
  int rawDial = analogRead(DIAL_PIN);
  dialFiltered = (dialFiltered * 0.80) + (rawDial * 0.20); 
  shipY = map((int)dialFiltered, 0, 4095, 12, screenHeight - shipHeight - 1);

  // --- 2. BRUTE-FORCE CANVAS CLEAR ---
  // Instead of clearing individual footprints, we clear the entire gameplay
  // arena (everything below the y=11 UI line) instantly in memory.
  canvas.fillRect(0, 11, screenWidth, screenHeight - 11, ST7735_BLACK);

  // --- 2.5. MANAGE POWER-UP TIMELINE ---
  if (!activePowerUp.active && score >= POWERUP_START_SCORE && millis() >= nextPowerUpSpawnTime) {
    
    // Default rolling assumption: Player gets a shield item drop
    PowerUpType rolledType = SHIELD;
    uint16_t rolledColor = ST7735_CYAN;

    // Roll a 100-sided percentage dice (0 to 99)
    int probabilityDice = random(0, 100);

    // GATE 1: Extra Life only drops if the dice lands on 0-19 (20% chance) 
    // AND the current score milestone is equal to or greater than 1000!
    if (probabilityDice < EXTRA_LIFE_CHANCE) {
      if (score >= MIN_SCORE_FOR_EXTRA_LIFE) {
        rolledType = EXTRA_LIFE;
        rolledColor = ST7735_MAGENTA; // Heart Pink
      } else {
        // BACKUP COMPASSION SYSTEM: If score is under 1000, force it to morph
        // into a Shield instead of wasting the item spawn window entirely!
        rolledType = SHIELD;
        rolledColor = ST7735_CYAN;
      }
    }

    // Launch the power-up item drop onto the gameplay arena
    activePowerUp.active = true;
    activePowerUp.x = screenWidth + 20;
    activePowerUp.y = random(20, screenHeight - 20);
    activePowerUp.vx = -1.2; // Steady, slow retro drift vector
    activePowerUp.type = rolledType;
    activePowerUp.color = rolledColor;
  }

  // Move and render the active power-up container
  if (activePowerUp.active) {
    activePowerUp.x += activePowerUp.vx;
    drawPowerUp(activePowerUp);

    // Despawn safely if it drifts off the left side of the screen edge
    if (activePowerUp.x + activePowerUp.radius < 0) {
      activePowerUp.active = false;
      // Pacing Buffer: Space out subsequent item arrivals by a wider 35 to 60 seconds
      nextPowerUpSpawnTime = millis() + random(POWERUP_RND_SPWN_LOW, POWERUP_RND_SPWN_HIGH);
    }

    // --- CHECK FOR ITEM PICKUP COLLISION ---
    // Forcing precise float casting prevents Arduino's min/max macros from miscalculating boundaries
    float shipLeft   = (float)shipX;
    float shipRight  = (float)(shipX + shipWidth);
    float shipTop    = (float)shipY;
    float shipBottom = (float)(shipY + shipHeight);

    float distSqX = max(shipLeft, min(activePowerUp.x, shipRight));
    float distSqY = max(shipTop,  min(activePowerUp.y, shipBottom));
    
    float deltaX = activePowerUp.x - distSqX;
    float deltaY = activePowerUp.y - distSqY;
    float distSq = (deltaX * deltaX) + (deltaY * deltaY);

    // Explicitly check radius squaring against the distance matrix
    if (distSq < (activePowerUp.radius * activePowerUp.radius)) {
      if (activePowerUp.type == EXTRA_LIFE) {
        lives++;
        uiNeedsUpdate = true;
        playSound(1500, 150); 
      } 
      else if (activePowerUp.type == SHIELD) {
        shieldActive = true;
        shieldEndTime = millis() + SHIELD_DURATION; // Gives you a solid 15 seconds of invulnerability
        playSound(1000, 250); 
      }
      activePowerUp.active = false;
      nextPowerUpSpawnTime = millis() + random(POWERUP_RND_SPWN_LOW, POWERUP_RND_SPWN_HIGH);
    }
  }
  // --- CORE SHIELD RUNTIME & RENDERING ENGINES ---
  if (shieldActive) {
    if (millis() >= shieldEndTime) {
      shieldActive = false;
      playSound(300, 200); // Low-power warning buzz
    } else {
      // FORCE RENDER CALL: This draws the bubble around the ship's current location!
      drawShieldBubble((int)shipX, shipY);
    }
  }
 // --- 3. UPDATE PHYSICS & RENDER HAZARDS ---

  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (!pool[i].active) continue;

    // If it's a comet, ensure its horizontal velocity vector stays boosted 
    // at 150% of current game speed, capping out at a max speed of 6.
    if (pool[i].isComet) {
      float cometSpeed = currentSpeed * 1.5f;
      if (cometSpeed > 6.0f) cometSpeed = 6.0f + (currentSpeed/10);
      pool[i].vx = -cometSpeed; 
    } else {
      // Standard rock velocity tracking
      pool[i].vx = -currentSpeed;
    }

    // Apply velocity vectors to coordinates smoothly
    pool[i].x += pool[i].vx; 
    pool[i].y += pool[i].vy;

    // Apply Rotational Spin
    pool[i].angle += pool[i].spinSpeed;
    if (pool[i].angle >= 360.0f) pool[i].angle -= 360.0f;
    if (pool[i].angle < 0.0f) pool[i].angle += 360.0f;

    // Boundary Physics: Bounce off top UI bar and bottom screen edge
    if (pool[i].y - pool[i].radius < 11) {
      pool[i].y = 11 + pool[i].radius; 
      pool[i].vy = -pool[i].vy;        
    } 
    else if (pool[i].y + pool[i].radius > screenHeight) {
      pool[i].y = screenHeight - pool[i].radius;
      pool[i].vy = -pool[i].vy;        
    }

    // Check if passed off-screen left safely
    if (pool[i].x + pool[i].radius + 12 < 0) {
      if (pool[i].isComet) {
        score += COMET_BONUS_SCORE;
        playSound(1200, 100); 
      } else {
        score += pool[i].sizeClass;
        playSound(800, 30);   
      }
      
      asteroidsPassed++;
      uiNeedsUpdate = true;

      if (asteroidsPassed >= nextTargetScore) {
        if (currentMaxActive < MAX_ASTEROIDS) {
          currentMaxActive++;
          pool[currentMaxActive - 1].active = true;
          resetAsteroid(currentMaxActive - 1, true);
        } else {
          currentSpeed += SPEED_STEP; 
        }
        nextTargetScore += SCORE_TO_SPAWN + (currentMaxActive * 4);
      }

      resetAsteroid(i, false);
      continue;
    }

    // Updated shipX bounds check to include the wing overhang (from shipX - 3 to shipX + shipWidth)
    float closestX = max(shipX, min(pool[i].x, shipX + shipWidth));
    float closestY = max((float)shipY, min(pool[i].y, (float)shipY + shipHeight));
    float distanceSquared = ((pool[i].x - closestX) * (pool[i].x - closestX)) + ((pool[i].y - closestY) * (pool[i].y - closestY));

    // Collision detected!
    if (distanceSquared < ((pool[i].radius * 0.9) * (pool[i].radius * 0.9))) {
      // CRITICAL DIRECTION CORRECTION: Check shield state BEFORE running death math!
      if (shieldActive) {
        // Shield is active! Completely ignore the impact and skip to the next asteroid
        continue; 
      } else {
        // 1. Run our clean explosion function
        triggerShipExplosion();

        lives--;
        delay(300);
        // Single-point flow control
        if (lives <= 0) {

          // Player is truly dead. Go to the final end sequence.
          gameOverSequence(); 
          // Break out of the ENTIRE loop, preventing any chance of mid-game reset
          return; 
        } else {
          // Mid-game recovery (Lives > 0). Perform a clean board wipe.
          tft.fillScreen(ST7735_BLACK);
          drawUI();
          
          // This is where we safely reset the global flag!
          cometOnScreen = false; 

          // Important: Use currentMaxActive, NOT MAX_ASTEROIDS, to ensure pool limits are respected
          for (int j = 0; j < currentMaxActive; j++) {
            pool[j].active = (j < currentMaxActive); // Reinforce active limit
            resetAsteroid(j, true); // Force offscreen start
          }
          runCountdown();
          return;
        }
      }
    }

    // --- RENDERING CORE (NO CLEARING CIRCUITS REQUIRED!) ---
    if (pool[i].isComet) {
      drawCometTeardrop(pool[i], pool[i].color);
    } else {
      drawJaggedAsteroid(pool[i], pool[i].color);
    }
  }

  // --- 4. ANIMATED BITMAP SHIP RENDER ---
  if (millis() >= nextShipFrameTime) {
    shipCurrentFrame = (shipCurrentFrame + 1) % 3; 
    nextShipFrameTime = millis() + SHIP_ANIM_SPEED;
  }
  // Draw the current animation frame directly onto our blank-slate canvas
  canvas.drawRGBBitmap((int)shipX, shipY, ship_frames[shipCurrentFrame], shipWidth, shipHeight);

  if (uiNeedsUpdate) {
    drawUI();
  }
  // --- THE GRAND FINALE ---
  // Push the completely assembled virtual frame to the physical hardware
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
}

void gameOverSequence() {
  tft.fillScreen(ST7735_BLACK);
  
  bool newRecord = false;
  if (score > highScore) {
    highScore = score;
    newRecord = true;
    preferences.putInt("highscore", highScore);
  }

  tft.setCursor(screenWidth / 4 - 5, 15);
  tft.setTextColor(ST7735_RED); 
  tft.setTextSize(2);
  tft.print("GAME OVER");

  //int notes = sizeof(gameOverMelody) / sizeof(gameOverMelody[0]);
  //playMelody(gameOverMelody, gameOverDurations, notes);
  playSample(gameend_data, sound_gameend_len);

  tft.setTextSize(1);
  tft.setCursor(screenWidth / 4, 45);
  tft.setTextColor(ST7735_WHITE);
  tft.print("YOUR SCORE: ");
  tft.setTextColor(ST7735_YELLOW);
  tft.print(score);

  tft.setCursor(screenWidth / 4, 65);
  if (newRecord) {
    tft.setTextColor(ST7735_GREEN);
    tft.print("NEW HIGH SCORE!! ");
  } else {
    tft.setTextColor(0x7BEF); 
    tft.print("HIGH SCORE: ");
    tft.print(highScore);
  }

  tft.setCursor(screenWidth / 4 - 20, 95);
  tft.setTextColor(ST7735_CYAN);
  tft.print("> TWIST DIAL TO RESET <");

  waitForPlayerInput();
  showWelcomeSplashScreen();
  initNewGame();
}

void waitForPlayerInput() {
  int startingValue = analogRead(DIAL_PIN);
  const int threshold = 150; 
  bool waiting = true;
  
  while (waiting) {
    // --- Keep the audio engine marching forward while we wait! ---
    runAudioEngine();
    // Smooth out frame cycles slightly so it doesn't cook the processor
    delay(1);

    int currentValue = analogRead(DIAL_PIN);
    if (abs(currentValue - startingValue) > threshold) {
      waiting = false;
    }
    delay(16); 
  }
  // Cut any lingering note cleanly when moving to the next state
  noTone(SPEAKER_PIN);
}