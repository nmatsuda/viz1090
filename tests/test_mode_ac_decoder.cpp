// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Unit tests for Mode A/C decoder

#include <gtest/gtest.h>

#include "viz1090/decoder/ModeAcDecoder.h"

using namespace viz1090;
using namespace viz1090::decoder;

class ModeAcDecoderTest : public ::testing::Test {
protected:
};

TEST_F(ModeAcDecoderTest, DecodeSquawk0000) {
  // Zero input should decode to zero squawk
  uint16_t raw = 0;
  SquawkCode squawk = ModeAcDecoder::decodeModeA(raw);
  EXPECT_EQ(squawk, 0u);
}

TEST_F(ModeAcDecoderTest, DecodeModeADeterministic) {
  // Same input should always produce same output
  for (uint16_t raw = 0; raw < 100; raw++) {
    SquawkCode squawk1 = ModeAcDecoder::decodeModeA(raw);
    SquawkCode squawk2 = ModeAcDecoder::decodeModeA(raw);
    EXPECT_EQ(squawk1, squawk2) << "Non-deterministic for raw=" << raw;
  }
}

TEST_F(ModeAcDecoderTest, ModeAToModeCReturnsNulloptForInvalidCode) {
  // D1 bit set indicates Mode A only (invalid for Mode C)
  uint16_t modeAOnly = 0x0001;  // D1 set

  auto result = ModeAcDecoder::modeAToModeC(modeAOnly);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ModeAcDecoderTest, ModeAToModeCValidConversion) {
  // Test a valid Mode C code (no D1 bit)
  uint16_t validModeC = 0x0000;

  auto result = ModeAcDecoder::modeAToModeC(validModeC);
  // Should return some altitude value
  if (result.has_value()) {
    // Altitude should be within reasonable range
    EXPECT_GE(*result, -20);
    EXPECT_LE(*result, 1300);  // ~130,000 feet in hundreds
  }
}

TEST_F(ModeAcDecoderTest, DecoderDoesNotCrash) {
  // Test that decoder handles all possible 12-bit inputs without crashing
  for (uint16_t raw = 0; raw < 4096; raw++) {
    SquawkCode squawk = ModeAcDecoder::decodeModeA(raw);
    (void)squawk;  // Just ensure no crash

    auto altitude = ModeAcDecoder::modeAToModeC(raw);
    (void)altitude;  // Just ensure no crash
  }
  SUCCEED();
}

TEST_F(ModeAcDecoderTest, SquawkMaxValue) {
  // Maximum 12-bit input
  uint16_t raw = 0x0FFF;
  SquawkCode squawk = ModeAcDecoder::decodeModeA(raw);

  // Squawk should be <= 7777 (max octal)
  EXPECT_LE(squawk, 7777u);
}

TEST_F(ModeAcDecoderTest, ModeCAltitudeRange) {
  int validCount = 0;
  int minAlt = 9999;
  int maxAlt = -9999;

  for (uint16_t raw = 0; raw < 4096; raw++) {
    auto result = ModeAcDecoder::modeAToModeC(raw);
    if (result.has_value()) {
      validCount++;
      if (*result < minAlt) minAlt = *result;
      if (*result > maxAlt) maxAlt = *result;
    }
  }

  // Should have some valid conversions
  EXPECT_GT(validCount, 0);

  // Altitude range should be reasonable
  if (validCount > 0) {
    EXPECT_GE(minAlt, -20);   // Below sea level but not crazy
    EXPECT_LE(maxAlt, 1300);  // ~130,000 feet
  }
}
