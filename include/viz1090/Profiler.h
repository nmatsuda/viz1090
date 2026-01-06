// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VIZ1090_PROFILER_H
#define VIZ1090_PROFILER_H

#include <chrono>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace viz1090 {

// Conditional compilation based on PROFILING_ENABLED
#ifdef PROFILING_ENABLED
#define PROFILE_SCOPE(name) viz1090::ScopedTimer _profiler_##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)
#define PROFILE_BEGIN_FRAME() viz1090::Profiler::instance().beginFrame()
#define PROFILE_END_FRAME() viz1090::Profiler::instance().endFrame()
#define PROFILE_REPORT() viz1090::Profiler::instance().report()
#define PROFILE_RESET() viz1090::Profiler::instance().reset()
#else
#define PROFILE_SCOPE(name) (void)0
#define PROFILE_FUNCTION() (void)0
#define PROFILE_BEGIN_FRAME() (void)0
#define PROFILE_END_FRAME() (void)0
#define PROFILE_REPORT() (void)0
#define PROFILE_RESET() (void)0
#endif

struct TimingStats {
  std::string name;
  double totalMs{0.0};
  double minMs{1e9};
  double maxMs{0.0};
  uint64_t callCount{0};
  uint64_t frameCallCount{0};  // Calls in current frame

  void addSample(double ms) {
    totalMs += ms;
    if (ms < minMs)
      minMs = ms;
    if (ms > maxMs)
      maxMs = ms;
    callCount++;
    frameCallCount++;
  }

  [[nodiscard]] double avgMs() const {
    return callCount > 0 ? totalMs / static_cast<double>(callCount) : 0.0;
  }

  void resetFrame() { frameCallCount = 0; }
};

class Profiler {
public:
  static Profiler& instance() {
    static Profiler sInstance;
    return sInstance;
  }

  void beginFrame() {
    std::lock_guard<std::mutex> lock(mMutex);
    mFrameStart = std::chrono::high_resolution_clock::now();

    // Reset per-frame counters
    for (auto& pair : mStats) {
      pair.second.resetFrame();
    }
  }

  void endFrame() {
    std::lock_guard<std::mutex> lock(mMutex);
    auto frameEnd = std::chrono::high_resolution_clock::now();
    double frameMs =
        std::chrono::duration<double, std::milli>(frameEnd - mFrameStart)
            .count();

    mFrameTimes.push_back(frameMs);
    mFrameCount++;

    // Keep only last N frames for rolling stats
    if (mFrameTimes.size() > kMaxFrameHistory) {
      mFrameTimes.erase(mFrameTimes.begin());
    }

    // Print periodic report
    if (mFrameCount % mReportInterval == 0) {
      printReport();
    }
  }

  void recordTime(const std::string& aName, double aMs) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto& stats = mStats[aName];
    stats.name = aName;
    stats.addSample(aMs);
  }

  void report() {
    std::lock_guard<std::mutex> lock(mMutex);
    printReport();
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mMutex);
    mStats.clear();
    mFrameTimes.clear();
    mFrameCount = 0;
  }

  void setReportInterval(uint64_t aFrames) { mReportInterval = aFrames; }

private:
  Profiler() = default;

  void printReport() {
    if (mFrameTimes.empty()) {
      return;
    }

    // Calculate frame time stats
    double totalFrameMs = 0.0;
    double minFrameMs = 1e9;
    double maxFrameMs = 0.0;

    for (double ft : mFrameTimes) {
      totalFrameMs += ft;
      if (ft < minFrameMs)
        minFrameMs = ft;
      if (ft > maxFrameMs)
        maxFrameMs = ft;
    }

    double avgFrameMs = totalFrameMs / static_cast<double>(mFrameTimes.size());
    double avgFps = 1000.0 / avgFrameMs;

    std::fprintf(stderr, "\n");
    std::fprintf(stderr,
                 "========== PROFILING REPORT (Frame %lu) ==========\n",
                 static_cast<unsigned long>(mFrameCount));
    std::fprintf(stderr, "Frame Time: avg=%.2fms min=%.2fms max=%.2fms "
                         "(%.1f FPS avg)\n",
                 avgFrameMs, minFrameMs, maxFrameMs, avgFps);
    std::fprintf(stderr, "\n");

    // Sort by total time descending
    std::vector<const TimingStats*> sorted;
    for (const auto& pair : mStats) {
      sorted.push_back(&pair.second);
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const TimingStats* a, const TimingStats* b) {
                return a->totalMs > b->totalMs;
              });

    std::fprintf(stderr, "%-30s %10s %10s %10s %10s %10s %8s\n", "Function",
                 "Total(ms)", "Avg(ms)", "Min(ms)", "Max(ms)", "%%Frame",
                 "Calls");
    std::fprintf(stderr, "%-30s %10s %10s %10s %10s %10s %8s\n",
                 "------------------------------", "----------", "----------",
                 "----------", "----------", "----------", "--------");

    for (const auto* stats : sorted) {
      double pctFrame =
          (avgFrameMs > 0) ? (stats->avgMs() * 100.0 / avgFrameMs) : 0.0;

      std::fprintf(stderr, "%-30s %10.2f %10.3f %10.3f %10.3f %9.1f%% %8lu\n",
                   stats->name.c_str(), stats->totalMs, stats->avgMs(),
                   stats->minMs, stats->maxMs, pctFrame,
                   static_cast<unsigned long>(stats->callCount));
    }
    std::fprintf(stderr, "=================================================\n");
    std::fprintf(stderr, "\n");
  }

  std::mutex mMutex;
  std::map<std::string, TimingStats> mStats;
  std::vector<double> mFrameTimes;
  std::chrono::high_resolution_clock::time_point mFrameStart;
  uint64_t mFrameCount{0};
  uint64_t mReportInterval{300};  // Report every 300 frames (~10 sec at 30fps)
  static constexpr size_t kMaxFrameHistory = 300;
};

class ScopedTimer {
public:
  explicit ScopedTimer(const char* aName)
      : mName(aName), mStart(std::chrono::high_resolution_clock::now()) {}

  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    double ms =
        std::chrono::duration<double, std::milli>(end - mStart).count();
    Profiler::instance().recordTime(mName, ms);
  }

  // Non-copyable, non-movable
  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;
  ScopedTimer(ScopedTimer&&) = delete;
  ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
  const char* mName;
  std::chrono::high_resolution_clock::time_point mStart;
};

}  // namespace viz1090

#endif  // VIZ1090_PROFILER_H
