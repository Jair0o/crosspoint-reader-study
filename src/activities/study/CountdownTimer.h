#pragma once

#include <cstdint>

// Countdown timer with no UI code. Tracks elapsed time via millis() deltas
// only, never wall-clock time: the Xteink X4 has no RTC, so an absolute
// clock reading is not meaningfully available.
class CountdownTimer {
 public:
  // Starts (or restarts) counting down from duration. duration == 0 leaves
  // the timer idle, matching the post-reset state.
  void start(uint32_t duration);
  void pause();
  void resume();
  void reset();

  // Time left until the duration passed to start() elapses; 0 once finished.
  uint32_t remainingMs() const;
  // True only while actively counting down (not idle, paused, or finished).
  bool isRunning() const;
  // True once the duration has elapsed; stays true until reset() or a new start().
  bool isFinished() const;

 private:
  enum class State { IDLE, RUNNING, PAUSED };

  // Elapsed time so far, including the current RUNNING span if any.
  uint32_t elapsedMs() const;

  State state = State::IDLE;
  uint32_t durationMs = 0;
  // millis() at the start of the current RUNNING span.
  uint32_t startMillis = 0;
  // Elapsed time banked from RUNNING spans before the current one.
  uint32_t elapsedBeforePauseMs = 0;
};
