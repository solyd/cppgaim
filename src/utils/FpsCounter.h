#pragma once

#include <cstdint>
#include <vector>
#include <numeric>
#include <algorithm>

namespace cppgaim {

class FpsCounter {
public:
  // TODO(solyd): use a circular buffer instead, would make operations easier
  using TimeMs = uint32_t;
  using SecondsWindow = std::vector<uint32_t>;

  explicit FpsCounter(SecondsWindow::size_type window_size_seconds)
    : seconds_window_(window_size_seconds, 0)
    , last_frame_start_(0)
  {}

  static double ms_per_frame_for(double desired_fps) {
    return 1000.0 / desired_fps;
  }

public:
  void mark_frame_start(TimeMs frame_start_time);

  TimeMs last_frame_start_time() { return last_frame_start_; }

  double avg(SecondsWindow::size_type over_x_seconds = 1) {
    assert(over_x_seconds >= 1);
    assert(over_x_seconds < seconds_window_.size());

    over_x_seconds = std::max(
      static_cast<decltype(over_x_seconds)>(1),
      std::min(over_x_seconds, seconds_window_.size())
    );

    bool need_to_wrap = active_second_index_ + over_x_seconds > seconds_window_.size();
    auto active_it = std::next(seconds_window_.cbegin(), active_second_index_);

    auto sum = std::accumulate(
      active_it,
      need_to_wrap ? seconds_window_.cend() : std::next(active_it, over_x_seconds - 1),
      static_cast<SecondsWindow::value_type>(0)
    );

    if (need_to_wrap) {
      sum = std::accumulate(seconds_window_.cbegin(), std::prev(active_it, 1), sum);
    }

    return static_cast<double>(sum) / static_cast<double>(over_x_seconds);
  }

private:
  void update_window_index(TimeMs frame_start_time) {
    auto last_active_index = active_second_index_;
    active_second_index_ = (frame_start_time / 1000) % seconds_window_.size();

    if (active_second_index_ == last_active_index) {
      // no need to reset any counters
      return;
    }

    // reset all counters between the old index and current one
    if (last_active_index < active_second_index_) {
      auto index_diff = active_second_index_ - last_active_index;
      memset(
        &seconds_window_[last_active_index],
        0,
        index_diff * sizeof(SecondsWindow::value_type)
      );
    } else {
      assert(last_active_index > active_second_index_);
      memset(
        &seconds_window_[last_active_index],
        0,
        (seconds_window_.size() - last_active_index) * sizeof(SecondsWindow::value_type)
      );

      memset(
        &seconds_window_[0],
        0,
        active_second_index_ * sizeof(SecondsWindow::value_type)
      );
    }
  }

  void inc_frame_count() {
    seconds_window_[active_second_index_]++;
  }

private:
  std::vector<uint32_t> seconds_window_;
  SecondsWindow::size_type active_second_index_; // index of last "active" second in which a frame has been marked

  TimeMs last_frame_start_;
};

}


