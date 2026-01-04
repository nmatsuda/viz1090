// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Unit tests for CPR (Compact Position Reporting) decoder

#include <gtest/gtest.h>

#include <cmath>

#include "viz1090/decoder/CprDecoder.h"

using namespace viz1090;
using namespace viz1090::decoder;

class CprDecoderTest : public ::testing::Test {
protected:
  // Tolerance for position comparisons (degrees)
  static constexpr double kPositionTolerance = 0.01;

  // Test vectors from CPR specification
  static constexpr int kEvenLat = 93000;
  static constexpr int kEvenLon = 51372;
  static constexpr int kOddLat = 74158;
  static constexpr int kOddLon = 50194;

  static constexpr double kExpectedLat = 52.2572021484375;
  static constexpr double kExpectedLon = 3.91937255859375;
};

TEST_F(CprDecoderTest, GlobalDecodeEvenFrame) {
  CprDecoder::CprFrame evenFrame{kEvenLat, kEvenLon, 0};
  CprDecoder::CprFrame oddFrame{kOddLat, kOddLon, 1};

  auto result = CprDecoder::decodeGlobal(evenFrame, oddFrame, false);

  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->latitude, kExpectedLat, kPositionTolerance);
  EXPECT_NEAR(result->longitude, kExpectedLon, kPositionTolerance);
}

TEST_F(CprDecoderTest, RelativeDecodeEvenFrame) {
  CprDecoder::CprFrame frame{kEvenLat, kEvenLon, 0};

  auto result =
      CprDecoder::decodeRelative(frame, false, kExpectedLat, kExpectedLon);

  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->latitude, kExpectedLat, kPositionTolerance);
  EXPECT_NEAR(result->longitude, kExpectedLon, kPositionTolerance);
}

TEST_F(CprDecoderTest, LatitudeRangeValidation) {
  CprDecoder::CprFrame evenFrame{kEvenLat, kEvenLon, 0};
  CprDecoder::CprFrame oddFrame{kOddLat, kOddLon, 1};

  auto result = CprDecoder::decodeGlobal(evenFrame, oddFrame, false);

  ASSERT_TRUE(result.has_value());
  EXPECT_GE(result->latitude, -90.0);
  EXPECT_LE(result->latitude, 90.0);
}

TEST_F(CprDecoderTest, LongitudeRangeValidation) {
  CprDecoder::CprFrame evenFrame{kEvenLat, kEvenLon, 0};
  CprDecoder::CprFrame oddFrame{kOddLat, kOddLon, 1};

  auto result = CprDecoder::decodeGlobal(evenFrame, oddFrame, false);

  ASSERT_TRUE(result.has_value());
  EXPECT_GE(result->longitude, -180.0);
  EXPECT_LE(result->longitude, 180.0);
}

TEST_F(CprDecoderTest, DecoderDoesNotCrashOnEdgeCases) {
  // Test with zero values
  CprDecoder::CprFrame zeroFrame{0, 0, 0};
  auto result1 = CprDecoder::decodeGlobal(zeroFrame, zeroFrame, false);
  (void)result1;

  // Test with maximum values
  CprDecoder::CprFrame maxFrame{131071, 131071, 0};
  auto result2 = CprDecoder::decodeGlobal(maxFrame, maxFrame, false);
  (void)result2;

  SUCCEED();
}

TEST_F(CprDecoderTest, RelativeDecodeNearReference) {
  CprDecoder::CprFrame frame{kEvenLat, kEvenLon, 0};

  auto result = CprDecoder::decodeRelative(
      frame, false, kExpectedLat + 0.001, kExpectedLon + 0.001);

  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->latitude, kExpectedLat, kPositionTolerance);
  EXPECT_NEAR(result->longitude, kExpectedLon, kPositionTolerance);
}
