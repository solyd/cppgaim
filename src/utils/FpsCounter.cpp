#include "FpsCounter.h"

namespace cppgaim {


void FpsCounter::mark_frame_start(FpsCounter::TimeMs frame_start_time) {
  update_window_index(frame_start_time);
  inc_frame_count();
  last_frame_start_ = frame_start_time;
}

}
