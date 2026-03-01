//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_extension.cpp
//
// AI Extension registration and loading
//===----------------------------------------------------------------------===//

#include "ai_extension.hpp"
#include "ai_functions.hpp"

#include "duckdb/main/extension/extension_loader.hpp"

namespace duckdb {

void AIExtension::Load(ExtensionLoader &loader) {
	// Register AI scalar functions
	AIFunctions::RegisterScalarFunctions(loader);

	// TODO M1: Register AI aggregate functions
	// AIAggregateFunctions::RegisterAggregateFunctions(loader);

	// TODO M2: Register AI table functions
	// AITableFunctions::RegisterTableFunctions(loader);

	// TODO M2: Register custom types (IMAGE_TYPE, EMBEDDING_TYPE)
	// loader.RegisterType(LogicalType::IMAGE_TYPE_NAME, LogicalType::IMAGE());
}

std::string AIExtension::Name() {
	return "ai";
}

std::string AIExtension::Version() const {
#ifdef EXT_VERSION_AI
	return EXT_VERSION_AI;
#else
	return "0.0.1-mvp";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(ai, loader) {
	duckdb::AIExtension extension;
	extension.Load(loader);
}

} // extern "C"
