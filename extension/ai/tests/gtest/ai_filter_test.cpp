//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_filter_test.cpp - Google Test unit tests for AI Filter
//   Tests retry logic, error handling, and edge cases using mocks
//===----------------------------------------------------------------------===//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ai_function_executor.hpp"
#include "http_client_interface.hpp"
#include "mock/mock_interfaces.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

using namespace duckdb;

// ============================================================================
// Test Fixture
// ============================================================================

class AIFilterTest : public ::testing::Test {
protected:
	void SetUp() override {
		mock_http = std::make_unique<mock::MockHTTPClient>();
		mock_clock = std::make_unique<mock::MockSystemClock>();
		mock_random = std::make_unique<mock::MockRandomGenerator>();
	}

	std::unique_ptr<AIFunctionExecutor> CreateExecutor() {
		return std::make_unique<AIFunctionExecutor>(
			std::move(mock_http),
			std::move(mock_clock),
			std::move(mock_random)
		);
	}

	std::unique_ptr<mock::MockHTTPClient> mock_http;
	std::unique_ptr<mock::MockSystemClock> mock_clock;
	std::unique_ptr<mock::MockRandomGenerator> mock_random;
};

// ============================================================================
// Success Path Tests
// ============================================================================

TEST_F(AIFilterTest, SuccessfulAPIResponse) {
	// Arrange
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.85\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert
	EXPECT_EQ(result, "{\"choices\":[{\"message\":{\"content\":\"0.85\"}}]}");
}

TEST_F(AIFilterTest, SuccessfulAPIResponseWithDifferentScore) {
	// Arrange
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.92\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "dog", "clip");

	// Assert
	EXPECT_NE(result.find("0.92"), std::string::npos);
}

// ============================================================================
// Retry Logic Tests
// ============================================================================

TEST_F(AIFilterTest, RetryOnceThenSuccess) {
	// Arrange
	testing::Sequence s;
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.InSequence(s)
		.WillOnce(Return(IHTTPClient::Response{1, "{\"error\":\"timeout\"}"}));
	EXPECT_CALL(*mock_clock, SleepMs(100));  // BASE_DELAY_MS
		EXPECT_CALL(*mock_http, Post(_, _, _))
		.InSequence(s)
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.75\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert
	EXPECT_NE(result.find("0.75"), std::string::npos);
}

TEST_F(AIFilterTest, MaxRetriesExhausted) {
	// Arrange - All 4 attempts fail
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.Times(4)
		.WillRepeatedly(Return(IHTTPClient::Response{1, "{\"error\":\"timeout\"}"}));

	EXPECT_CALL(*mock_clock, SleepMs(100));  // Attempt 1
	EXPECT_CALL(*mock_clock, SleepMs(200));  // Attempt 2
	EXPECT_CALL(*mock_clock, SleepMs(400));  // Attempt 3

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert - Should return error marker
	EXPECT_NE(result.find("\"error\":\"max_retries_exceeded\""), std::string::npos);
}

// ============================================================================
// Empty Response Tests (popen failure simulation)
// ============================================================================

TEST_F(AIFilterTest, EmptyResponseThenRetry) {
	// Arrange
	testing::Sequence s;
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.InSequence(s)
		.WillOnce(Return(IHTTPClient::Response{-1, ""}));  // popen failure
	EXPECT_CALL(*mock_clock, SleepMs(100));
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.InSequence(s)
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.70\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert
	EXPECT_NE(result.find("0.70"), std::string::npos);
}

// ============================================================================
// Random Jitter Tests
// ============================================================================

TEST_F(AIFilterTest, RandomJitterInDelay) {
	// Arrange - All 4 attempts (initial + 3 retries) fail
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.Times(4)
		.WillRepeatedly(Return(IHTTPClient::Response{1, "{\"error\":\"timeout\"}"}));

	// Exponential backoff with jitter: 100*2^attempt ± jitter%
	// Retry 1: 100ms ± 20ms
	EXPECT_CALL(*mock_random, GetRandomInt(-20, 20))
		.WillOnce(Return(10));
	EXPECT_CALL(*mock_clock, SleepMs(110));  // 100 + 10

	// Retry 2: 200ms ± 40ms
	EXPECT_CALL(*mock_random, GetRandomInt(-40, 40))
		.WillOnce(Return(-20));
	EXPECT_CALL(*mock_clock, SleepMs(180));  // 200 - 20

	// Retry 3: 400ms ± 80ms
	EXPECT_CALL(*mock_random, GetRandomInt(-80, 80))
		.WillOnce(Return(50));
	EXPECT_CALL(*mock_clock, SleepMs(450));  // 400 + 50

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert
	EXPECT_NE(result.find("\"error\":\"max_retries_exceeded\""), std::string::npos);
}

// ============================================================================
// Different Model Tests
// ============================================================================

TEST_F(AIFilterTest, DifferentModelNames) {
	// Arrange
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.88\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "chatgpt-4o-latest");

	// Assert
	EXPECT_NE(result.find("0.88"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(AIFilterTest, EmptyImage) {
	// Arrange
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.WillOnce(Return(IHTTPClient::Response{0, "{\"choices\":[{\"message\":{\"content\":\"0.50\"}}]}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("", "cat", "clip");

	// Assert
	EXPECT_FALSE(result.empty());
}

TEST_F(AIFilterTest, ResponseWithInvalidJSON) {
	// Arrange
	EXPECT_CALL(*mock_http, Post(_, _, _))
		.WillOnce(Return(IHTTPClient::Response{0, "{\"data\":\"value with 0.75 inside\"}"}));

	auto executor = CreateExecutor();

	// Act
	auto result = executor->CallAPIWithRetry("base64_image", "cat", "clip");

	// Assert
	EXPECT_FALSE(result.empty());
	EXPECT_NE(result.find("0.75"), std::string::npos);
}

// ============================================================================
// Main function
// ============================================================================

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
