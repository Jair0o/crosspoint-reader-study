#include "TimerActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <algorithm>
#include <cstdio>

#include "CountdownDisplay.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace fui = freeink::ui;

TimerActivity::TimerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Timer", renderer, mappedInput), UiAppHost(renderer) {}

void TimerActivity::onEnter() {
  Activity::onEnter();
  resetUi();
  app.on(ACTION_CONFIRM, &TimerActivity::onConfirmActionTrampoline, this);
  app.on(ACTION_STEP, &TimerActivity::onStepActionTrampoline, this);
  app.setScreen(&TimerActivity::screenTrampoline, this);
  requestUpdate();
}

void TimerActivity::screenTrampoline(UiScreen& screen, void* user) {
  static_cast<TimerActivity*>(user)->buildScreen(screen);
}

void TimerActivity::onConfirmActionTrampoline(const fui::ActionEvent&, void* user) {
  auto* self = static_cast<TimerActivity*>(user);
  self->app.clearTapFlash();  // the tap may leave this screen (FINISHED dismiss)
  self->onConfirm();
}

void TimerActivity::onStepActionTrampoline(const fui::ActionEvent& event, void* user) {
  static_cast<TimerActivity*>(user)->adjustDuration(event.value);
}

void TimerActivity::adjustDuration(const int deltaMinutes) {
  if (phase != Phase::SETTING) return;
  const int clamped = std::clamp(durationMinutes + deltaMinutes, kMinDurationMinutes, kMaxDurationMinutes);
  if (clamped == durationMinutes) return;
  durationMinutes = clamped;
  requestUpdate();
}

void TimerActivity::onConfirm() {
  switch (phase) {
    case Phase::SETTING:
      timer.start(static_cast<uint32_t>(durationMinutes) * 60000u);
      phase = Phase::RUNNING;
      lastDisplayBucket = CountdownDisplay::bucket(timer.remainingMs());
      break;
    case Phase::RUNNING:
      timer.pause();
      phase = Phase::PAUSED;
      break;
    case Phase::PAUSED:
      timer.resume();
      phase = Phase::RUNNING;
      lastDisplayBucket = CountdownDisplay::bucket(timer.remainingMs());
      break;
    case Phase::FINISHED:
      finish();
      return;
  }
  requestUpdate();
}

void TimerActivity::onBack() {
  if (phase == Phase::RUNNING || phase == Phase::PAUSED) {
    confirmStop();
    return;
  }
  // SETTING has no progress to lose; FINISHED is a dead end either button dismisses.
  finish();
}

void TimerActivity::confirmStop() {
  auto confirmation = makeUniqueNoThrow<ConfirmationActivity>(renderer, mappedInput, tr(STR_STUDY_TIMER_STOP_TITLE),
                                                              tr(STR_STUDY_TIMER_STOP_BODY));
  if (!confirmation) {
    LOG_ERR("Timer", "OOM: stop confirmation");
    return;
  }
  startActivityForResult(std::move(confirmation), [this](const ActivityResult& result) {
    if (result.isCancelled) return;
    timer.reset();
    finish();
  });
}

void TimerActivity::checkTimerProgress() {
  if (phase != Phase::RUNNING) return;
  if (timer.isFinished()) {
    phase = Phase::FINISHED;
    requestUpdate();
    return;
  }
  const int bucket = CountdownDisplay::bucket(timer.remainingMs());
  if (bucket != lastDisplayBucket) {
    lastDisplayBucket = bucket;
    requestUpdate();
  }
}

void TimerActivity::loop() {
  // Polled every pass so the final-minute switch to seconds and the
  // RUNNING -> FINISHED transition both happen without needing input.
  checkTimerProgress();

  // Touch goes through the FreeInkApp: render() registered the stepper and
  // confirm-button hit rects.
  const auto route = routeTouch(mappedInput, false);
  if (route.routed && app.invalidated()) requestUpdate();
  if (route) return;

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onBack();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    onConfirm();
    return;
  }

  if (phase == Phase::SETTING) {
    buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up}, [this] { adjustDuration(kStepMinutes); });
    buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down}, [this] { adjustDuration(-kStepMinutes); });
  }
}

const char* TimerActivity::confirmLabel() const {
  switch (phase) {
    case Phase::SETTING:
      return tr(STR_STUDY_TIMER_START);
    case Phase::RUNNING:
      return tr(STR_STUDY_TIMER_PAUSE);
    case Phase::PAUSED:
      return tr(STR_RESUME);
    case Phase::FINISHED:
      return tr(STR_SELECT);
  }
  return tr(STR_SELECT);
}

void TimerActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  // Content: the safe area minus the title band render() paints.
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing),
      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height)), static_cast<int16_t>(safe.x)});

  switch (phase) {
    case Phase::SETTING:
      buildSettingScreen(screen);
      break;
    case Phase::RUNNING:
    case Phase::PAUSED:
      buildRunningScreen(screen);
      break;
    case Phase::FINISHED:
      buildFinishedScreen(screen);
      break;
  }
}

void TimerActivity::buildSettingScreen(UiScreen& screen) {
  const auto& theme = screen.theme();
  screen.spacer(theme.spaceLg);

  char value[16];
  snprintf(value, sizeof(value), tr(STR_SLEEP_TIMER_VALUE_FORMAT), static_cast<unsigned>(durationMinutes));

  fui::StepperRowProps stepper;
  stepper.row.label = tr(STR_STUDY_TIMER_DURATION);
  stepper.value = value;
  stepper.widestValue = "120 min";
  stepper.decrement = ACTION_STEP;
  stepper.increment = ACTION_STEP;
  stepper.decrementValue = static_cast<int16_t>(-kStepMinutes);
  stepper.incrementValue = static_cast<int16_t>(kStepMinutes);
  screen.stepperRow(stepper);

  screen.spacer(static_cast<int16_t>(theme.spaceLg * 2));
  screen.button(confirmLabel(), ACTION_CONFIRM);
}

void TimerActivity::buildRunningScreen(UiScreen& screen) {
  CountdownDisplay::Block block;
  if (phase == Phase::PAUSED) block.captions[block.captionCount++] = tr(STR_STUDY_TIMER_PAUSED);
  CountdownDisplay::buildRemainingBlock(screen, timer.remainingMs(), block);
  screen.button(confirmLabel(), ACTION_CONFIRM);
}

void TimerActivity::buildFinishedScreen(UiScreen& screen) {
  CountdownDisplay::buildMessageScreen(screen, tr(STR_STUDY_TIMER_DONE), confirmLabel(), ACTION_CONFIRM);
}

void TimerActivity::render(RenderLock&&) {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 15, tr(STR_STUDY_TIMER), true, EpdFontFamily::BOLD);

  // Stepper/readout/button render through the app so they register touch hit rects.
  renderUi();

  auto mode = HalDisplay::FAST_REFRESH;
  if (phase == Phase::FINISHED) {
    if (!finishedRefreshDone) {
      mode = HalDisplay::FULL_REFRESH;
      finishedRefreshDone = true;
    }
  } else if (++renderCount % kHalfRefreshEveryNRenders == 0) {
    // Periodic ghost cleanup: renders are roughly once a minute while
    // running, so this lands every few minutes without a dedicated timer.
    mode = HalDisplay::HALF_REFRESH;
  }

  const char* confirm = confirmLabel();
  const auto labels = phase == Phase::SETTING ? mappedInput.mapLabels(tr(STR_BACK), confirm, "-", "+")
                                              : mappedInput.mapLabels(tr(STR_BACK), confirm, "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(mode);
}
