#include "PomodoroActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <cstdio>

#include "CountdownDisplay.h"
#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace fui = freeink::ui;

PomodoroActivity::PomodoroActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Pomodoro", renderer, mappedInput), UiAppHost(renderer) {}

void PomodoroActivity::onEnter() {
  Activity::onEnter();
  resetUi();
  app.on(ACTION_CONFIRM, &PomodoroActivity::onConfirmActionTrampoline, this);
  app.on(ACTION_SKIP, &PomodoroActivity::onSkipActionTrampoline, this);
  app.setScreen(&PomodoroActivity::screenTrampoline, this);
  startPhase(Phase::WORK);
  requestUpdate();
}

void PomodoroActivity::screenTrampoline(UiScreen& screen, void* user) {
  static_cast<PomodoroActivity*>(user)->buildScreen(screen);
}

void PomodoroActivity::onConfirmActionTrampoline(const fui::ActionEvent&, void* user) {
  auto* self = static_cast<PomodoroActivity*>(user);
  self->app.clearTapFlash();  // the tap doesn't leave this screen, but a stale flash would still gray the next paint
  self->onConfirm();
}

void PomodoroActivity::onSkipActionTrampoline(const fui::ActionEvent&, void* user) {
  auto* self = static_cast<PomodoroActivity*>(user);
  self->app.clearTapFlash();
  self->skipPhase();
}

int PomodoroActivity::durationMinutes(const Phase phase) const {
  switch (phase) {
    case Phase::WORK:
      return kWorkMinutes;
    case Phase::SHORT_BREAK:
      return kShortBreakMinutes;
    case Phase::LONG_BREAK:
      return kLongBreakMinutes;
  }
  return kWorkMinutes;
}

const char* PomodoroActivity::phaseName(const Phase phase) const {
  switch (phase) {
    case Phase::WORK:
      return tr(STR_STUDY_POMODORO_WORK);
    case Phase::SHORT_BREAK:
      return tr(STR_STUDY_POMODORO_SHORT_BREAK);
    case Phase::LONG_BREAK:
      return tr(STR_STUDY_POMODORO_LONG_BREAK);
  }
  return tr(STR_STUDY_POMODORO_WORK);
}

PomodoroActivity::Phase PomodoroActivity::nextPhaseAfter(const Phase phase) const {
  if (phase != Phase::WORK) return Phase::WORK;
  return (completedWorkSessions % kWorkSessionsBeforeLongBreak == 0) ? Phase::LONG_BREAK : Phase::SHORT_BREAK;
}

const char* PomodoroActivity::waitingMessage() const {
  return pendingPhase == Phase::WORK ? tr(STR_STUDY_POMODORO_BACK_TO_WORK) : tr(STR_STUDY_POMODORO_BREAK_TIME);
}

const char* PomodoroActivity::confirmLabel() const {
  switch (mode) {
    case Mode::WAITING:
      return tr(STR_STUDY_TIMER_START);
    case Mode::RUNNING:
      return tr(STR_STUDY_TIMER_PAUSE);
    case Mode::PAUSED:
      return tr(STR_RESUME);
  }
  return tr(STR_SELECT);
}

void PomodoroActivity::startPhase(const Phase phase) {
  currentPhase = phase;
  timer.start(static_cast<uint32_t>(durationMinutes(phase)) * 60000u);
  mode = Mode::RUNNING;
  lastDisplayBucket = CountdownDisplay::bucket(timer.remainingMs());
}

void PomodoroActivity::endPhase(const bool completed) {
  if (currentPhase == Phase::WORK && completed) ++completedWorkSessions;
  pendingPhase = nextPhaseAfter(currentPhase);
  mode = Mode::WAITING;
  messageRefreshPending = true;
  requestUpdate();
}

void PomodoroActivity::skipPhase() { endPhase(/*completed=*/false); }

void PomodoroActivity::onConfirm() {
  switch (mode) {
    case Mode::WAITING:
      startPhase(pendingPhase);
      break;
    case Mode::RUNNING:
      timer.pause();
      mode = Mode::PAUSED;
      break;
    case Mode::PAUSED:
      timer.resume();
      mode = Mode::RUNNING;
      break;
  }
  requestUpdate();
}

