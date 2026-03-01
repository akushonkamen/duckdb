//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_filter.cpp
//
// AI_filter ScalarFunction implementation
//===----------------------------------------------------------------------===//

#include "ai_functions.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/execution/expression_executor.hpp"

#include <random>
#include <ctime>

namespace duckdb {

// ============================================================================
// AI_filter: Mock AI Similarity Filter Function
// ============================================================================
// M0-M1: Returns random similarity score 0.0-1.0
// M2: Will call actual HTTP AI inference service
//
// SQL: ai_filter(image_blob, prompt, model) -> DOUBLE
// Example: SELECT ai_filter(image_data, 'a cat', 'clip') FROM images;
// ============================================================================

struct AIFilterLocalState : public FunctionLocalState {
	std::mt19937 rng;
	std::uniform_real_distribution<double> dist;

	AIFilterLocalState(uint64_t seed) : rng(seed), dist(0.0, 1.0) {
	}
};

static void AIFilterFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &lstate = ExecuteFunctionState::GetFunctionState(state)->Cast<AIFilterLocalState>();

	auto &image_vector = args.data[0];  // BLOB
	auto &prompt_vector = args.data[1]; // VARCHAR
	// Model is in args.data[2] but not used in MVP

	// M1 MVP: Generate random similarity scores
	// In M2, this will make actual HTTP calls to AI service
	BinaryExecutor::Execute<string_t, string_t, double>(
	    image_vector, prompt_vector, result, args.size(),
	    [&](string_t image, string_t prompt) {
		    // M0-M1: Mock implementation - return random score
		    // M2: TODO - Replace with actual HTTP call to AI service
		    double score = lstate.dist(lstate.rng);

		    // Log what would be sent to AI service (for debugging)
		    // In M2, this will be actual HTTP request:
		    // POST /api/v1/similarity
		    // Body: {"image": base64(image), "prompt": prompt, "model": model}
		    return score;
	    });

	// Mark result as constant if all inputs are constant
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

unique_ptr<FunctionLocalState> AIFilterInitLocalState(ExpressionState &state, const BoundFunctionExpression &expr,
                                                      FunctionData *bind_data) {
	auto &random_engine = RandomEngine::Get(state.GetContext());
	lock_guard<mutex> guard(random_engine.lock);
	return make_uniq<AIFilterLocalState>(random_engine.NextRandomInteger64());
}

ScalarFunction AIFunctions::GetAIFilterFunction() {
	// ai_filter(image_blob BLOB, prompt VARCHAR, model VARCHAR) -> DOUBLE
	ScalarFunction ai_filter_function("ai_filter",
	                                  {LogicalType::BLOB, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                  LogicalType::DOUBLE, AIFilterFunction,
	                                  nullptr,  // bind
	                                  nullptr,  // dependencies
	                                  nullptr,  // function data
	                                  AIFilterInitLocalState);  // init local state

	// Mark as volatile since results are random
	ai_filter_function.SetStability(FunctionStability::VOLATILE);

	return ai_filter_function;
}

void AIFunctions::RegisterScalarFunctions(ExtensionLoader &loader) {
	// Register AI_filter function
	loader.RegisterFunction(GetAIFilterFunction());

	// TODO M1: Add more AI functions
	// loader.RegisterFunction(GetAITransformFunction());
	// loader.RegisterFunction(GetAIClusterFunction());
}

} // namespace duckdb
