#include "CountdownTimer.h"

#include <Arduino.h>

void CountdownTimer::start(const uint32_t duration) {
  durationMs = duration;
  elapsedBeforePauseMs = 0;
  startMillis = millis();
  state = duration == 0 ? State::IDLE : State::RUNNING;
}

void CountdownTimer::pause() {
  if (state != State::RUNNING) return;
  elapsedBeforePauseMs += static_cast<uint32_t>(millis() - startMillis);
  state = State::PAUSED;
}

void CountdownTimer::resume() {
  if (state != State::PAUSED) return;
  startMillis = millis();
  state = State::RUNNING;
}

void CountdownTimer::reset() {
  state = State::IDLE;
  durationMs = 0;
  elapsedBeforePauseMs = 0;
  startMillis = 0;
}

uint32_t CountdownTimer::elapsedMs() const {
  if (state == State::IDLE) return 0;
  uint32_t elapsed = elapsedBeforePauseMs;
  if (state == State::RUNNING) {
    // Unsigned subtraction wraps correctly across a millis() rollover
    // (~49.7 days), well beyond any realistic session length.
    elapsed += static_cast<uint32_t>(millis() - startMillis);
  }
  return elapsed;
}

uint32_t CountdownTimer::remainingMs() const {
  const uint32_t elapsed = elapsedMs();
  return elapsed >= durationMs ? 0 : durationMs - elapsed;
}

bool CountdownTimer::isFinished() const { return state != State::IDLE && remainingMs() == 0; }

bool CountdownTimer::isRunning() const { return state == State::RUNNING && !isFinished(); }
