#ifndef HAMMER_HAMMER_TEST__HPP
#define HAMMER_HAMMER_TEST__HPP
#include <string>

#include <gtest/gtest.h>
#include <hammer/hammer.hpp>

static ::testing::AssertionResult ParseFails(hammer::Parser parser,
					     const string &input) {
  hammer::ParseResult result = parser.Parse(input);
  if (result) {
    return ::testing::AssertionFailure() << "Parse succeeded with " << result.AsUnambiguous() << "; expected failure";
  } else {
    return ::testing::AssertionSuccess();
  }
}

static ::testing::AssertionResult ParsesOK(hammer::Parser parser,
					   const string &input) {
  hammer::ParseResult result = parser.Parse(input);
  if (!result) {
    return ::testing::AssertionFailure() << "Parse failed; expected success";
  } else {
    return ::testing::AssertionSuccess();
  }
}

static ::testing::AssertionResult ParsesTo(hammer::Parser parser,
					   const string &input,
					   const string &expected_result) {
  hammer::ParseResult result = parser.Parse(input);
  if (!result) {
    return ::testing::AssertionFailure() << "Parse failed; expected success";
  } else if (result.AsUnambiguous() != expected_result) {
    return ::testing::AssertionFailure()
      << "Parse succeeded with wrong result: got "
      << result.AsUnambiguous()
      << "; expected "
      << expected_result;
  } else {
    return ::testing::AssertionSuccess();
  }
}


#endif // defined(HAMMER_HAMMER_TEST__HPP)
