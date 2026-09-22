#pragma once

#include <GfxRenderer.h>

#include "activities/UiListActivity.h"

class MappedInputManager;

// Study menu: Timer and Pomodoro entry points.
class StudyActivity final : public UiListActivity {
 public:
  explicit StudyActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;

 private:
  static constexpr int ROW_COUNT = 2;

  int listCount() const override { return ROW_COUNT; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;

  // Labels are static, so the rows are built once in onEnter() and reused by every buildScreen().
  freeink::ui::ListItem rowItems[ROW_COUNT]{};
};
