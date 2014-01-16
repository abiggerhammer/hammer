#ifndef HAMMER_HAMMER_TEST__HPP
#define HAMMER_HAMMER_TEST__HPP
#include <string>

#include <gtest/gtest.h>
#include <hammer/hammer.hpp>

#define HAMMER_DECL_UNUSED __attribute__((unused))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused"

static ::testing::AssertionResult ParseFails (hammer::Parser parser,
					     const std::string &input) {
  hammer::ParseResult result = parser.parse(input);
  if (result) {
    return ::testing::AssertionFailure() << "Parse succeeded with " << result.asUnambiguous() << "; expected failure";
  } else {
    return ::testing::AssertionSuccess();
  }
}

static ::testing::AssertionResult ParsesOK(hammer::Parser parser,
					   const std::string &input) {
  hammer::ParseResult result = parser.parse(input);
  if (!result) {
    return ::testing::AssertionFailure() << "Parse failed; expected success";
  } else {
    return ::testing::AssertionSuccess();
  }
}

static ::testing::AssertionResult ParsesTo(hammer::Parser parser,
					   const std::string &input,
					   const std::string &expected_result) {
  hammer::ParseResult result = parser.parse(input);
  if (!result) {
    return ::testing::AssertionFailure() << "Parse failed; expected success";
  } else if (result.asUnambiguous() != expected_result) {
    return ::testing::AssertionFailure()
      << "Parse succeeded with wrong result: got "
      << result.asUnambiguous()
      << "; expected "
      << expected_result;
  } else {
    return ::testing::AssertionSuccess();
  }
}

#pragma GCC diagnostic pop

#endif // defined(HAMMER_HAMMER_TEST__HPP)
