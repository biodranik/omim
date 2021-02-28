#include "testing/testing.hpp"

//#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <string>

namespace {
  strings::UniString allUtf8Chars[100];
}

UNIT_TEST(Normalize_Unicode_Speed_Init) {
strings::UniString s;
  for (uint32_t c = 0x0001; c <= 0xD7FF; ++c)
    s.push_back(c);
  for (uint32_t c = 0xE000; c <= 0x10FFFF; ++c)
    s.push_back(c);
  for (int i = 0; i < sizeof(allUtf8Chars)/sizeof(allUtf8Chars[0]); ++i)
    allUtf8Chars[i] = s;
}

UNIT_TEST(Normalize_Unicode_Speed) {
  for (int i = 0; i < sizeof(allUtf8Chars)/sizeof(allUtf8Chars[0]); ++i)
    strings::NormalizeInplace(allUtf8Chars[i]);
}
