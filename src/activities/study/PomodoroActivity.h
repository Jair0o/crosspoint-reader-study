#pragma once

#include <GfxRenderer.h>

#include "CountdownTimer.h"
#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "components/UiAppHost.h"

// Pomodoro screen: Work / Short Break / Long Break phases cycling
// automatically, each pausing on a full-screen message for Confirm before the
// next one starts. A custom FUI layout (not a list), sharing CountdownTimer
// and the countdown readout rendering with TimerActivity via
// CountdownDisplay.
class PomodoroActivity final : public Activity, private UiAppHost {
 public:
  explicit PomodoroActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  // A pomodoro session should never let the device sleep, including while a
  // phase-end message is waiting on Confirm.
  bool preventAutoSleep() override { return true; }

 private:
  enum class Phase { WORK, SHORT_BREAK, LONG_BREAK };
  // RUNNING/PAUSED: currentPhase is counting down. WAITING: currentPhase just
  // ended and pendingPhase is queued to start on Confirm.
  enum class Mode { RUNNING, PAUSED, WAITING };

  static constexpr freeink::ui::ActionId ACTION_CONFIRM = 1;
  static constexpr freeink::ui::ActionId ACTION_SKIP = 2;

  // Phase durations and the long-break cadence, kept together so a future
  // settings screen only has to change values here.
  static constexpr int kWorkMinutes = 25;
  static constexpr int kShortBreakMinutes = 5;
  static constexpr int kLongBreakMinutes = 15;
  static constexpr int kWorkSessionsBeforeLongBreak = 4;

  // Hold threshold for the Confirm-button skip gesture.
  static constexpr unsigned long kSkipHoldMs = 700;
  // How many renders between ghost-cleanup passes while a phase is running.
  static constexpr int kHalfRefreshEveryNRenders = 5;

  static void screenTrampoline(UiScreen& screen, void* user);
  static void onConfirmActionTrampoline(const freeink::ui::ActionEvent& event, void* user);
  static void onSkipActionTrampoline(const freeink::ui::ActionEvent& event, void* user);

  void buildScreen(UiScreen& screen);
  void buildRunningScreen(UiScreen& screen);
  const char* confirmLabel() const;
  const char* phaseName(Phase phase) const;
  const char* waitingMessage() const;
  int durationMinutes(Phase phase) const;
  Phase nextPhaseAfter(Phase phase) const;

  void startPhase(Phase phase);
  // completed is false for a skip: the phase advances but is not credited
  // toward the pomodoro count.
  void endPhase(bool completed);
  void onConfirm();
  void onBack();
  void confirmStop();
  void skipPhase();
  // Polls the timer for completion and for CountdownDisplay::bucket changes;
  // called every loop() pass so both are caught without needing button or
  // touch input.
  void checkTimerProgress();

  Phase currentPhase = Phase::WORK;
  Phase pendingPhase = Phase::WORK;
  Mode mode = Mode::RUNNING;
  int completedWorkSessions = 0;
  CountdownTimer timer;
  int lastDisplayBucket = 0;
  int renderCount = 0;
  // Set on entering WAITING; consumed (as one FULL_REFRESH) by the next render.
  bool messageRefreshPending = false;
};
