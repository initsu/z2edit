#ifndef PROJECT_UTIL_FPSMGR_H
#define PROJECT_UTIL_FPSMGR_H
#include <SDL2/SDL.h>

class FPSManager {
  public:
    FPSManager() : rate_(60), frame_count_(0), last_tick_(0) {}

    int SetRate(int rate) {
        rate_ = rate > 0 ? rate : 1;
        return 0;
    }
    int GetRate(int rate) {
        return rate_;
    }
    int Count() {
        return frame_count_;
    }
    int Delay() {
        frame_count_++;
        Uint32 now = SDL_GetTicks();
        Uint32 target = last_tick_ + 1000 / rate_;
        if (now < target)
            SDL_Delay(target - now);
        last_tick_ = SDL_GetTicks();
        return 0;
    }
  private:
    int rate_;
    int frame_count_;
    Uint32 last_tick_;
};

#endif // PROJECT_UTIL_FPSMGR_H