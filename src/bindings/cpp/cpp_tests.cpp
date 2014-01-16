#include <gtest/gtest.h>
#include <hammer/hammer.hpp>
#include <hammer/hammer_test.hpp>

namespace {
  using namespace ::hammer;
  TEST(ParserTypes, Token) {
    Parser p = Token("95\xA2");
    EXPECT_TRUE(ParsesTo(p, "95\xA2", "<39.35.a2>"));
    EXPECT_TRUE(ParseFails(p, "95"));
  }

  TEST(ParserTypes, Ch) {
    Parser p = Ch(0xA2);
    EXPECT_TRUE(ParsesTo(p, "\xA2", "u0xa2"));
    EXPECT_TRUE(ParseFails(p, "\xA3"));
  }
};

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
