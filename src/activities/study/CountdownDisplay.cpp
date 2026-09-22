#include "CountdownDisplay.h"

#include <I18n.h>

#include <cstdio>

#include "I18nKeys.h"

namespace fui = freeink::ui;

namespace CountdownDisplay {

int bucket(const uint32_t remainingMs) {
  return remainingMs > 60000 ? static_cast<int>((remainingMs + 59999) / 60000)
                             : static_cast<int>((remainingMs + 999) / 1000);
}

void buildRemainingBlock(UiAppHost::UiScreen& screen, const uint32_t remainingMs, const Block& block) {
  const auto& theme = screen.theme();
  const bool finalMinute = remainingMs <= 60000;

  char big[8];
  snprintf(big, sizeof(big), "%u", static_cast<unsigned>(bucket(remainingMs)));
  const char* unitCaption = finalMinute ? tr(STR_STUDY_TIMER_SECONDS_REMAINING) : tr(STR_STUDY_TIMER_MINUTES_REMAINING);

  fui::TextStyle bigStyle = theme.titleText;
  bigStyle.align = fui::TextAlign::Center;
  bigStyle.bold = true;
  const int16_t bigLh = screen.target().lineHeight(bigStyle.font);

  fui::TextStyle smallStyle = theme.smallText;
  smallStyle.align = fui::TextAlign::Center;
  const int16_t smallLh = screen.target().lineHeight(smallStyle.font);

  const int16_t captionsH = static_cast<int16_t>(block.captionCount * (smallLh + theme.spaceSm));
  const int16_t blockH = static_cast<int16_t>(captionsH + bigLh + theme.spaceMd + smallLh);
  const fui::Rect body = screen.body();
  if (body.height > blockH) screen.spacer(static_cast<int16_t>((body.height - blockH) / 2));

  for (int i = 0; i < block.captionCount; ++i) {
    screen.target().text(screen.takeTop(smallLh, theme.spaceSm), block.captions[i], smallStyle);
  }
  screen.target().text(screen.takeTop(bigLh, theme.spaceMd), big, bigStyle);
  screen.target().text(screen.takeTop(smallLh, theme.spaceLg), unitCaption, smallStyle);
}

void buildMessageScreen(UiAppHost::UiScreen& screen, const char* message, const char* confirmLabel,
                        const fui::ActionId confirmAction) {
  const auto& theme = screen.theme();
  fui::TextStyle style = theme.titleText;
  style.align = fui::TextAlign::Center;
  style.bold = true;
  style.maxLines = 2;
  const int16_t lh = screen.target().lineHeight(style.font);
  const int16_t blockH = static_cast<int16_t>(lh * 2);

  const fui::Rect body = screen.body();
  if (body.height > blockH) screen.spacer(static_cast<int16_t>((body.height - blockH) / 2));

  screen.target().text(screen.takeTop(blockH, static_cast<int16_t>(theme.spaceLg * 2)), message, style);
  screen.button(confirmLabel, confirmAction);
}

}  // namespace CountdownDisplay
