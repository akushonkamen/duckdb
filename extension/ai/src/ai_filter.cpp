//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_filter.cpp - Enhanced with Retry Logic, Error Handling, and DI
//   TASK-OPS-001: 错误处理和重试机制
//   TASK-COV-001: Dependency Injection for Google Mock testing
//===----------------------------------------------------------------------===//

#include "ai_functions.hpp"
#include "http_client_interface.hpp"
#include "ai_function_executor.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/execution/expression_executor.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <future>
#include <ctime>
#include <cstdlib>

namespace duckdb {

// ============================================================================
// Configuration
// ============================================================================

static constexpr char DEFAULT_MODEL[] = "chatgpt-4o-latest";
static constexpr double DEFAULT_DEGRADATION_SCORE = 0.5;

// ============================================================================
// Legacy static functions (backward compatibility)
// ============================================================================

static int GetRetryDelayMs(int attempt) {
	// Exponential backoff: BASE_DELAY_MS * 2^attempt
	int delay = BASE_DELAY_MS * (1 << attempt);
	// Cap at MAX_DELAY_MS
	delay = std::min(delay, MAX_DELAY_MS);
	// Add jitter (±20%)
	int jitter = delay / 5;
	delay += (std::rand() % (2 * jitter + 1)) - jitter;
	// Ensure non-negative
	return std::max(0, delay);
}

static string CallAI_API_WithRetry(const string &image, const string &prompt, const string &model) {
	return GetGlobalExecutor().CallAPIWithRetry(image, prompt, model);
}

// ============================================================================
// Extract Score with Enhanced Error Handling
// ============================================================================

static double ExtractScore(const string &json_response) {
	// Check for retry exhaustion marker (TASK-OPS-001)
	if (json_response.find("\"error\":\"max_retries_exceeded\"") != std::string::npos) {
		fprintf(stderr, "[AI_FILTER] All retries exhausted, using degradation score\\n");
		// Check environment variable for custom default score
		if (const char* env_default = std::getenv("AI_FILTER_DEFAULT_SCORE")) {
			try {
				return std::stod(env_default);
			} catch (...) {
				return DEFAULT_DEGRADATION_SCORE;
			}
		}
		return DEFAULT_DEGRADATION_SCORE;
	}

	// Strategy 1: Look for "content": "number" pattern
	size_t content_pos = json_response.find("\"content\":");
	if (content_pos != string::npos) {
		size_t colon_pos = json_response.find(":", content_pos);
		if (colon_pos != string::npos) {
			size_t quote_start = json_response.find("\"", colon_pos);
			if (quote_start != string::npos) {
				quote_start++;
				size_t quote_end = json_response.find("\"", quote_start);
				if (quote_end != string::npos) {
					string value_str = json_response.substr(quote_start, quote_end - quote_start);
					try {
						double score = std::stod(value_str);
						if (score < 0.0) score = 0.0;
						if (score > 1.0) score = 1.0;
						return score;
					} catch (...) {
						// Fall through to next strategy
					}
				}
			}
		}
	}

	// Strategy 2: Search for any decimal number in the response
	for (size_t i = 0; i < json_response.length() - 3; i++) {
		if (json_response[i] >= '0' && json_response[i] <= '9' && json_response[i + 1] == '.') {
			size_t start = i;
			size_t end = i + 2;
			while (end < json_response.length() &&
			       ((json_response[end] >= '0' && json_response[end] <= '9') || json_response[end] == '.')) {
				end++;
			}
			string num_str = json_response.substr(start, end - start);
			try {
				double score = std::stod(num_str);
				if (score >= 0.0 && score <= 1.0) {
					return score;
				}
			} catch (...) {
				// Continue searching
			}
		}
	}

	return DEFAULT_DEGRADATION_SCORE;
}

// ============================================================================
// AI Filter Function
// ============================================================================

static void AIFilterFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &image_vector = args.data[0];
	auto &prompt_vector = args.data[1];
	auto &model_vector = args.data[2];

	TernaryExecutor::Execute<string_t, string_t, string_t, double>(
	    image_vector, prompt_vector, model_vector, result, args.size(),
	    [&](string_t image, string_t prompt, string_t model) {
		    string image_str = image.GetString();
		    string prompt_str = prompt.GetString();
		    string model_str = model.GetString();

		    if (model_str.empty()) {
			    model_str = DEFAULT_MODEL;
		    }

		    string response = CallAI_API_WithRetry(image_str, prompt_str, model_str);
		    double score = ExtractScore(response);

		    return score;
	    });

