cmake_minimum_required(VERSION 3.15)

project(AIExtension)

# Include directories
include_directories(include)

# Source files for AI Extension
set(AI_EXTENSION_FILES
    src/ai_extension.cpp
    src/ai_filter.cpp
)

# Build static extension (for linking into DuckDB)
build_static_extension(ai ${AI_EXTENSION_FILES})

# Build loadable extension (standalone .duckdb_extension file)
set(PARAMETERS "-warnings")
build_loadable_extension(ai ${PARAMETERS} ${AI_EXTENSION_FILES})

# Install extension
install(
  TARGETS ai_extension
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
