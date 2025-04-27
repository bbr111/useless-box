#include "speed-servo.h"

const int SLOW_MOVE_DELAY = 2;
const int FAST_MOVE_DELAY = 15;

void SpeedServo::attach(uint8_t pin) {
  _servo.attach(pin, 500, 2400);
}

// Valid position: 0-180.
void SpeedServo::moveNowTo(int newPosition) {
  _lastPosition = newPosition;
  _servo.write(newPosition);
}

// Valid position: 0-180.
void SpeedServo::moveFastTo(int newPosition) {
  _moveTo(newPosition, FAST_MOVE_DELAY);
}

// Valid position: 0-180.
void SpeedServo::moveSlowTo(int newPosition) {
  _moveTo(newPosition, SLOW_MOVE_DELAY);
}

// Valid position: 0-180.
void SpeedServo::_moveTo(int newPosition, unsigned long stepDelay) {
  int stepSize = stepDelay;
  if (newPosition > _lastPosition) {
    for (int pos = _lastPosition; pos <= newPosition; pos += stepSize) {
      _servo.write(pos);
      //delay(stepDelay);
    }
  } else {
    for (int pos = _lastPosition; pos >= newPosition; pos -= stepSize) {
      _servo.write(pos);
      //delay(stepDelay);
    }
  }
  _lastPosition = newPosition;
}
