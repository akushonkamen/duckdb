//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_similarity.cpp
//
// AI Similarity Functions - Cosine similarity for vector embeddings
// TASK-K-001: AI_join/AI_window Extension
//===----------------------------------------------------------------------===//

#include "ai_functions.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"

#include <cmath>
#include <cstdlib>

namespace duckdb {

// ============================================================================
// Helper: Cosine Similarity Calculation
// ============================================================================

static double CosineSimilarity(const float *left, const float *right, idx_t size) {
	if (size == 0) {
		return 0.0; // Empty vectors have 0 similarity
	}

	double dot_product = 0.0;
	double norm_left = 0.0;
	double norm_right = 0.0;

	for (idx_t i = 0; i < size; i++) {
		dot_product += static_cast<double>(left[i]) * static_cast<double>(right[i]);
		norm_left += static_cast<double>(left[i]) * static_cast<double>(left[i]);
		norm_right += static_cast<double>(right[i]) * static_cast<double>(right[i]);
	}

	// Avoid division by zero
	double denominator = sqrt(norm_left) * sqrt(norm_right);
	if (denominator < 1e-10) {
		return 0.0; // Both vectors are zero vectors
	}

	return dot_product / denominator;
}

// ============================================================================
// AISimilarityFunction - Scalar function for vector similarity
// ============================================================================

struct AISimilarityLocalState : public FunctionLocalState {
	explicit AISimilarityLocalState(ClientContext &context) {
	}
};

static unique_ptr<FunctionLocalState> AISimilarityInitLocalState(ExpressionState &state,
                                                                 const BoundFunctionExpression &expr,
                                                                 FunctionData *bind_data) {
	return make_uniq<AISimilarityLocalState>(expr.GetContext());
}

static void AISimilarityFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &left_vec = args.data[0]; // FLOAT[]
	auto &right_vec = args.data[1]; // FLOAT[]
	auto &model_vec = args.data[2]; // VARCHAR (unused, for logging only)

	// Get list entry types
	D_ASSERT(left_vec.GetType().id() == LogicalTypeId::LIST);
	D_ASSERT(right_vec.GetType().id() == LogicalTypeId::LIST);

	auto &left_list_type = ListType::GetChildType(left_vec.GetType());
	auto &right_list_type = ListType::GetChildType(right_vec.GetType());

	D_ASSERT(left_list_type == LogicalType::FLOAT);
	D_ASSERT(right_list_type == LogicalType::FLOAT);

	// Flatten inputs for iteration
	Vector left_list_vector(ListVector::GetEntry(left_vec), left_list_type);
	Vector right_list_vector(ListVector::GetEntry(right_vec), right_list_type);

	UnifiedVectorFormat left_list_format;
	UnifiedVectorFormat right_list_format;
	UnifiedVectorFormat left_vec_format;
	UnifiedVectorFormat right_vec_format;
	UnifiedVectorFormat model_format;

	left_list_vector.ToUnifiedFormat(args.size(), left_list_format);
	right_list_vector.ToUnifiedFormat(args.size(), right_list_format);
	left_vec.ToUnifiedFormat(args.size(), left_vec_format);
	right_vec.ToUnifiedFormat(args.size(), right_vec_format);
	model_vec.ToUnifiedFormat(args.size(), model_format);

	// Get list offsets
	auto &left_list_entries = ListVector::GetData(left_vec);
	auto &right_list_entries = ListVector::GetData(right_vec);

	// Initialize result vector
	result.SetVectorType(VectorType::FLAT_VECTOR);
	auto result_data = FlatVector::GetData<double>(result);
	auto &result_validity = FlatVector::Validity(result);

	// Process each row
	for (idx_t i = 0; i < args.size(); i++) {
		idx_t left_idx = left_vec_format.sel->get_index(i);
		idx_t right_idx = right_vec_format.sel->get_index(i);

		// Check for NULL inputs
		if (!left_vec_format.validity.RowIsValid(left_idx) ||
		    !right_vec_format.validity.RowIsValid(right_idx)) {
			result_validity.SetInvalid(i);
			continue;
		}

		// Get list entries
		auto &left_entry = left_list_entries[left_idx];
		auto &right_entry = right_list_entries[right_idx];

		// Check for NULL list entries
		if (left_entry.length == 0 || right_entry.length == 0) {
			result_validity.SetInvalid(i);
			continue;
		}

		// Validate vector dimensions
		if (left_entry.length != right_entry.length) {
			throw InvalidInputException(
			    "ai_similarity: Vector dimension mismatch - left vector has %u dimensions, right vector has %u dimensions. "
			    "Both vectors must have the same dimension for cosine similarity calculation.",
			    left_entry.length, right_entry.length);
		}

		// Get pointer to list data
		auto left_data = reinterpret_cast<const float *>(left_list_format.data) + left_entry.offset;
		auto right_data = reinterpret_cast<const float *>(right_list_format.data) + right_entry.offset;

		// Calculate cosine similarity
		double similarity = CosineSimilarity(left_data, right_data, left_entry.length);

		// Clamp to [0, 1] range
		if (similarity < 0.0) {
			similarity = 0.0;
		} else if (similarity > 1.0) {
			similarity = 1.0;
		}

		result_data[i] = similarity;
	}
}

// ============================================================================
// Register ai_similarity function
// ============================================================================

ScalarFunction AIFunctions::GetAISimilarityFunction() {
	// ai_similarity(vec1 FLOAT[], vec2 FLOAT[], model VARCHAR) -> DOUBLE
	auto func = ScalarFunction(
	    {LogicalType::LIST(LogicalType::FLOAT), LogicalType::LIST(LogicalType::FLOAT), LogicalType::VARCHAR},
	    LogicalType::DOUBLE, AISimilarityFunction);

	// Set function properties
	func.stability = FunctionStability::CONSISTENT; // Deterministic calculation
	func.null_handling = FunctionNullHandling::SPECIAL_HANDLING;
	func.init_local_state = AISimilarityInitLocalState;

	return func;
}

} // namespace duckdb
