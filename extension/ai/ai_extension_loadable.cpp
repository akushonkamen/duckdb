//===----------------------------------------------------------------------===//
//                         DuckDB AI Extension (MVP)
//                         Minimal Test Version
//===----------------------------------------------------------------------===//

#include "duckdb.hpp"

using namespace duckdb;

// Simplest possible test function - returns constant 0.42
// Uses proper DataChunk vectorized execution pattern
static void ai_filter_test(DataChunk &args, ExpressionState &state, Vector &result) {
	// Return constant 0.42 for all rows
	result.SetVectorType(VectorType::CONSTANT_VECTOR);
	auto result_data = FlatVector::GetData<double>(result);
	result_data[0] = 0.42;
}

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(ai, loader) {
	// Register function directly using loader
	ScalarFunction ai_filter_func("ai_filter", {}, LogicalType::DOUBLE, &ai_filter_test);
	ai_filter_func.SetStability(FunctionStability::CONSISTENT);
	loader.RegisterFunction(ai_filter_func);
}

}