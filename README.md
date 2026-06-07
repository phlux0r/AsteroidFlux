# Asteroid Flux 🚀

A fast-paced, retro-inspired space survival arcade game written in C++ for the **ESP32-S3** microcontroller. Dodge procedurally generated jagged asteroids, hunt down high-velocity comets, and capture dynamic drifting power-ups using an analog rotary dial controller.

The project is built entirely on modern **Object-Oriented Programming (OOP)** principles, separating hardware abstraction, game state engines, and physics layers into clean, modular components.

---

## 🎮 Gameplay Features

* **Parallax Starfield:** A multi-layered background scrolling architecture that provides an immersive sense of deep-space depth.
* **Procedural Hazards:** Asteroids are dynamically allocated from an object pool with randomized sizes, speeds, jagged geometries, and spin rotations.
* **High-Velocity Comets:** Rare, fast-moving comets that challenge your reaction times but yield massive score rewards.
* **Analog Precision Control:** Built to map native analog dial inputs through an exponential software filter for butter-smooth ship maneuvering.
* **Dynamic Power-Up System:** Drops float through space using realistic 2D drift vectors and bounce off UI boundaries.
    * 🛡️ **Shield (Cyan Rounded Box):** Grants a temporary 10-second defensive energy bubble.
    * ❤️ **Extra Life (Magenta Heart):** Rewards an extra ship stock (unlocks at a 600+ score tier).
    * ⏱️ **Slow Speed (Green Clock):** Clocks back the surging global game speed by 3 full difficulty steps to give you breathing room.
* **Dual-Engine Audio System:** Features background melody parsing paired with direct-memory execution loops that play retro PCM wave audio sequences for explosions and title cues without blocking gameplay frame rates.
* **Non-Volatile Storage (NVS):** Tracks and saves your lifetime **High Score Record** directly to the ESP32-S3's internal flash memory, persisting even after power loss.

---

## 🛠️ Hardware Requirements

| Component | Specification / Description |
| :--- | :--- |
| **Microcontroller** | ESP32-S3 (e.g., Seeed Studio Grove Vision AI V2, Dev Modules, or similar) |
| **Display** | ST7735 1.8" TFT SPI Color Display ($160 \times 128$ resolution) |
| **Input** | Standard Rotary Potentiometer / Analog Dial |
| **Audio Output** | Passive Buzzer or Speaker Pin |

### Pin Configuration Dashboard

All hardware interfaces are centralized within `GameConfig.h` for rapid tweaking:

* **TFT Display:** `CS` $\rightarrow$ Pin 10 | `DC` $\rightarrow$ Pin 8 | `RST` $\rightarrow$ Pin 9 | `Backlight (BLK)` $\rightarrow$ Pin 7
* **Analog Input (Dial):** Pin 1 (Analog In)
* **Audio (Speaker):** Pin 5 (Sigma-Delta/PWM capable)

---

## 📁 Repository Structure

The project codebase is neatly separated to eliminate global variable clutter and simplify maintenance:

```text
AsteroidFlux/
├── AsteroidFlux_v1_5.ino # Main game loop orchestration & state coordinator
├── GameConfig.h          # Centralized configuration (difficulty, constants, pins)
├── AudioEngine.h         # Tone melody sequencer & raw 8-bit PCM wave sound engine
├── PlayerShip.h          # Input smoothing, tracking, animations, and shield handling
├── AsteroidManager.h     # Dynamic object-pooling, asteroid geometries, and collision math
├── PowerUpManager.h      # Random spawn engine, 2D drift physics, and pickup triggers
├── BackgroundStars.h     # Parallax background scrolling system
├── splash_image.h        # Hexadecimal full-screen title bitmap data
├── explosion.h           # Raw 8-bit PCM audio array for ship destruction
├── gamestart.h           # Raw 8-bit PCM audio array for introduction cues
└── gameend.h             # Raw 8-bit PCM audio array for game over sequences