	result.SetVectorType(VectorType::FLAT_VECTOR);
}

// ============================================================================
// Batch Processing
// ============================================================================

struct BatchConfig {
	static constexpr size_t DEFAULT_BATCH_SIZE = 10;
	static constexpr size_t MAX_CONCURRENT = 5;
	static constexpr int REQUEST_TIMEOUT_SEC = 30;
};

struct BatchResult {
	size_t index;
	double score;
};

static BatchResult ProcessSingleImage(size_t idx, const string &image, const string &prompt, const string &model) {
	string response = CallAI_API_WithRetry(image, prompt, model);
	double score = ExtractScore(response);
	return BatchResult{idx, score};
}

static void AIFilterBatchFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &image_vector = args.data[0];
	auto &prompt_vector = args.data[1];
	auto &model_vector = args.data[2];

	size_t row_count = args.size();

	UnifiedVectorFormat image_format;
	UnifiedVectorFormat prompt_format;
	UnifiedVectorFormat model_format;

	image_vector.ToUnifiedFormat(row_count, image_format);
	prompt_vector.ToUnifiedFormat(row_count, prompt_format);
	model_vector.ToUnifiedFormat(row_count, model_format);

	vector<string> images(row_count);
	vector<string> prompts(row_count);
	vector<string> models(row_count);

	for (size_t i = 0; i < row_count; i++) {
		auto image_idx = image_format.sel->get_index(i);
		if (!image_format.validity.RowIsValid(image_idx)) {
			images[i] = "";
		} else {
			auto image_val = ((string_t *)image_format.data)[image_idx];
			images[i] = image_val.GetString();
		}

		auto prompt_idx = prompt_format.sel->get_index(i);
		if (!prompt_format.validity.RowIsValid(prompt_idx)) {
			prompts[i] = "";
		} else {
			auto prompt_val = ((string_t *)prompt_format.data)[prompt_idx];
			prompts[i] = prompt_val.GetString();
		}

		auto model_idx = model_format.sel->get_index(i);
		if (!model_format.validity.RowIsValid(model_idx)) {
			models[i] = DEFAULT_MODEL;
		} else {
			auto model_val = ((string_t *)model_format.data)[model_idx];
			string model_str = model_val.GetString();
			models[i] = model_str.empty() ? DEFAULT_MODEL : model_str;
		}
	}

	vector<double> scores(row_count, DEFAULT_DEGRADATION_SCORE);
	vector<std::future<BatchResult>> futures;

	for (size_t i = 0; i < row_count; i++) {
		if (images[i].empty() || prompts[i].empty()) {
			scores[i] = DEFAULT_DEGRADATION_SCORE;
			continue;
		}

		futures.push_back(std::async(std::launch::async, ProcessSingleImage, i, images[i], prompts[i], models[i]));

		if (futures.size() >= BatchConfig::MAX_CONCURRENT) {
			for (auto &f : futures) {
				BatchResult res = f.get();
				scores[res.index] = res.score;
			}
			futures.clear();
		}
	}

	for (auto &f : futures) {
		BatchResult res = f.get();
		scores[res.index] = res.score;
	}

	auto result_data = FlatVector::GetData<double>(result);
	for (size_t i = 0; i < row_count; i++) {
		result_data[i] = scores[i];
	}

	result.SetVectorType(VectorType::FLAT_VECTOR);
}

// ============================================================================
// Function Registration
// ============================================================================

ScalarFunction AIFunctions::GetAIFilterFunction() {
	ScalarFunction ai_filter_function("ai_filter",
	                                  {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                  LogicalType::DOUBLE, AIFilterFunction);
	ai_filter_function.SetStability(FunctionStability::VOLATILE);
	return ai_filter_function;
}

ScalarFunction AIFunctions::GetAIFilterBatchFunction() {
	ScalarFunction ai_filter_batch_function("ai_filter_batch",
	                                        {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                        LogicalType::DOUBLE, AIFilterBatchFunction);
	ai_filter_batch_function.SetStability(FunctionStability::VOLATILE);
	return ai_filter_batch_function;
}

void AIFunctions::RegisterScalarFunctions(ExtensionLoader &loader) {
	loader.RegisterFunction(GetAIFilterFunction());
	loader.RegisterFunction(GetAIFilterBatchFunction());
	loader.RegisterFunction(GetAISimilarityFunction());
}

} // namespace duckdb
