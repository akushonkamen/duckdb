//===----------------------------------------------------------------------===//
//                         DuckDB AI Extension (MVP)
//                         Loadable Extension Demo
//===----------------------------------------------------------------------===//
// M1 MVP: Mock AI similarity filter
// SQL: ai_filter(image_blob, prompt, model) -> DOUBLE
// Returns: Random similarity score 0.0-1.0
// M2: Will integrate actual HTTP AI service calls
//===----------------------------------------------------------------------===//

#include "duckdb.hpp"
#include "duckdb/parser/parsed_data/create_scalar_function_info.hpp"

#include <random>
#include <ctime>

using namespace duckdb;

//===--------------------------------------------------------------------===//
// AI Filter Function (MVP - Mock Implementation)
//===--------------------------------------------------------------------===//

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
	// Model parameter is in args.data[2] but not used in MVP

	// M1 MVP: Generate random similarity scores
	// In M2, this will make actual HTTP calls to AI service
	BinaryExecutor::Execute<string_t, string_t, double>(
	    image_vector, prompt_vector, result, args.size(),
	    [&](string_t image, string_t prompt) {
		    // M0-M1: Mock implementation - return random score
		    // M2: TODO - Replace with actual HTTP call to AI service
		    double score = lstate.dist(lstate.rng);
		    return score;
	    });

	// Mark result as constant if all inputs are constant
	if (args.AllConstant()) {
		result.SetVectorType(VectorType::CONSTANT_VECTOR);
	}
}

static unique_ptr<FunctionLocalState> AIFilterInitLocalState(ExpressionState &state, const BoundFunctionExpression &expr,
                                                            FunctionData *bind_data) {
	auto &random_engine = RandomEngine::Get(state.GetContext());
	lock_guard<mutex> guard(random_engine.lock);
	return make_uniq<AIFilterLocalState>(random_engine.NextRandomInteger64());
}

//===--------------------------------------------------------------------===//
// Extension Load Entry Point
//===--------------------------------------------------------------------===//

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(ai, loader) {
	auto &db = loader.GetDatabaseInstance();
	Connection con(db);
	auto &client_context = *con.context;
	auto &catalog = Catalog::GetSystemCatalog(client_context);

	con.BeginTransaction();

	// Register AI_filter scalar function
	ScalarFunction ai_filter_func("ai_filter",
	                              {LogicalType::BLOB, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                              LogicalType::DOUBLE, AIFilterFunction);
	ai_filter_func.SetInitStateCallback(AIFilterInitLocalState);
	ai_filter_func.SetStability(FunctionStability::VOLATILE);

	CreateScalarFunctionInfo ai_filter_info(ai_filter_func);
	auto ai_filter_entry = catalog.CreateFunction(client_context, ai_filter_info);
	ai_filter_entry->tags["ext:name"] = "ai";
	ai_filter_entry->tags["ext:version"] = "0.0.1-mvp";
	ai_filter_entry->tags["ext:author"] = "DuckDB AI Team";

	con.Commit();

	// Log successful load
	// (In production, use proper logging)
}

} // extern "C"
