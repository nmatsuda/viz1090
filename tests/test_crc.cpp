// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Unit tests for CRC-24 computation

#include <gtest/gtest.h>

#include "viz1090/decoder/Crc.h"

using namespace viz1090::decoder;

class CrcTest : public ::testing::Test {
protected:
  // Known good DF17 message with valid CRC (verified to work)
  // This is the same test vector used in ModeSDecoder tests
  static constexpr uint8_t kValidDF17[] = {0x8D, 0x48, 0x40, 0xD6, 0x20, 0x2C,
                                           0xC3, 0x71, 0xC3, 0x2C, 0xE0, 0x57,
                                           0x60, 0x98};
};

TEST_F(CrcTest, ComputeValidDF17ReturnsZero) {
  // A valid DF17 message should have CRC = 0 when computed over entire message
  uint32_t crc = Crc::compute(kValidDF17, 112);  // 14 bytes = 112 bits
  EXPECT_EQ(crc, 0u);
}

TEST_F(CrcTest, ComputeCorruptedMessageReturnsNonZero) {
  uint8_t corrupted[14];
  std::copy(std::begin(kValidDF17), std::end(kValidDF17), corrupted);

  // Flip a bit in the middle of the message
  corrupted[5] ^= 0x01;

  uint32_t crc = Crc::compute(corrupted, 112);
  EXPECT_NE(crc, 0u);
}

TEST_F(CrcTest, CrcIs24Bits) {
  // Even with maximum input, CRC should be 24 bits
  uint8_t allOnes[14];
  std::fill(std::begin(allOnes), std::end(allOnes), 0xFF);

  uint32_t crc = Crc::compute(allOnes, 112);
  EXPECT_LE(crc, 0xFFFFFFu);
}

TEST_F(CrcTest, CrcIsDeterministic) {
  // Same input should always produce same output
  uint8_t pattern[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint32_t crc1 = Crc::compute(pattern, 56);
  uint32_t crc2 = Crc::compute(pattern, 56);
  EXPECT_EQ(crc1, crc2);
}

TEST_F(CrcTest, DifferentInputsProduceDifferentCrc) {
  uint8_t pattern1[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8_t pattern2[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  uint32_t crc1 = Crc::compute(pattern1, 56);
  uint32_t crc2 = Crc::compute(pattern2, 56);
  EXPECT_NE(crc1, crc2);
}

TEST_F(CrcTest, LongAndShortMessages) {
  // Test with 56-bit (short) message
  uint8_t shortMsg[7] = {0};
  uint32_t crcShort = Crc::compute(shortMsg, 56);
  EXPECT_LE(crcShort, 0xFFFFFFu);

  // Test with 112-bit (long) message
  uint8_t longMsg[14] = {0};
  uint32_t crcLong = Crc::compute(longMsg, 112);
  EXPECT_LE(crcLong, 0xFFFFFFu);
}

TEST_F(CrcTest, SingleBitErrorDetected) {
  // CRC should detect single bit errors
  uint8_t original[14];
  std::copy(std::begin(kValidDF17), std::end(kValidDF17), original);

  // Compute CRC of valid message
  uint32_t crcOriginal = Crc::compute(original, 112);
  EXPECT_EQ(crcOriginal, 0u);

  // Try flipping each bit and verify CRC changes
  for (int byte = 0; byte < 14; byte++) {
    for (int bit = 0; bit < 8; bit++) {
      uint8_t modified[14];
      std::copy(std::begin(original), std::end(original), modified);
      modified[byte] ^= (1 << bit);

      uint32_t crcModified = Crc::compute(modified, 112);
      EXPECT_NE(crcModified, 0u)
          << "Bit flip at byte " << byte << " bit " << bit << " not detected";
    }
  }
}
