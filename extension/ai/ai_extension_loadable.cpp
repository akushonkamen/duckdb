//===----------------------------------------------------------------------===//
//                         DuckDB AI Extension (M3)
//                         HTTP-based AI Filter Implementation (MVP)
//===----------------------------------------------------------------------===//

#include "duckdb.hpp"
#include <string>
#include <sstream>
#include <cstring>

using namespace duckdb;

// Configuration for M3 MVP
static std::string g_ai_service_url = "http://localhost:8000";

/**
 * Simple HTTP client for MVP (M3)
 * Note: This is a simplified implementation for M3.
 * Full HTTP client with httplib integration will be added in M4.
 */
namespace duckdb {
namespace ai {

struct HTTPResponse {
    int status_code;
    std::string body;
    std::string error;

    bool is_success() const { return status_code >= 200 && status_code < 300; }
};

class HttpClient {
public:
    HttpClient(const std::string& base_url, int timeout_sec = 5)
        : base_url_(base_url), timeout_sec_(timeout_sec) {}

    HTTPResponse post(const std::string& endpoint, const std::string& json_body) {
        HTTPResponse response;

        // M3 MVP: Simulate HTTP call with mock response
        // In production, this would make actual HTTP request using httplib

        // Parse request to extract image data
        std::string image = extract_json_field(json_body, "image");
        std::string prompt = extract_json_field(json_body, "prompt");

        // Generate mock similarity score based on prompt
        // In production, this will call actual AI service
        double score = generate_mock_score(prompt);

        // Build mock JSON response
        std::ostringstream response_body;
        response_body << "{";
        response_body << "\"score\": " << score << ",";
        response_body << "\"latency_ms\": 50,";
        response_body << "\"model\": \"clip\",";
        response_body << "\"mock\": true";
        response_body << "}";

        response.status_code = 200;
        response.body = response_body.str();

        return response;
    }

private:
    std::string base_url_;
    int timeout_sec_;

    std::string extract_json_field(const std::string& json, const std::string& field) {
        // Simple JSON parsing for MVP
        std::string search = "\"" + field + "\":\"";
        size_t pos = json.find(search);
        if (pos != std::string::npos) {
            size_t start = pos + search.length();
            size_t end = json.find("\"", start);
            if (end != std::string::npos) {
                return json.substr(start, end - start);
            }
        }
        return "";
    }

    double generate_mock_score(const std::string& prompt) {
        // Generate deterministic mock score based on prompt
        // In production, this will be replaced with actual AI inference
        double score = 0.5;
        for (char c : prompt) {
            score = (score * 31.0 + c) / 32.0;
        }
        return score;
    }
};

} // namespace ai
} // namespace duckdb

// Global HTTP client
static unique_ptr<ai::HttpClient> g_http_client = nullptr;

/**
 * Initialize HTTP client if not already initialized
 */
static void ensure_client_initialized() {
    if (!g_http_client) {
        g_http_client = make_uniq<ai::HttpClient>(g_ai_service_url, 10);
    }
}

/**
 * Parse JSON response and extract score
 */
static double parse_score_from_response(const std::string& json_body) {
    // Simple JSON parsing for MVP
    std::string search = "\"score\":";
    size_t pos = json_body.find(search);
    if (pos != std::string::npos) {
        size_t start = pos + search.length();
        size_t end = json_body.find_first_of(",}", start);
        if (end != std::string::npos) {
            std::string score_str = json_body.substr(start, end - start);
            try {
                return std::stod(score_str);
            } catch (...) {
                return 0.5;
            }
        }
    }
    return 0.5;
}

/**
 * AI Filter function - makes HTTP call to AI service
 * Parameters: image (VARCHAR), prompt (VARCHAR), model (VARCHAR)
 * Returns: similarity score (DOUBLE)
 */
static void ai_filter_function(DataChunk &args, ExpressionState &state, Vector &result) {
    // Ensure HTTP client is initialized
    ensure_client_initialized();

    auto &image_vector = args.data[0];
    auto &prompt_vector = args.data[1];
    auto &model_vector = args.data[2];

    // Initialize result vector
    result.SetVectorType(VectorType::FLAT_VECTOR);
    auto result_data = FlatVector::GetData<double>(result);

    // Get number of rows
    const idx_t count = args.size();

    // Process each row
    UnifiedVectorFormat image_data;
    UnifiedVectorFormat prompt_data;
    UnifiedVectorFormat model_data;

    image_vector.ToUnifiedFormat(count, image_data);
    prompt_vector.ToUnifiedFormat(count, prompt_data);
    model_vector.ToUnifiedFormat(count, model_data);

    for (idx_t i = 0; i < count; i++) {
        // Check for NULL values
        idx_t image_idx = image_data.sel->get_index(i);
        idx_t prompt_idx = prompt_data.sel->get_index(i);

        if (!image_data.validity.RowIsValid(image_idx) ||
            !prompt_data.validity.RowIsValid(prompt_idx)) {
            FlatVector::SetNull(result, i, true);
            continue;
        }

        // Extract parameters
        auto image_val = FlatVector::GetData<string_t>(image_vector)[i];
        auto prompt_val = FlatVector::GetData<string_t>(prompt_vector)[i];

        std::string image_str(image_val.GetData(), image_val.GetSize());
        std::string prompt_str(prompt_val.GetData(), prompt_val.GetSize());

        // Get model (default to "clip" if NULL)
        std::string model_str = "clip";
        idx_t model_idx = model_data.sel->get_index(i);
        if (model_data.validity.RowIsValid(model_idx)) {
            auto model_val = FlatVector::GetData<string_t>(model_vector)[i];
            model_str = std::string(model_val.GetData(), model_val.GetSize());
        }

        // Make HTTP request
        try {
            // Build JSON request manually (simpler for MVP)
            std::ostringstream json_str;
            json_str << "{";
            json_str << "\"image\":\"" << image_str << "\",";
            json_str << "\"prompt\":\"" << prompt_str << "\",";
            json_str << "\"model\":\"" << model_str << "\"";
            json_str << "}";

            std::string json_payload = json_str.str();

            auto response = g_http_client->post("/api/v1/similarity", json_payload);

            if (response.is_success()) {
                result_data[i] = parse_score_from_response(response.body);
            } else {
                // On HTTP error, return default score
                result_data[i] = 0.5;
            }
        } catch (const std::exception& e) {
            // On exception, return default score
            result_data[i] = 0.5;
        }
    }

    // Verify result
    result.Verify(count);
}

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(ai, loader) {
    // Register ai_filter function with 3 parameters
    ScalarFunction ai_filter_func(
        "ai_filter",
        {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
        LogicalType::DOUBLE,
        &ai_filter_function
    );
    ai_filter_func.SetStability(FunctionStability::VOLATILE);
    loader.RegisterFunction(ai_filter_func);
}

}
