// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// Copyright (C) 2014, Malcolm Robb <Support@ATTAvionics.com>
// Copyright (C) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef VIZ1090_NETWORK_CONNECTION_MANAGER_H
#define VIZ1090_NETWORK_CONNECTION_MANAGER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <asio.hpp>

#include "../ModesMessage.h"
#include "../Types.h"
#include "BeastClient.h"

namespace viz1090 {
namespace decoder {
class ModeSDecoder;
}
}  // namespace viz1090

namespace viz1090::network {

/// Network connection manager
///
/// Manages connections to dump1090 servers and delivers decoded messages
/// to the application. Runs ASIO on a dedicated thread.
class ConnectionManager {
public:
  /// Callback for decoded messages
  using MessageHandler = std::function<void(const ModesMessage&)>;

  /// Callback for connection state changes
  using StateHandler = std::function<void(bool connected, const std::string& host)>;

  /// Configuration
  struct Config {
    std::string host = "localhost";
    uint16_t port = 30005;
    bool autoReconnect = true;
    Seconds reconnectDelay{5};
  };

  ConnectionManager();
  ~ConnectionManager();

  // Non-copyable
  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  /// Set message handler (called from network thread)
  void onMessage(MessageHandler aHandler) { mMessageHandler = std::move(aHandler); }

  /// Set state change handler (called from network thread)
  void onStateChange(StateHandler aHandler) { mStateHandler = std::move(aHandler); }

  /// Start the network thread and connect
  void start(const Config& aConfig);

  /// Stop the network thread
  void stop();

  /// Check if running
  [[nodiscard]] bool isRunning() const { return mRunning; }

  /// Check if connected
  [[nodiscard]] bool isConnected() const;

  /// Get connection statistics
  struct Stats {
    uint64_t messagesReceived = 0;
    uint64_t messagesDecoded = 0;
    uint64_t bytesReceived = 0;
    uint64_t reconnects = 0;
  };
  [[nodiscard]] Stats stats() const;

private:
  void runIoContext();
  void handleBeastMessage(const BeastMessage& aMsg);

  std::unique_ptr<asio::io_context> mIoContext;
  std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>>
      mWorkGuard;
  std::shared_ptr<BeastClient> mBeastClient;
  std::unique_ptr<decoder::ModeSDecoder> mDecoder;
  std::thread mNetworkThread;

  Config mConfig;
  std::atomic<bool> mRunning{false};

  MessageHandler mMessageHandler;
  StateHandler mStateHandler;

  mutable std::mutex mStatsMutex;
  Stats mStats;
};

}  // namespace viz1090::network

#endif  // VIZ1090_NETWORK_CONNECTION_MANAGER_H
