// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Unit tests for Mode S decoder

#include <gtest/gtest.h>

#include "viz1090/decoder/ModeSDecoder.h"

using namespace viz1090;
using namespace viz1090::decoder;

class ModeSDecoderTest : public ::testing::Test {
protected:
  ModeSDecoder decoder;

  // Real ADS-B message test vectors (verified working)
  // DF17 Extended Squitter - Aircraft Identification
  static constexpr uint8_t kDF17Ident[] = {0x8D, 0x48, 0x40, 0xD6, 0x20, 0x2C,
                                           0xC3, 0x71, 0xC3, 0x2C, 0xE0, 0x57,
                                           0x60, 0x98};

  // DF17 Extended Squitter - Airborne Position (odd)
  static constexpr uint8_t kDF17PosOdd[] = {0x8D, 0x40, 0x62, 0x1D, 0x58, 0xC3,
                                            0x82, 0xD6, 0x90, 0xC8, 0xAC, 0x28,
                                            0x63, 0xA7};

  // DF17 Extended Squitter - Airborne Velocity
  static constexpr uint8_t kDF17Velocity[] = {0x8D, 0x48, 0x50, 0x20, 0x99, 0x44,
                                              0x09, 0x94, 0x08, 0x38, 0x17, 0x5B,
                                              0x28, 0x4F};
};

TEST_F(ModeSDecoderTest, DecodeDF17Identification) {
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->msgType(), DownlinkFormat::ExtendedSquitter);
  EXPECT_EQ(result->addr(), 0x4840D6u);
  EXPECT_TRUE(result->crcOk());
}

TEST_F(ModeSDecoderTest, DecodeDF17Position) {
  auto result = decoder.decode(kDF17PosOdd, sizeof(kDF17PosOdd));

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->msgType(), DownlinkFormat::ExtendedSquitter);
  EXPECT_EQ(result->addr(), 0x40621Du);
  EXPECT_TRUE(result->crcOk());

  // Should have raw lat/lon
  EXPECT_TRUE(result->rawLatitude().has_value());
  EXPECT_TRUE(result->rawLongitude().has_value());
}

TEST_F(ModeSDecoderTest, DecodeDF17Velocity) {
  auto result = decoder.decode(kDF17Velocity, sizeof(kDF17Velocity));

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->msgType(), DownlinkFormat::ExtendedSquitter);
  EXPECT_EQ(result->addr(), 0x485020u);
  EXPECT_TRUE(result->crcOk());
}

TEST_F(ModeSDecoderTest, RejectCorruptedMessage) {
  uint8_t corrupted[14];
  std::copy(std::begin(kDF17Ident), std::end(kDF17Ident), corrupted);

  // Corrupt multiple bits to make it unrecoverable
  corrupted[5] ^= 0xFF;
  corrupted[6] ^= 0xFF;

  auto result = decoder.decode(corrupted, sizeof(corrupted));

  // Should reject corrupted message
  EXPECT_FALSE(result.has_value());
}

TEST_F(ModeSDecoderTest, IcaoCaching) {
  // First decode should add to cache
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));
  ASSERT_TRUE(result.has_value());

  // Address should now be in cache
  EXPECT_TRUE(decoder.isIcaoRecentlySeen(0x4840D6u));

  // Unknown address should not be in cache
  EXPECT_FALSE(decoder.isIcaoRecentlySeen(0x123456u));
}

TEST_F(ModeSDecoderTest, StatsTracking) {
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));
  ASSERT_TRUE(result.has_value());

  const auto& stats = decoder.stats();
  EXPECT_GE(stats.messagesDecoded, 1u);
}

TEST_F(ModeSDecoderTest, DownlinkFormatExtraction) {
  // DF is in first 5 bits
  // DF17 = 0x8D >> 3 = 17
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(static_cast<int>(result->msgType()), 17);
}

TEST_F(ModeSDecoderTest, ConfigurableMaxBitErrors) {
  ModeSDecoder::Config config;
  config.maxBitErrors = 0;  // Disable error correction

  ModeSDecoder strictDecoder(config);

  // Valid message should still decode
  auto result = strictDecoder.decode(kDF17Ident, sizeof(kDF17Ident));
  EXPECT_TRUE(result.has_value());
}

TEST_F(ModeSDecoderTest, IcaoAddressExtraction) {
  // ICAO address is in bytes 1-3 of DF17
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));
  ASSERT_TRUE(result.has_value());

  // 0x4840D6 = bytes 1, 2, 3 of message
  EXPECT_EQ(result->addr(), 0x4840D6u);
}

TEST_F(ModeSDecoderTest, MessageBitsSet) {
  // Long message should report 112 bits
  auto result = decoder.decode(kDF17Ident, sizeof(kDF17Ident));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->msgBits(), 112);
}

TEST_F(ModeSDecoderTest, TimestampPassedThrough) {
  uint64_t testTimestamp = 123456789;
  auto result =
      decoder.decode(kDF17Ident, sizeof(kDF17Ident), testTimestamp, 200);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->timestamp(), testTimestamp);
  EXPECT_EQ(result->signalLevel(), 200);
}
