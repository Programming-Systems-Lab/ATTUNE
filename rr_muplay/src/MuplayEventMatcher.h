#ifndef RR_MUPLAY_EVENT_MATCHER_H_
#define RR_MUPLAY_EVENT_MATCHER_H_

#include <string>
#include "TraceStream.h"

namespace rr {

  /**
  * Given two sets of events does the event matching logic to determine what events match with what
  */

  class MuplayEventMatcher {
  public:
    /* Variables */
    std::vector<TraceFrame>& oldFrames, newFrames;

    /* Constructors */
    MuplayEventMatcher(std::vector<TraceFrame>& oldTrace,
                       std::vector<TraceFrame>& newTrace);

    /* Functions */
    std::vector<TraceFrame> combineTraces();
    bool is_frame_match(int index);
  };
} // namespace rr

#endif
