#ifndef BEEPER_FX_H
#define BEEPER_FX_H

#include <Arduino.h>

namespace BeeperFX {
    void begin(int pin);
    void startup();
    void success();
    void error();
    void r2d2_v1();
    void r2d2_v2();
    void imperialMarchShort();
    void imperialMarchLong();
    void superMarioShort();
    void nokiaTune();
    void tetrisIntro();
    void starTrekBeep();
    void waveSweep();
    void powerUp();
    void airwolfTheme();
    void aTeamTheme();
}

#endif
