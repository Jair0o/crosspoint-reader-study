#pragma once

#include <GfxRenderer.h>

#include "CountdownTimer.h"
#include "MappedInputManager.h"
#include "activities/Activity.h"
#include "components/UiAppHost.h"
#include "util/ButtonNavigator.h"

// Countdown timer screen: adjustable duration before starting, then a large
// remaining-time readout. A custom FUI layout (not a single list), per
// docs/contributing/touch-and-ui.md.
class TimerActivity final : public Activity, private UiAppHost {
 public:
  explicit TimerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  // RUNNING and PAUSED both have progress worth protecting; only SETTING (nothing
  // started yet) and FINISHED (a dead end) allow the device to sleep.
  bool preventAutoSleep() override { return phase == Phase::RUNNING || phase == Phase::PAUSED; }

 private:
  enum class Phase { SETTING, RUNNING, PAUSED, FINISHED };

  static constexpr freeink::ui::ActionId ACTION_CONFIRM = 1;
  static constexpr freeink::ui::ActionId ACTION_STEP = 2;
  static constexpr int kMinDurationMinutes = 5;
  static constexpr int kMaxDurationMinutes = 120;
  static constexpr int kDefaultDurationMinutes = 25;
  static constexpr int kStepMinutes = 5;
  // How many renders between ghost-cleanup passes (roughly every 5 displayed
  // minutes, since a render happens about once per minute while running).
  static constexpr int kHalfRefreshEveryNRenders = 5;

  static void screenTrampoline(UiScreen& screen, void* user);
  static void onConfirmActionTrampoline(const freeink::ui::ActionEvent& event, void* user);
  static void onStepActionTrampoline(const freeink::ui::ActionEvent& event, void* user);

  void buildScreen(UiScreen& screen);
  void buildSettingScreen(UiScreen& screen);
  void buildRunningScreen(UiScreen& screen);
  void buildFinishedScreen(UiScreen& screen);
  const char* confirmLabel() const;

  void adjustDuration(int deltaMinutes);
  void onConfirm();
  void onBack();
  void confirmStop();
  // Polls the timer for completion and for CountdownDisplay::bucket changes;
  // called every loop() pass so both are caught without needing button or
  // touch input.
  void checkTimerProgress();

  Phase phase = Phase::SETTING;
  int durationMinutes = kDefaultDurationMinutes;
  CountdownTimer timer;
  ButtonNavigator buttonNavigator;
  int lastDisplayBucket = 0;
  int renderCount = 0;
  // The FINISHED screen gets one FULL_REFRESH on first render only.
  bool finishedRefreshDone = false;
};
