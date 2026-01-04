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

#include "viz1090/network/BeastClient.h"

#include <algorithm>

namespace viz1090::network {

// Beast protocol constants
constexpr uint8_t kBeastEscape = 0x1A;
constexpr size_t kModeACBytes = 2;
constexpr size_t kTimestampBytes = 6;
constexpr size_t kSignalBytes = 1;

// Minimum message size: escape + type + timestamp(6) + signal(1) + data(2)
constexpr size_t kMinMessageSize = 1 + 1 + kTimestampBytes + kSignalBytes + kModeACBytes;

BeastClient::BeastClient(asio::io_context& aIoContext)
    : mIoContext(aIoContext),
      mResolver(aIoContext),
      mSocket(aIoContext),
      mReconnectTimer(aIoContext) {
  mParseBuffer.reserve(kBufferSize * 2);
}

BeastClient::~BeastClient() {
  disconnect();
}

void
BeastClient::connect(const Config& aConfig) {
  mConfig = aConfig;
  mStopping = false;
  doResolve();
}

void
BeastClient::disconnect() {
  mStopping = true;
  mReconnectTimer.cancel();

  asio::error_code ec;
  if (mSocket.is_open()) {
    mSocket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    mSocket.close(ec);
  }

  if (mConnected.exchange(false)) {
    if (mStateHandler) {
      mStateHandler(false);
    }
  }
}

void
BeastClient::doResolve() {
  auto self = shared_from_this();

  mResolver.async_resolve(
      mConfig.host, std::to_string(mConfig.port),
      [this, self](const asio::error_code& aEc,
                   asio::ip::tcp::resolver::results_type aResults) {
        if (mStopping) {
          return;
        }

        if (aEc) {
          handleError(aEc, "resolve");
          return;
        }

        doConnect(aResults);
      });
}

void
BeastClient::doConnect(const asio::ip::tcp::resolver::results_type& aEndpoints) {
  auto self = shared_from_this();

  asio::async_connect(
      mSocket, aEndpoints,
      [this, self](const asio::error_code& aEc,
                   const asio::ip::tcp::endpoint& /*aEndpoint*/) {
        if (mStopping) {
          return;
        }

        if (aEc) {
          handleError(aEc, "connect");
          return;
        }

        // Connection successful
        mConnected = true;
        mParseBuffer.clear();

        // Set TCP_NODELAY for low latency
        asio::error_code ec;
        mSocket.set_option(asio::ip::tcp::no_delay(true), ec);

        if (mStateHandler) {
          mStateHandler(true);
        }

        // Start reading
        doRead();
      });
}

void
BeastClient::doRead() {
  auto self = shared_from_this();

  mSocket.async_read_some(
      asio::buffer(mReadBuffer),
      [this, self](const asio::error_code& aEc, size_t aBytesRead) {
        if (mStopping) {
          return;
        }

        if (aEc) {
          handleError(aEc, "read");
          return;
        }

        mStats.bytesReceived += aBytesRead;

        // Append to parse buffer
        mParseBuffer.insert(mParseBuffer.end(), mReadBuffer.begin(),
                            mReadBuffer.begin() + aBytesRead);

        // Process complete messages
        processBuffer();

        // Continue reading
        doRead();
      });
}

void
BeastClient::processBuffer() {
  // Find and process Beast messages in the buffer
  // Beast format: 0x1A <type> <timestamp:6> <signal:1> <data:N>
  // Type: '1' = ModeA/C (2 bytes), '2' = Mode S short (7), '3' = Mode S long (14)
  // 0x1A bytes in data are escaped as 0x1A 0x1A

  size_t pos = 0;
  while (pos < mParseBuffer.size()) {
    // Find start of message (0x1A)
    auto it = std::find(mParseBuffer.begin() + pos, mParseBuffer.end(), kBeastEscape);
    if (it == mParseBuffer.end()) {
      // No message start found, discard buffer
      mParseBuffer.clear();
      return;
    }

    pos = it - mParseBuffer.begin();

    // Need at least escape + type
    if (pos + 1 >= mParseBuffer.size()) {
      break;
    }

    // Check for escaped 0x1A (0x1A 0x1A)
    if (mParseBuffer[pos + 1] == kBeastEscape) {
      pos += 2;
      continue;
    }

    // Get message type
    uint8_t typeChar = mParseBuffer[pos + 1];
    size_t dataLen = 0;

    switch (typeChar) {
      case static_cast<uint8_t>(BeastMessageType::ModeAC):
        dataLen = kModeACBytes;
        break;
      case static_cast<uint8_t>(BeastMessageType::ModeShort):
        dataLen = kShortMsgBytes;
        break;
      case static_cast<uint8_t>(BeastMessageType::ModeLong):
        dataLen = kLongMsgBytes;
        break;
      default:
        // Invalid type, skip this byte
        pos++;
        continue;
    }

    // Calculate minimum message length (without escapes)
    size_t minLen = 2 + kTimestampBytes + kSignalBytes + dataLen;

    // We need to account for potential escape sequences
    // Worst case: every byte is escaped, so 2x length
    size_t remaining = mParseBuffer.size() - pos;
    if (remaining < minLen) {
      // Need more data
      break;
    }

    // Try to parse the message, accounting for escapes
    // Work buffer for unescaped data
    std::array<uint8_t, 64> unescaped;
    size_t srcPos = pos + 2;  // Skip 0x1A and type
    size_t dstPos = 0;
    size_t needed = kTimestampBytes + kSignalBytes + dataLen;

    bool incomplete = false;
    while (dstPos < needed && srcPos < mParseBuffer.size()) {
      uint8_t b = mParseBuffer[srcPos++];
      if (b == kBeastEscape) {
        if (srcPos >= mParseBuffer.size()) {
          incomplete = true;
          break;
        }
        b = mParseBuffer[srcPos++];
        // If it's another 0x1A, it's an escaped 0x1A
        // Otherwise it's start of next message (shouldn't happen mid-message)
        if (b != kBeastEscape) {
          // Malformed, treat as end of this message
          mStats.parseErrors++;
          pos = srcPos - 2;  // Restart at the new 0x1A
          incomplete = true;
          break;
        }
      }
      unescaped[dstPos++] = b;
    }

    if (incomplete || dstPos < needed) {
      // Need more data
      break;
    }

    // Parse the message
    BeastMessage msg;
    msg.type = static_cast<BeastMessageType>(typeChar);
    msg.dataLen = dataLen;

    // Extract 48-bit timestamp (big-endian)
    msg.timestamp = 0;
    for (size_t i = 0; i < kTimestampBytes; i++) {
      msg.timestamp = (msg.timestamp << 8) | unescaped[i];
    }

    // Extract signal level
    msg.signalLevel = unescaped[kTimestampBytes];

    // Extract message data
    std::copy(unescaped.begin() + kTimestampBytes + kSignalBytes,
              unescaped.begin() + kTimestampBytes + kSignalBytes + dataLen,
              msg.data.begin());

    mStats.messagesReceived++;

    // Deliver message
    if (mMessageHandler) {
      mMessageHandler(msg);
    }

    // Move past this message
    pos = srcPos;
  }

  // Remove processed data from buffer
  if (pos > 0) {
    mParseBuffer.erase(mParseBuffer.begin(), mParseBuffer.begin() + pos);
  }

  // Prevent buffer from growing too large
  if (mParseBuffer.size() > kBufferSize * 4) {
    mStats.parseErrors++;
    mParseBuffer.clear();
  }
}

void
BeastClient::handleError(const asio::error_code& aEc, const std::string& aContext) {
  // Close socket
  asio::error_code ec;
  if (mSocket.is_open()) {
    mSocket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    mSocket.close(ec);
  }

  bool wasConnected = mConnected.exchange(false);

  if (wasConnected && mStateHandler) {
    mStateHandler(false);
  }

  // Report error
  if (mErrorHandler && aEc != asio::error::operation_aborted) {
    mErrorHandler(aContext + ": " + aEc.message());
  }

  // Schedule reconnect if enabled
  if (!mStopping && mConfig.autoReconnect) {
    scheduleReconnect();
  }
}

void
BeastClient::scheduleReconnect() {
  mStats.reconnects++;

  auto self = shared_from_this();
  mReconnectTimer.expires_after(mConfig.reconnectDelay);
  mReconnectTimer.async_wait([this, self](const asio::error_code& aEc) {
    if (!aEc && !mStopping) {
      doResolve();
    }
  });
}

size_t
BeastClient::unescapeData(const uint8_t* aSrc, size_t aSrcLen,
                          uint8_t* aDst, size_t aDstLen) {
  size_t srcPos = 0;
  size_t dstPos = 0;

  while (srcPos < aSrcLen && dstPos < aDstLen) {
    uint8_t b = aSrc[srcPos++];
    if (b == kBeastEscape && srcPos < aSrcLen) {
      b = aSrc[srcPos++];
    }
    aDst[dstPos++] = b;
  }

  return dstPos;
}

}  // namespace viz1090::network
