//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_functions.hpp
//
// AI Function declarations for DuckDB
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"

namespace duckdb {

class AIFunctions {
public:
	// Register all AI scalar functions
	static void RegisterScalarFunctions(ExtensionLoader &loader);

	// AI_filter: Returns similarity score between image and text prompt
	// SQL: ai_filter(image_blob, prompt, model) -> DOUBLE
	static ScalarFunction GetAIFilterFunction();

	// AI_filter_batch: Batch processing with concurrent requests
	// SQL: ai_filter_batch(image_blob, prompt, model) -> DOUBLE
	// Uses async/concurrent execution for better throughput
	static ScalarFunction GetAIFilterBatchFunction();

	// AI_similarity: Cosine similarity between two vectors
	// SQL: ai_similarity(vec1 FLOAT[], vec2 FLOAT[], model VARCHAR) -> DOUBLE
	// Used for AI_join operations
	static ScalarFunction GetAISimilarityFunction();
};

} // namespace duckdb
