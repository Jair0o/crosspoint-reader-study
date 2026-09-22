#include "StudyActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include "I18nKeys.h"
#include "MappedInputManager.h"
#include "PomodoroActivity.h"
#include "TimerActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

StudyActivity::StudyActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("Study", renderer, mappedInput) {}

void StudyActivity::onEnter() {
  UiListActivity::onEnter();

  const char* const labels[ROW_COUNT] = {tr(STR_STUDY_TIMER), tr(STR_STUDY_POMODORO)};
  for (int i = 0; i < ROW_COUNT; ++i) {
    fui::ListItem item;
    item.label = labels[i];
    item.actionValue = static_cast<int16_t>(i);
    rowItems[i] = item;
  }
}

const char* StudyActivity::headerTitle() const { return tr(STR_STUDY); }

void StudyActivity::activateIndex(const int index) {
  app.clearTapFlash();  // the tap leaves this screen either way

  if (index == 0) {  // Timer
    auto timer = makeUniqueNoThrow<TimerActivity>(renderer, mappedInput);
    if (!timer) {
      LOG_ERR("Study", "OOM: timer activity");
      return;
    }
    startActivityForResult(std::move(timer), [](const ActivityResult&) {});
    return;
  }

  // Pomodoro
  auto pomodoro = makeUniqueNoThrow<PomodoroActivity>(renderer, mappedInput);
  if (!pomodoro) {
    LOG_ERR("Study", "OOM: pomodoro activity");
    return;
  }
  startActivityForResult(std::move(pomodoro), [](const ActivityResult&) {});
}

void StudyActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  // Content: the safe area minus the header band GUI.drawHeader paints.
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight),
      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height)), static_cast<int16_t>(safe.x)});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  fui::ListProps props;
  props.items = rowItems;
  props.count = static_cast<uint16_t>(ROW_COUNT);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  syncListViewport(screen, props);
  screen.list(props);
}
