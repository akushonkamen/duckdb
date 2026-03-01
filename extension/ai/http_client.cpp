//===----------------------------------------------------------------------===//
//                         HTTP Client Implementation
//===----------------------------------------------------------------------===//

#include "http_client.hpp"
#include <httplib.hpp>
#include <sstream>
#include <iostream>

namespace duckdb {
namespace ai {

HttpClient::HttpClient(const std::string& base_url, int timeout_sec)
    : base_url_(base_url), timeout_sec_(timeout_sec) {
    parse_url();
}

void HttpClient::parse_url() {
    // Parse URL like "http://localhost:8000" or "https://api.example.com"
    size_t proto_end = base_url_.find("://");
    if (proto_end == std::string::npos) {
        throw HTTPException("Invalid URL: missing protocol (http:// or https://)");
    }

    std::string proto = base_url_.substr(0, proto_end);
    use_https_ = (proto == "https");

    size_t host_start = proto_end + 3;
    size_t port_start = base_url_.find(':', host_start);
    size_t path_start = base_url_.find('/', host_start);

    if (port_start != std::string::npos && (path_start == std::string::npos || port_start < path_start)) {
        // URL has port
        host_ = base_url_.substr(host_start, port_start - host_start);
        size_t port_end = (path_start != std::string::npos) ? path_start : base_url_.length();
        std::string port_str = base_url_.substr(port_start + 1, port_end - port_start - 1);
        port_ = std::stoi(port_str);
    } else {
        // No port specified, use default
        if (path_start != std::string::npos) {
            host_ = base_url_.substr(host_start, path_start - host_start);
        } else {
            host_ = base_url_.substr(host_start);
        }
        port_ = use_https_ ? 443 : 80;
    }
}

HTTPResponse HttpClient::post(const std::string& endpoint, const std::string& json_body) {
    try {
        return make_request(endpoint, json_body);
    } catch (const std::exception& e) {
        HTTPResponse response;
        response.status_code = -1;
        response.error = std::string("HTTP request failed: ") + e.what();
        return response;
    }
}

HTTPResponse HttpClient::make_request(const std::string& endpoint, const std::string& json_body) {
    HTTPResponse response;

    try {
        if (use_https_) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            httplib::SSLClient cli(host_.c_str(), port_);
            cli.set_timeout_sec(timeout_sec_);
            cli.set_read_timeout_sec(timeout_sec_);
            cli.set_write_timeout_sec(timeout_sec_);

            // Enable for testing only (disable certificate verification)
            cli.enable_server_certificate_verification(false);

            auto res = cli.Post(endpoint.c_str(), json_body.c_str(), "application/json");
            if (res) {
                response.status_code = res->status;
                response.body = res->body;
            } else {
                response.status_code = -1;
                response.error = "No response from server";
            }
#else
            response.status_code = -1;
            response.error = "HTTPS support not compiled in";
#endif
        } else {
            httplib::Client cli(host_.c_str(), port_);
            cli.set_timeout_sec(timeout_sec_);
            cli.set_read_timeout_sec(timeout_sec_);
            cli.set_write_timeout_sec(timeout_sec_);

            auto res = cli.Post(endpoint.c_str(), json_body.c_str(), "application/json");
            if (res) {
                response.status_code = res->status;
                response.body = res->body;
            } else {
                response.status_code = -1;
                response.error = "No response from server";
            }
        }
    } catch (const std::exception& e) {
        response.status_code = -1;
        response.error = std::string("HTTP client error: ") + e.what();
    }

    return response;
}

void HttpClient::set_timeout_sec(int seconds) {
    timeout_sec_ = seconds;
}

} // namespace ai
} // namespace duckdb
