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
};

} // namespace duckdb
