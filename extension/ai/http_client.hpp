//===----------------------------------------------------------------------===//
//                         HTTP Client for AI Extension
//                         Wraps httplib for AI service calls
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <stdexcept>
#include <chrono>

namespace duckdb {
namespace ai {

/**
 * HTTP response structure
 */
struct HTTPResponse {
    int status_code;
    std::string body;
    std::string error;

    bool is_success() const { return status_code >= 200 && status_code < 300; }
};

/**
 * HTTP Client for AI service calls
 * Wraps httplib with error handling and timeout configuration
 */
class HttpClient {
public:
    /**
     * Construct HTTP client
     * @param base_url Base URL of AI service (e.g., "http://localhost:8000")
     * @param timeout_sec Connection timeout in seconds (default: 5)
     */
    HttpClient(const std::string& base_url, int timeout_sec = 5);

    /**
     * Make POST request to AI service
     * @param endpoint API endpoint (e.g., "/api/v1/similarity")
     * @param json_body JSON request body
     * @return HTTPResponse with status code and body
     */
    HTTPResponse post(const std::string& endpoint, const std::string& json_body);

    /**
     * Set connection timeout
     */
    void set_timeout_sec(int seconds);

    /**
     * Get base URL
     */
    const std::string& get_base_url() const { return base_url_; }

private:
    std::string base_url_;
    std::string host_;
    int port_;
    bool use_https_;
    int timeout_sec_;

    /**
     * Parse URL into host and port
     */
    void parse_url();

    /**
     * Make HTTP request using httplib
     */
    HTTPResponse make_request(const std::string& endpoint, const std::string& json_body);
};

/**
 * Exception for HTTP errors
 */
class HTTPException : public std::runtime_error {
public:
    HTTPException(const std::string& message) : std::runtime_error(message) {}
};

} // namespace ai
} // namespace duckdb
