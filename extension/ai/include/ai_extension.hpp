//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_extension.hpp
//
// Multi-modal AI Extension for DuckDB
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"

namespace duckdb {

class AIExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb
