#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include "GameConfig.h"

// Note Frequencies
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
#define NOTE_REST 0

class AudioEngine {
private:
    const int* _currentMelody;
    const int* _currentDurations;
    int _totalNotes;
    int _currentNoteIndex;
    unsigned long _nextNoteTime;
    bool _melodyPlaying;

    unsigned long _soundEndTime;
    bool _soundActive;

public:
    AudioEngine() : _currentMelody(nullptr), _currentDurations(nullptr), _totalNotes(0), 
                    _currentNoteIndex(-1), _nextNoteTime(0), _melodyPlaying(false),
                    _soundEndTime(0), _soundActive(false) {}

    void playSound(int frequency, int durationMs) {
        tone(GameConfig::PIN_SPEAKER, frequency);
        _soundEndTime = millis() + durationMs;
        _soundActive = true;
    }

    void playMelody(const int* melodyArray, const int* durationArray, int noteCount) {
        _currentMelody = melodyArray;
        _currentDurations = durationArray;
        _totalNotes = noteCount;
        _currentNoteIndex = 0;
        _melodyPlaying = true;
        _nextNoteTime = 0; 
    }

    void update() {
        if (_soundActive) {
            if (millis() >= _soundEndTime) {
                noTone(GameConfig::PIN_SPEAKER);
                _soundActive = false;
                if (_melodyPlaying) _nextNoteTime = millis();
            }
            return; 
        }

        if (_melodyPlaying) {
            if (millis() >= _nextNoteTime) {
                if (_currentNoteIndex >= _totalNotes) {
                    noTone(GameConfig::PIN_SPEAKER);
                    _melodyPlaying = false;
                    _currentNoteIndex = -1;
                    return;
                }

                int currentFreq = _currentMelody[_currentNoteIndex];
                int currentDuration = _currentDurations[_currentNoteIndex];

                if (currentFreq == NOTE_REST) {
                    noTone(GameConfig::PIN_SPEAKER);
                } else {
                    tone(GameConfig::PIN_SPEAKER, currentFreq);
                }

                _nextNoteTime = millis() + (currentDuration * 1.30);
                _currentNoteIndex++;
            }
        }
    }

    void playSample(const unsigned char* sound_data, unsigned int sound_data_len) {
        sigmaDeltaAttach(GameConfig::PIN_SPEAKER, 312500);
        sigmaDeltaWrite(GameConfig::PIN_SPEAKER, 128); 

        for (unsigned int i = 44; i < sound_data_len; i++) { // Auto-skips header block safely
            uint8_t audioSample = pgm_read_byte(&sound_data[i]);
            sigmaDeltaWrite(GameConfig::PIN_SPEAKER, audioSample);
            delayMicroseconds(125); 
        }
        
        sigmaDeltaWrite(GameConfig::PIN_SPEAKER, 0);
        sigmaDeltaDetach(GameConfig::PIN_SPEAKER);
        
        pinMode(GameConfig::PIN_SPEAKER, OUTPUT);
        digitalWrite(GameConfig::PIN_SPEAKER, LOW);
        noTone(GameConfig::PIN_SPEAKER); 
    }

    void mute() {
        noTone(GameConfig::PIN_SPEAKER);
    }
};

#endif