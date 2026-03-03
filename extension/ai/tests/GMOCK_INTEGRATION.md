# AI Extension Google Mock 集成指南

## 概述

AI Extension 现已集成 Google Mock 测试框架到主 DuckDB 构建系统，支持单元测试和代码覆盖率测量。

## 测试结构

```
duckdb/
├── extension/ai/
│   ├── include/
│   │   ├── http_client_interface.hpp   # 依赖注入接口
│   │   └── ai_function_executor.hpp    # 可测试的执行器
│   ├── src/
│   │   └── http_client.cpp             # 生产实现
│   └── tests/gtest/
│       ├── CMakeLists.txt              # 独立 CMake 配置
│       ├── ai_filter_test.cpp          # 9 个单元测试
│       ├── build_and_test.sh           # 独立构建脚本
│       └── mock/
│           └── mock_interfaces.hpp     # Mock 类定义
└── test/
    └── ai_unittest/
        └── CMakeLists.txt              # 主构建集成
```

## 构建和运行测试

### 方法 1: 主 DuckDB 构建（推荐）

```bash
cd duckdb
mkdir -p build && cd build

# 配置时启用 AI 单元测试
cmake .. -DBUILD_AI_UNITTESTS=ON

# 构建
make ai_extension_unittest

# 运行测试
make unittest_ai

# 或直接运行
./test/ai_unittest/ai_extension_unittest --gtest_color=yes
```

### 方法 2: 独立构建

```bash
cd duckdb/extension/ai/tests/gtest
mkdir -p build && cd build
cmake .. -DBUILD_AI_UNITTESTS=ON
make ai_filter_gtest
./ai_filter_gtest
```

### 方法 3: 使用脚本

```bash
cd duckdb/extension/ai/tests/gtest
./build_and_test.sh
```

## 测试目标

| 目标 | 描述 |
|------|------|
| `ai_extension_unittest` | 编译测试可执行文件（主构建） |
| `unittest_ai` | 运行测试（主构建） |

## 测试用例

| 测试 | 描述 |
|------|------|
| `SuccessfulAPIResponse` | 成功 API 响应 |
| `SuccessfulAPIResponseWithDifferentScore` | 不同分数值 |
| `RetryOnceThenSuccess` | 重试一次后成功 |
| `MaxRetriesExhausted` | 达到最大重试次数 |
| `EmptyResponseThenRetry` | 空响应后重试 |
| `RandomJitterInDelay` | 指数退避随机抖动 |
| `DifferentModelNames` | 不同模型名称 |
| `EmptyImage` | 空图片输入 |
| `ResponseWithInvalidJSON` | 无效 JSON 响应 |

## 依赖

- Google Test >= 1.10
- Google Mock >= 1.10

### macOS 安装

```bash
brew install googletest
```

### Linux 安装

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev libgmock-dev

# Fedora
sudo dnf install gtest-devel gmock-devel
```

## 覆盖率

当前覆盖率（单元测试）：
- `ai_function_executor.hpp`: 50.59% (43/85 行)
- `ai_filter_test.cpp`: 100% (103/103 行)

未覆盖代码主要是：
- 测试模式代码 (`AI_FILTER_TEST_MODE`)
- Fallback 到 `ExecuteCurlCommand` (生产代码)
- 直接 curl 命令执行 (集成测试领域)

## CI 集成

要集成到 CI/CD，添加以下步骤：

```yaml
- name: Build DuckDB with AI Extension Tests
  run: |
    cd duckdb
    mkdir -p build && cd build
    cmake .. -DBUILD_AI_UNITTESTS=ON
    make ai_extension_unittest

- name: Run AI Extension Unit Tests
  run: |
    cd duckdb/build
    make unittest_ai
```

## 问题排查

### Google Test 未找到

```
CMake Error: Could not find GTest
```

**解决方案**：
- macOS: `brew install googletest`
- Linux: `sudo apt-get install libgtest-dev libgmock-dev`

### 链接错误

```
ld: library not found for -lgtest
```

**解决方案**：
- 检查 CMakeLists.txt 中的库路径
- 确保 `GTEST_LIBRARIES` 变量正确设置

## 下一步

- [x] 集成到主 CMake 构建流程
- [ ] 添加更多边界条件测试
- [ ] 添加性能基准测试
- [ ] 添加 CI/CD 自动化测试
