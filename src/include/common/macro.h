//
// Created by huan.yang on 2026-04-30.
//
#pragma once

namespace chickenDB {

#define PAGE_SIZE (1024 * 256)

#define K_SIMD_ALIGNMENT 64

#define K_DEFAULT_CAPACITY 2048

#define K_LRU_FILE_CAPACITY 1024

#define LRU_MAX_PIN 5

// Background flusher: wake interval and dirty-ratio trigger
#define K_FLUSH_INTERVAL_MS 200
#define K_DIRTY_FLUSH_RATIO 0.30

}
