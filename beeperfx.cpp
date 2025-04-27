#include "beeperfx.h"

namespace {
    int beeperPin = -1;

    void beep(int freq, int dur) {
        tone(beeperPin, freq);
        delay(dur);
        noTone(beeperPin);
        delay(20);
    }
}

void BeeperFX::begin(int pin) {
    beeperPin = pin;
    pinMode(pin, OUTPUT);
}

void BeeperFX::startup() {
    beep(440, 100);
    beep(660, 100);
    beep(880, 100);
}

void BeeperFX::success() {
    beep(1000, 80);
    beep(1500, 80);
}

void BeeperFX::error() {
    beep(400, 200);
}

void BeeperFX::r2d2_v1() {
    for (int i = 0; i < 5; i++) {
        int freq = random(800, 2000);
        int dur = random(30, 120);
        beep(freq, dur);
    }
}

void BeeperFX::r2d2_v2() {
  for (int i = 0; i < 20; i++) {
    int freq = random(600, 3000);
    int dur = random(50, 150);
    tone(beeperPin, freq, dur);
    delay(dur + 20);
  }
}

void BeeperFX::imperialMarchShort() {
    beep(440, 300);
    beep(440, 300);
    beep(440, 300);
    beep(349, 200);
    beep(523, 100);
    beep(440, 300);
    beep(349, 200);
    beep(523, 100);
    beep(440, 500);
}

void BeeperFX::imperialMarchLong() {
  int melody[] = {
    440, 440, 440, 349, 523, 440, 349, 523, 440,
    659, 659, 659, 698, 523, 415, 349, 523, 440,
    880, 440, 440, 880, 830, 784, 740, 698, 659, 622, 659,
    349, 415, 523, 440, 349, 523, 440
  };

  int durations[] = {
    500, 500, 500, 350, 150, 500, 350, 150, 1000,
    500, 500, 500, 350, 150, 500, 350, 150, 1000,
    250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 500,
    250, 250, 500, 350, 150, 1000, 500
  };

  int notes = sizeof(melody) / sizeof(melody[0]);

  for (int i = 0; i < notes; i++) {
    tone(beeperPin, melody[i], durations[i]);
    delay(durations[i] * 1.3);
  }
  noTone(beeperPin);
}

void BeeperFX::superMarioShort() {
  int melody[] = { 660, 660, 0, 660, 0, 520, 660, 0, 770 };
  int duration[] = { 100, 100, 100, 100, 100, 100, 100, 100, 100 };
  for (int i = 0; i < 9; i++) {
    if (melody[i] == 0) {
      noTone(beeperPin);
    } else {
      tone(beeperPin, melody[i], duration[i]);
    }
    delay(duration[i] * 1.3);
  }
}

void BeeperFX::nokiaTune() {
  int melody[] = { 659, 587, 523, 587, 659, 659, 659, 587, 587, 587, 659, 784 };
  int duration[] = { 150, 150, 150, 150, 150, 150, 300, 150, 150, 300, 150, 300 };
  for (int i = 0; i < 12; i++) {
    tone(beeperPin, melody[i], duration[i]);
    delay(duration[i] * 1.3);
  }
  noTone(beeperPin);
}

void BeeperFX::tetrisIntro() {
  int melody[] = {
    659, 494, 523, 587, 523, 494, 440, 440, 523, 659, 587, 523, 494
  };
  int duration[] = {
    300, 150, 150, 300, 150, 150, 300, 150, 150, 300, 150, 150, 300
  };
  for (int i = 0; i < 13; i++) {
    tone(beeperPin, melody[i], duration[i]);
    delay(duration[i] * 1.3);
  }
}

void BeeperFX::starTrekBeep() {
  tone(beeperPin, 1000, 150);
  delay(150);
  tone(beeperPin, 1500, 150);
  delay(150);
  tone(beeperPin, 2000, 300);
  delay(350);
  noTone(beeperPin);
}

void BeeperFX::waveSweep() {
  for (int f = 300; f < 1200; f += 50) {
    tone(beeperPin, f, 20);
    delay(20);
  }
  for (int f = 1200; f > 300; f -= 50) {
    tone(beeperPin, f, 20);
    delay(20);
  }
  noTone(beeperPin);
}

void BeeperFX::powerUp() {
  for (int i = 200; i < 1000; i += 20) {
    tone(beeperPin, i);
    delay(10);
  }
  noTone(beeperPin);
}

void BeeperFX::airwolfTheme() {
  int melody[] = {
    988, 988, 784, 988, 1318, 988, 784, 988, 784
  };
  int durations[] = {
    200, 200, 200, 200, 300, 200, 200, 200, 400
  };

  for (int i = 0; i < 9; i++) {
    tone(beeperPin, melody[i], durations[i]);
    delay(durations[i] * 1.3);
  }
  noTone(beeperPin);
}

void BeeperFX::aTeamTheme() {
  int melody[] = {
    880, 880, 880, 698, 880, 988, 880, 784
  };
  int durations[] = {
    150, 150, 300, 150, 150, 300, 150, 300
  };

  for (int i = 0; i < 8; i++) {
    tone(beeperPin, melody[i], durations[i]);
    delay(durations[i] * 1.3);
  }
  noTone(beeperPin);
}
