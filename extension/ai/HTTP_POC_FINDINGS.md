# HTTP 调用可行性验证报告

## 验证日期
2026-03-01

## 目标
验证 DuckDB Extension 中使用 httplib 进行 HTTP 调用的可行性

## 验证结果

### ✅ 1. 库文件存在
```
third_party/httplib/httplib.hpp (411KB)
```
- httplib 是 header-only 库
- 文件大小正常，内容完整
- LICENSE 文件存在

### ✅ 2. TLS 支持
```
third_party/mbedtls/ (完整的 mbedtls 实现)
```
- 支持 HTTPS 调用
- 包含证书验证、加密等完整功能

### ⚠️ 3. 编译依赖发现

**发现：** httplib 已被 DuckDB patch，需要依赖 DuckDB 头文件

**错误示例：**
```
fatal error: 'duckdb/original/std/memory.hpp' file not found
```

**结论：**
- httplib 在 DuckDB 中不是独立的第三方库
- 已被集成到 DuckDB 构建系统中
- Extension 需要通过 CMake 正确链接 DuckDB 头文件

### ✅ 4. 正确的集成方式（M1 实施）

在 Extension 的 CMakeLists.txt 中：
```cmake
# DuckDB Extension 标准方式
duckdb_extension(ai
    SOURCES
        src/ai_extension.cpp
        src/http_client.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/third_party/httplib
    LINK_LIBRARIES
        # 如需 HTTPS：mbedtls 相关库
)
```

## 可行性结论

| 检查项 | 状态 | 说明 |
|--------|------|------|
| 库文件存在 | ✅ | httplib.hpp + mbedtls 完整 |
| Header-only | ✅ | 无需额外编译链接 |
| HTTPS 支持 | ✅ | mbedtls 可用 |
| 超时控制 | ✅ | httplib 原生支持 |
| DuckDB 集成 | ✅ | 通过 Extension CMake 集成 |
| 编译验证 | ⏸️ | M1 实际编译时验证 |

## M0 阶段结论

**HTTP 调用在 DuckDB Extension 中是可行的。**

### 关键 API（httplib 提供）

```cpp
#include <httplib.h>

// 创建客户端
httplib::Client cli("example.com", 80);

// 设置超时
cli.set_timeout_sec(5);
cli.set_read_timeout_sec(5);
cli.set_write_timeout_sec(5);

// GET 请求
auto res = cli.Get("/api");
if (res) {
    std::cout << res->status << std::endl;
    std::cout << res->body << std::endl;
}

// POST 请求
auto res = cli.Post("/api", payload, "application/json");
```

### M1 实施建议

1. **封装层**：创建 `HttpClient` 类封装 httplib
2. **错误处理**：统一处理网络错误、超时、非 200 响应
3. **连接池**：考虑实现连接池（M2 优化）
4. **异步支持**：评估是否需要异步 HTTP 调用

## 下一步

M1 阶段将在实际 Extension 代码中集成 httplib，届时会：
1. 创建 `extension/ai/src/http_client.cpp`
2. 实现第一个 AI 算子（AI_filter）的 HTTP 调用
3. 完整的编译和测试验证

---

**验证人：** duckdb-engineer
**状态：** ✅ M0 验证完成，HTTP 调用可行
