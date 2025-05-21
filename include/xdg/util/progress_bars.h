#ifndef _XDG_PROGRESS_BAR_H
#define _XDG_PROGRESS_BAR_H

#include <string>

#include <indicators/block_progress_bar.hpp>

using namespace indicators;

auto block_progress_bar(const std::string& description) {
  return BlockProgressBar(
    option::BarWidth{50},
    option::Start{"["},
    option::End{"]"},
    option::PostfixText{description},
    option::ForegroundColor{Color::green},
    option::ShowPercentage{true},
    option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
  );
}

#endif