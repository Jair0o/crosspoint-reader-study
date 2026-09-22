#pragma once

#include <cstdint>

#include "components/UiAppHost.h"

// Shared rendering for a countdown-style screen: the large remaining-time
// readout and the full-screen message shown between phases. Used by
// TimerActivity and PomodoroActivity. No timing logic lives here — callers
// own a CountdownTimer and pass its remainingMs().
namespace CountdownDisplay {

// Redraw-cadence bucket: minutes while more than a minute remains, seconds
// once inside the final minute. Callers redraw only when this value changes,
// giving once-a-minute updates until the final minute and once-a-second
// within it. Also the value shown in the big readout.
int bucket(uint32_t remainingMs);

// Small centered caption lines stacked above the big readout (phase name, a
// completed-count line, a "Paused" indicator, ...). Unused entries stay null.
struct Block {
  static constexpr int kMaxCaptions = 3;
  const char* captions[kMaxCaptions] = {};
  int captionCount = 0;
};

// Big centered remaining-time readout (minutes, or seconds in the final
// minute) with a unit caption below it, and block.captions stacked above.
void buildRemainingBlock(UiAppHost::UiScreen& screen, uint32_t remainingMs, const Block& block);

// Full-screen centered message with a confirm button below it, for a
// terminal or between-phases screen ("Time's Up", "Break time", ...).
void buildMessageScreen(UiAppHost::UiScreen& screen, const char* message, const char* confirmLabel,
                        freeink::ui::ActionId confirmAction);

}  // namespace CountdownDisplay
