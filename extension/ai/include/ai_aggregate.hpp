//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_aggregate.hpp
//
// AI Aggregate Function declarations for DuckDB
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"

namespace duckdb {

class AIAggregateFunctions {
public:
	// Register all AI aggregate functions
	static void RegisterAggregateFunctions(ExtensionLoader &loader);

	// ai_similarity_avg: Average similarity score for a group of images
	// SQL: ai_similarity_avg(image_blob, prompt, model) -> DOUBLE
	//
	// Example:
	//   SELECT product_id, ai_similarity_avg(image, 'product quality', 'clip')
	//   FROM products
	//   GROUP BY product_id;
	//
	// Returns the average similarity score across all images in the group.
	static AggregateFunctionSet GetAISimilarityAvgFunction();
};

} // namespace duckdb
