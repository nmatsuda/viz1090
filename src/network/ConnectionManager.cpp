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

#include "viz1090/network/ConnectionManager.h"

#include "viz1090/decoder/ModeSDecoder.h"

namespace viz1090::network {

ConnectionManager::ConnectionManager() = default;

ConnectionManager::~ConnectionManager() {
  stop();
}

void
ConnectionManager::start(const Config& aConfig) {
  if (mRunning) {
    return;
  }

  mConfig = aConfig;
  mRunning = true;

  // Create io_context and work guard
  mIoContext = std::make_unique<asio::io_context>();
  mWorkGuard = std::make_unique<
      asio::executor_work_guard<asio::io_context::executor_type>>(
      mIoContext->get_executor());

  // Create decoder
  mDecoder = std::make_unique<decoder::ModeSDecoder>();

  // Create Beast client
  mBeastClient = std::make_shared<BeastClient>(*mIoContext);

  // Set up handlers
  mBeastClient->onMessage(
      [this](const BeastMessage& aMsg) { handleBeastMessage(aMsg); });

  mBeastClient->onStateChange([this](bool aConnected) {
    if (mStateHandler) {
      mStateHandler(aConnected, mConfig.host);
    }
  });

  mBeastClient->onError([this](const std::string& aError) {
    // Log error (could add an error handler callback)
    (void)aError;
  });

  // Start network thread
  mNetworkThread = std::thread(&ConnectionManager::runIoContext, this);

  // Connect
  BeastClient::Config clientConfig;
  clientConfig.host = aConfig.host;
  clientConfig.port = aConfig.port;
  clientConfig.autoReconnect = aConfig.autoReconnect;
  clientConfig.reconnectDelay = aConfig.reconnectDelay;

  asio::post(*mIoContext,
             [this, clientConfig]() { mBeastClient->connect(clientConfig); });
}

void
ConnectionManager::stop() {
  if (!mRunning) {
    return;
  }

  mRunning = false;

  // Disconnect client
  if (mBeastClient) {
    asio::post(*mIoContext, [this]() { mBeastClient->disconnect(); });
  }

  // Release work guard to allow io_context to finish
  mWorkGuard.reset();

  // Stop io_context
  if (mIoContext) {
    mIoContext->stop();
  }

  // Join network thread
  if (mNetworkThread.joinable()) {
    mNetworkThread.join();
  }

  // Clean up
  mBeastClient.reset();
  mDecoder.reset();
  mIoContext.reset();
}

bool
ConnectionManager::isConnected() const {
  return mBeastClient && mBeastClient->isConnected();
}

ConnectionManager::Stats
ConnectionManager::stats() const {
  std::lock_guard<std::mutex> lock(mStatsMutex);
  Stats s = mStats;

  if (mBeastClient) {
    auto clientStats = mBeastClient->stats();
    s.messagesReceived = clientStats.messagesReceived;
    s.bytesReceived = clientStats.bytesReceived;
    s.reconnects = clientStats.reconnects;
  }

  return s;
}

void
ConnectionManager::runIoContext() {
  mIoContext->run();
}

void
ConnectionManager::handleBeastMessage(const BeastMessage& aMsg) {
  // Skip Mode A/C messages for now (could add support later)
  if (aMsg.type == BeastMessageType::ModeAC) {
    return;
  }

  // Decode the Mode S message
  auto decoded = mDecoder->decode(aMsg.data.data(), aMsg.dataLen,
                                  aMsg.timestamp, aMsg.signalLevel);

  if (decoded) {
    decoded->setRemote(true);  // Mark as received over network

    {
      std::lock_guard<std::mutex> lock(mStatsMutex);
      mStats.messagesDecoded++;
    }

    // Deliver to handler
    if (mMessageHandler) {
      mMessageHandler(*decoded);
    }
  }
}

}  // namespace viz1090::network
