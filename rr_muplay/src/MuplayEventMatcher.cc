using namespace std;

#include "MuplayEventMatcher.h"
#include "TraceStream.h"
namespace rr {

  MuplayEventMatcher::MuplayEventMatcher(vector<TraceFrame>& oldFrames,
                                         vector<TraceFrame>& newFrames) :
    oldFrames(oldFrames),
    newFrames(newFrames) {}

  /* Returns 1 combined log from the two */
  vector<TraceFrame>   MuplayEventMatcher::combineTraces()
  {
    vector<TraceFrame> res;

    unsigned int bigger = 0;
    if (this->oldFrames.size() > this->newFrames.size())
      bigger = this->oldFrames.size();
    else
      bigger = this->newFrames.size();

    for (unsigned int i=0; i < bigger; i++ )
    {
      if (i >= this->oldFrames.size())
        res.push_back(this->newFrames.at(i));
      else if (i >= this->newFrames.size())
        res.push_back(this->oldFrames.at(i));
      else if ( i < this->oldFrames.size() && i < newFrames.size())
      {
        TraceFrame oldFrame, newFrame;
        oldFrame = this->oldFrames.at(i);
        newFrame = this->newFrames.at(i);
        if (oldFrame.event().str().compare(newFrame.event().str()))
        {
          unsigned int comp = oldFrame.event().str().compare(newFrame.event().str());
          printf("This isn't supposed to happen\n");
          printf("value of compare and i: %u     %u\n", comp, i );
          printf("oldFrame event%s\n",oldFrame.event().str().c_str() );
          printf("newFrame event%s\n",newFrame.event().str().c_str() );
          printf("For now going with new frame\n");
          printf("-----\n");
          res.push_back(newFrame);
        } else {
          res.push_back(oldFrame);
        }
      }

    }
    return res;
  }

  bool is_frame_match(int index)
  {
    // TraceFrame oldFrame = oldFrames.at(index);
    // TraceFrame currentFrame = newFrames.at(index);
    // return   if (oldFrame.event().str().compare(newFrame.event().str()))
    if(index > 0)
      return true;
    return false;
  }

}
