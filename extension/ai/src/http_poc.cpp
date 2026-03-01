/**
 * HTTP 调用 PoC - 验证 httplib 在 DuckDB Extension 中的可用性
 *
 * 目标：
 * 1. 验证 httplib 可以链接
 * 2. 验证基本的 HTTP GET 请求
 * 3. 验证超时控制
 *
 * M0 阶段：仅验证可行性，不实现完整功能
 *
 * 注意：DuckDB 的 httplib 已被 patch，需要 DuckDB 头文件支持
 * 实际使用时需要在 Extension 环境中编译
 */

// M0 阶段：先验证库文件存在性，实际集成将在 M1 完成
// #include "httplib.hpp"
#include <iostream>
#include <string>

namespace duckdb {
namespace ai_extension {

/**
 * 简单的 HTTP GET 包装器
 * 用于验证 httplib 可用性
 */
struct HTTPResult {
    int status_code;
    std::string body;
    bool success;
};

/**
 * 执行 HTTP GET 请求
 *
 * @param url 完整 URL（如 http://example.com/api）
 * @param timeout_sec 超时时间（秒）
 * @return HTTPResult 包含状态码、响应体和成功标志
 */
HTTPResult HTTPGet(const std::string &url, int timeout_sec = 5) {
    HTTPResult result;
    result.success = false;
    result.status_code = -1;

    try {
        // 解析 URL 和路径
        // 简化版：假设格式为 http://host:port/path
        std::string host = "localhost";
        std::string path = "/";
        int port = 80;

        // 提取协议
        bool is_https = false;
        size_t proto_pos = url.find("://");
        size_t host_start = 0;
        if (proto_pos != std::string::npos) {
            std::string proto = url.substr(0, proto_pos);
            if (proto == "https") {
                is_https = true;
                port = 443;
            }
            host_start = proto_pos + 3;
        }

        // 提取主机和路径
        size_t path_pos = url.find('/', host_start);
        if (path_pos != std::string::npos) {
            std::string host_port = url.substr(host_start, path_pos - host_start);
            path = url.substr(path_pos);

            // 提取端口
            size_t port_pos = host_port.find(':');
            if (port_pos != std::string::npos) {
                host = host_port.substr(0, port_pos);
                port = std::stoi(host_port.substr(port_pos + 1));
            } else {
                host = host_port;
            }
        }

        // 创建 HTTP 客户端
        httplib::Client cli(host, port);

        // 设置超时
        cli.set_timeout_sec(timeout_sec);
        cli.set_read_timeout_sec(timeout_sec);
        cli.set_write_timeout_sec(timeout_sec);

        // 执行 GET 请求
        auto res = cli.Get(path);

        if (res) {
            result.status_code = res->status;
            result.body = res->body;
            result.success = (res->status == 200);
        }

    } catch (const std::exception &e) {
        std::cerr << "HTTP request failed: " << e.what() << std::endl;
        result.body = e.what();
    }

    return result;
}

/**
 * PoC 测试函数
 * 在独立程序中测试 HTTP 调用
 */
void TestHTTPPoC() {
    std::cout << "=== DuckDB AI Extension - HTTP PoC Test ===" << std::endl;

    // 测试 1: HTTP GET 请求
    std::cout << "\n[Test 1] HTTP GET to httpbin.org" << std::endl;
    auto result = HTTPGet("http://httpbin.org/get", 10);

    std::cout << "  Status: " << result.status_code << std::endl;
    std::cout << "  Success: " << (result.success ? "YES" : "NO") << std::endl;
    std::cout << "  Body length: " << result.body.length() << " bytes" << std::endl;

    if (result.status_code == 200) {
        std::cout << "  ✅ HTTP call successful!" << std::endl;
    } else {
        std::cout << "  ❌ HTTP call failed" << std::endl;
    }

    // 测试 2: 超时控制
    std::cout << "\n[Test 2] Timeout test (2 second timeout)" << std::endl;
    auto timeout_result = HTTPGet("http://httpbin.org/delay/5", 2);

    if (!timeout_result.success) {
        std::cout << "  ✅ Timeout control works (expected failure)" << std::endl;
    } else {
        std::cout << "  ⚠️  Timeout might not work properly" << std::endl;
    }

    std::cout << "\n=== PoC Test Complete ===" << std::endl;
}

} // namespace ai_extension
} // namespace duckdb

/**
 * 独立测试程序
 * 编译：g++ -std=c++17 -I../../third_party/httplib http_poc.cpp -o http_poc
 * 运行：./http_poc
 */
int main() {
    duckdb::ai_extension::TestHTTPPoC();
    return 0;
}
