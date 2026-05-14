//
// Created by huan.yang on 2026-05-14.
//
#include "buffer/clock_sweep_lru.h"

#include "common/chicken_execption.h"
#include "common/macro.h"

using namespace chickenDB;

auto ClockSweepLRU::Evict() -> frame_id_t {
    while (true) {
        for (size_t i = 0; i < capacity_; i++) {
            Unpin(i);

            if (frames_[i].load() == 0) {
                return i;
            }
        }
    }
}


auto ClockSweepLRU::Pin(frame_id_t frame_id) -> void {
    ChickenException::AssertCondition(frame_id < frames_.size(),"[ClockSweepLru::Pin] frame_id is out of bounds]");
    if (frames_[frame_id].load() < LRU_MAX_PIN) {
        frames_[frame_id].fetch_add(1);
    }
}



auto ClockSweepLRU::Unpin(frame_id_t frame_id) -> void {
    ChickenException::AssertCondition(frame_id < frames_.size(),"[ClockSweepLru::Unpin] frame_id is out of bounds]");
    if (frames_[frame_id].load() > 0) {
        frames_[frame_id].fetch_sub(1);
    }
}



