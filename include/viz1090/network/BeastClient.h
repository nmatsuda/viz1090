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

#ifndef VIZ1090_NETWORK_BEAST_CLIENT_H
#define VIZ1090_NETWORK_BEAST_CLIENT_H

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <asio.hpp>

#include "../Types.h"

namespace viz1090::network {

/// Beast protocol message types
enum class BeastMessageType : uint8_t {
  ModeAC = '1',   // Mode A/C message (2 bytes)
  ModeShort = '2', // Mode S short message (7 bytes)
  ModeLong = '3'   // Mode S long message (14 bytes)
};

/// Raw Beast message data
struct BeastMessage {
  BeastMessageType type;
  uint64_t timestamp;        // 48-bit MLAT timestamp
  SignalLevel signalLevel;   // Signal amplitude
  std::array<uint8_t, kLongMsgBytes> data;
  size_t dataLen;            // Actual data length (2, 7, or 14)
};

/// Beast protocol client using ASIO
///
/// Connects to a dump1090 Beast output and decodes the binary protocol.
/// Messages are delivered via a callback function.
class BeastClient : public std::enable_shared_from_this<BeastClient> {
public:
  /// Callback for received messages
  using MessageHandler = std::function<void(const BeastMessage&)>;

  /// Connection state callback
  using StateHandler = std::function<void(bool connected)>;

  /// Error callback
  using ErrorHandler = std::function<void(const std::string& error)>;

  /// Configuration
  struct Config {
    std::string host;
    uint16_t port = 30005;  // Default Beast output port
    Seconds reconnectDelay{5};
    bool autoReconnect = true;
  };

  explicit BeastClient(asio::io_context& aIoContext);
  ~BeastClient();

  // Non-copyable
  BeastClient(const BeastClient&) = delete;
  BeastClient& operator=(const BeastClient&) = delete;

  /// Set message handler
  void onMessage(MessageHandler aHandler) { mMessageHandler = std::move(aHandler); }

  /// Set state change handler
  void onStateChange(StateHandler aHandler) { mStateHandler = std::move(aHandler); }

  /// Set error handler
  void onError(ErrorHandler aHandler) { mErrorHandler = std::move(aHandler); }

  /// Connect to server
  void connect(const Config& aConfig);

  /// Disconnect from server
  void disconnect();

  /// Check if connected
  [[nodiscard]] bool isConnected() const { return mConnected; }

  /// Get statistics
  struct Stats {
    uint64_t messagesReceived = 0;
    uint64_t bytesReceived = 0;
    uint64_t parseErrors = 0;
    uint64_t reconnects = 0;
  };
  [[nodiscard]] const Stats& stats() const { return mStats; }

private:
  void doResolve();
  void doConnect(const asio::ip::tcp::resolver::results_type& aEndpoints);
  void doRead();
  void processBuffer();
  bool parseMessage(const uint8_t* aData, size_t aLen, BeastMessage& aMsg);
  void handleError(const asio::error_code& aEc, const std::string& aContext);
  void scheduleReconnect();

  // Unescape Beast protocol (0x1A escape sequences)
  static size_t unescapeData(const uint8_t* aSrc, size_t aSrcLen,
                             uint8_t* aDst, size_t aDstLen);

  asio::io_context& mIoContext;
  asio::ip::tcp::resolver mResolver;
  asio::ip::tcp::socket mSocket;
  asio::steady_timer mReconnectTimer;

  Config mConfig;
  std::atomic<bool> mConnected{false};
  bool mStopping = false;

  // Read buffer
  static constexpr size_t kBufferSize = 4096;
  std::array<uint8_t, kBufferSize> mReadBuffer;
  std::vector<uint8_t> mParseBuffer;

  // Handlers
  MessageHandler mMessageHandler;
  StateHandler mStateHandler;
  ErrorHandler mErrorHandler;

  Stats mStats;
};

}  // namespace viz1090::network

#endif  // VIZ1090_NETWORK_BEAST_CLIENT_H
