//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_aggregate.cpp
//
// AI Aggregate Functions - Similarity statistics for grouped data
//===----------------------------------------------------------------------===//

#include "ai_aggregate.hpp"
#include "ai_functions.hpp"
#include "ai_function_executor.hpp"

namespace duckdb {

// ============================================================================
// Helper: Extract score from AI response
// ============================================================================

static double ExtractAIResponseScore(const std::string &json_response) {
	constexpr double DEFAULT_DEGRADATION_SCORE = 0.5;

	// Check for retry exhaustion marker
	if (json_response.find("\"error\":\"max_retries_exceeded\"") != std::string::npos) {
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
	if (content_pos != std::string::npos) {
		size_t colon_pos = json_response.find(":", content_pos);
		if (colon_pos != std::string::npos) {
			size_t quote_start = json_response.find("\"", colon_pos);
			if (quote_start != std::string::npos) {
				quote_start++;
				size_t quote_end = json_response.find("\"", quote_start);
				if (quote_end != std::string::npos) {
					std::string value_str = json_response.substr(quote_start, quote_end - quote_start);
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
			while (start > 0 && json_response[start - 1] == '.') {
				start--;
			}
			size_t end = i + 2;
			while (end < json_response.length() &&
			       ((json_response[end] >= '0' && json_response[end] <= '9') || json_response[end] == '.')) {
				end++;
			}
			std::string num_str = json_response.substr(start, end - start);
			try {
				double score = std::stod(num_str);
				if (score < 0.0) score = 0.0;
				if (score > 1.0) score = 1.0;
				return score;
			} catch (...) {
				// Continue searching
			}
		}
	}

	// Default degradation score if no number found
	return DEFAULT_DEGRADATION_SCORE;
}

// ============================================================================
// ai_similarity_avg - Average similarity score for a group of images
// ============================================================================

struct AISimilarityAvgState {
	uint64_t count;
	double sum;

	void Initialize() {
		count = 0;
		sum = 0.0;
	}

	void Combine(const AISimilarityAvgState &other) {
		count += other.count;
		sum += other.sum;
	}
};

struct AISimilarityAvgBindData : public FunctionData {
	std::string prompt;
	std::string model;

	AISimilarityAvgBindData() = default;
	explicit AISimilarityAvgBindData(std::string prompt_p, std::string model_p)
	    : prompt(std::move(prompt_p)), model(std::move(model_p)) {
	}

	unique_ptr<FunctionData> Copy() const override {
		return make_uniq<AISimilarityAvgBindData>(prompt, model);
	}

	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<AISimilarityAvgBindData>();
		return prompt == other.prompt && model == other.model;
	}
};

struct AISimilarityAvgOperation {
	template <class STATE>
	static void Initialize(STATE &state) {
		state.Initialize();
	}

	template <class STATE, class OP>
	static void Combine(const STATE &source, STATE &target, AggregateInputData &) {
		target.count += source.count;
		target.sum += source.sum;
	}

	template <class STATE>
	static void AddValues(STATE &state, idx_t count) {
		state.count += count;
	}

	static bool IgnoreNull() {
		return true;
	}

	// Process a single value - call AI API and get similarity score
	template <class INPUT_TYPE, class STATE, class OP>
	static void Operation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &unary_input) {
		auto image = input;

		std::string image_str(image.GetString());
		std::string prompt_str;
		std::string model_str;

		if (unary_input.input.bind_data) {
			auto &data = unary_input.input.bind_data->Cast<AISimilarityAvgBindData>();
			prompt_str = data.prompt;
			model_str = data.model;
		} else {
			prompt_str = "Describe the content of this image";
			model_str = "chatgpt-4o-latest";
		}

		// Call AI API
		auto response = duckdb::AIFunctionExecutor::GetGlobalExecutor().CallAPIWithRetry(
		    image_str, prompt_str, model_str);

		// Extract score
		double score = ExtractAIResponseScore(response);

		// Update state
		state.sum += score;
		state.count++;
	}

	// For constant values (e.g., same image for multiple rows)
	template <class INPUT_TYPE, class STATE, class OP>
	static void ConstantOperation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &unary_input,
	                              idx_t count) {
		for (idx_t i = 0; i < count; i++) {
			Operation<INPUT_TYPE, STATE, OP>(state, input, unary_input);
		}
	}

	template <class T, class STATE>
	static void Finalize(STATE &state, T &target, AggregateFinalizeData &finalize_data) {
		if (state.count == 0) {
			finalize_data.ReturnNull();
		} else {
			target = state.sum / state.count;
		}
	}
};

AggregateFunctionSet AIAggregateFunctions::GetAISimilarityAvgFunction() {
	AggregateFunctionSet avg_set;

	// BLOB input (image) -> DOUBLE output (similarity score)
	auto function = AggregateFunction::UnaryAggregate<AISimilarityAvgState, string_t, double,
	                                                    AISimilarityAvgOperation>(
	                    LogicalType::BLOB, LogicalType::DOUBLE);

	// Bind function to extract prompt and model parameters
	function.bind = [](ClientContext &context, AggregateFunction &function,
	                      vector<unique_ptr<Expression>> &arguments) -> unique_ptr<FunctionData> {
		if (arguments.size() < 2) {
			throw BinderException("ai_similarity_avg requires at least 2 arguments: image and prompt");
		}

		// Get default model if not provided
		std::string model_str;
		if (arguments.size() >= 3) {
			model_str = arguments[2]->ToString();
		}
		if (model_str.empty()) {
			model_str = "chatgpt-4o-latest";
		}

		auto prompt_str = arguments[1]->ToString();
		return make_uniq<AISimilarityAvgBindData>(prompt_str, model_str);
	};

	avg_set.AddFunction(function);
	return avg_set;
}

// ============================================================================
// Register all AI aggregate functions
// ============================================================================

void AIAggregateFunctions::RegisterAggregateFunctions(ExtensionLoader &loader) {
	// Register ai_similarity_avg
	loader.RegisterFunction(GetAISimilarityAvgFunction());
}

} // namespace duckdb
