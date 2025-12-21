#pragma once
// =============================================================================
// TEST.H - Lightweight Test Framework
// =============================================================================
// Simple test framework without external dependencies.
// =============================================================================

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace Hub {
namespace Test {

struct TestResult {
  std::string name;
  bool passed = false;
  std::string error;
};

class TestRunner {
public:
  static TestRunner &instance() {
    static TestRunner inst;
    return inst;
  }

  void addTest(const std::string &name, std::function<void()> test) {
    tests_.push_back({name, test});
  }

  int run() {
    int passed = 0, failed = 0;

    std::cout << "\n=== Running " << tests_.size() << " tests ===\n\n";

    for (size_t i = 0; i < tests_.size(); i++) {
      const auto &testPair = tests_[i];
      const std::string &name = testPair.first;
      const std::function<void()> &test = testPair.second;
      currentTest_ = name;
      currentError_.clear();

      try {
        test();
        if (currentError_.empty()) {
          std::cout << "[PASS] " << name << "\n";
          passed++;
          results_.push_back({name, true, ""});
        } else {
          std::cout << "[FAIL] " << name << ": " << currentError_ << "\n";
          failed++;
          results_.push_back({name, false, currentError_});
        }
      } catch (const std::exception &e) {
        std::cout << "[FAIL] " << name << ": Exception: " << e.what() << "\n";
        failed++;
        results_.push_back({name, false, e.what()});
      } catch (...) {
        std::cout << "[FAIL] " << name << ": Unknown exception\n";
        failed++;
        results_.push_back({name, false, "Unknown exception"});
      }
    }

    std::cout << "\n=== Results: " << passed << " passed, " << failed
              << " failed ===\n";
    return failed;
  }

  void fail(const std::string &msg) { currentError_ = msg; }

  const std::vector<TestResult> &results() const { return results_; }

private:
  std::vector<std::pair<std::string, std::function<void()>>> tests_;
  std::vector<TestResult> results_;
  std::string currentTest_;
  std::string currentError_;
};

// Test macros
#define TEST(name)                                                             \
  void test_##name();                                                          \
  static struct TestRegistrar_##name {                                         \
    TestRegistrar_##name() {                                                   \
      Hub::Test::TestRunner::instance().addTest(#name, test_##name);           \
    }                                                                          \
  } testRegistrar_##name;                                                      \
  void test_##name()

#define ASSERT_TRUE(expr)                                                      \
  if (!(expr)) {                                                               \
    Hub::Test::TestRunner::instance().fail("ASSERT_TRUE failed: " #expr);      \
    return;                                                                    \
  }

#define ASSERT_FALSE(expr)                                                     \
  if (expr) {                                                                  \
    Hub::Test::TestRunner::instance().fail("ASSERT_FALSE failed: " #expr);     \
    return;                                                                    \
  }

#define ASSERT_EQ(a, b)                                                        \
  if ((a) != (b)) {                                                            \
    Hub::Test::TestRunner::instance().fail("ASSERT_EQ failed: " #a " != " #b); \
    return;                                                                    \
  }

#define ASSERT_NE(a, b)                                                        \
  if ((a) == (b)) {                                                            \
    Hub::Test::TestRunner::instance().fail("ASSERT_NE failed: " #a " == " #b); \
    return;                                                                    \
  }

#define ASSERT_GT(a, b)                                                        \
  if (!((a) > (b))) {                                                          \
    Hub::Test::TestRunner::instance().fail("ASSERT_GT failed: " #a " <= " #b); \
    return;                                                                    \
  }

#define ASSERT_GE(a, b)                                                        \
  if (!((a) >= (b))) {                                                         \
    Hub::Test::TestRunner::instance().fail("ASSERT_GE failed: " #a " < " #b);  \
    return;                                                                    \
  }

#define ASSERT_STREQ(a, b)                                                     \
  if (std::string(a) != std::string(b)) {                                      \
    Hub::Test::TestRunner::instance().fail("ASSERT_STREQ failed");             \
    return;                                                                    \
  }

#define RUN_ALL_TESTS() Hub::Test::TestRunner::instance().run()

} // namespace Test
} // namespace Hub