void PomodoroActivity::onBack() {
  if (mode == Mode::RUNNING || mode == Mode::PAUSED) {
    confirmStop();
    return;
  }
  // WAITING: no ticking progress to lose.
  finish();
}

void PomodoroActivity::confirmStop() {
  auto confirmation = makeUniqueNoThrow<ConfirmationActivity>(renderer, mappedInput, tr(STR_STUDY_TIMER_STOP_TITLE),
                                                              tr(STR_STUDY_TIMER_STOP_BODY));
  if (!confirmation) {
    LOG_ERR("Pomodoro", "OOM: stop confirmation");
    return;
  }
  startActivityForResult(std::move(confirmation), [this](const ActivityResult& result) {
    if (result.isCancelled) return;
    timer.reset();
    finish();
  });
}

void PomodoroActivity::checkTimerProgress() {
  if (mode != Mode::RUNNING) return;
  if (timer.isFinished()) {
    endPhase(/*completed=*/true);
    return;
  }
  const int bucket = CountdownDisplay::bucket(timer.remainingMs());
  if (bucket != lastDisplayBucket) {
    lastDisplayBucket = bucket;
    requestUpdate();
  }
}

void PomodoroActivity::loop() {
  // Polled every pass so the final-minute switch to seconds and the phase
  // completion both happen without needing input.
  checkTimerProgress();

  // Touch goes through the FreeInkApp: render() registered the confirm and
  // skip button hit rects.
  const auto route = routeTouch(mappedInput, false);
  if (route.routed && app.invalidated()) requestUpdate();
  if (route) return;

  // Long-press Confirm skips the current phase; wasLongPressed() suppresses
  // the release that follows it, so the short-press check below never
  // double-fires for the same hold.
  if ((mode == Mode::RUNNING || mode == Mode::PAUSED) &&
      mappedInput.wasLongPressed(MappedInputManager::Button::Confirm, kSkipHoldMs)) {
    skipPhase();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onBack();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    onConfirm();
    return;
  }
}

void PomodoroActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  // Content: the safe area minus the title band render() paints.
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing),
      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height)), static_cast<int16_t>(safe.x)});

  if (mode == Mode::WAITING) {
    CountdownDisplay::buildMessageScreen(screen, waitingMessage(), confirmLabel(), ACTION_CONFIRM);
    return;
  }
  buildRunningScreen(screen);
}

void PomodoroActivity::buildRunningScreen(UiScreen& screen) {
  char countText[32];
  snprintf(countText, sizeof(countText), tr(STR_STUDY_POMODORO_COMPLETED_FORMAT),
           static_cast<unsigned>(completedWorkSessions));

  CountdownDisplay::Block block;
  block.captions[block.captionCount++] = phaseName(currentPhase);
  block.captions[block.captionCount++] = countText;
  if (mode == Mode::PAUSED) block.captions[block.captionCount++] = tr(STR_STUDY_TIMER_PAUSED);
  CountdownDisplay::buildRemainingBlock(screen, timer.remainingMs(), block);

  screen.button(confirmLabel(), ACTION_CONFIRM);
  screen.button(tr(STR_STUDY_POMODORO_SKIP), ACTION_SKIP);
}

void PomodoroActivity::render(RenderLock&&) {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 15, tr(STR_STUDY_POMODORO), true, EpdFontFamily::BOLD);

  // Readout/buttons render through the app so they register touch hit rects.
  renderUi();

  auto refreshMode = HalDisplay::FAST_REFRESH;
  if (mode == Mode::WAITING && messageRefreshPending) {
    // Ghost-clean the transition into a full-screen message; only once per
    // message, not on every idle re-render while it's waiting on Confirm.
    refreshMode = HalDisplay::FULL_REFRESH;
    messageRefreshPending = false;
  } else if (mode != Mode::WAITING && ++renderCount % kHalfRefreshEveryNRenders == 0) {
    refreshMode = HalDisplay::HALF_REFRESH;
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel(), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(refreshMode);
}
