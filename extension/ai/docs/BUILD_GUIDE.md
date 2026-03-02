# DuckDB AI Extension - 编译和安装指南

## 概述

本指南介绍如何编译、构建和安装 DuckDB AI Extension。

## 前置要求

### 系统要求

- **操作系统**：macOS, Linux
- **编译器**：Clang (推荐) 或 GCC
- **CMake**：3.15+
- **DuckDB**：v1.4.4

### 工具依赖

```bash
# macOS
xcode-select --install

# Linux (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y cmake clang libssl-dev

# Linux (CentOS/RHEL)
sudo yum install -y cmake clang openssl-devel
```

## 快速开始

### 方法 1：使用 DuckDB 构建系统（推荐）

```bash
# 1. 进入 DuckDB 根目录
cd /path/to/duckdb

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. 编译 Extension
make ai_extension

# 5. 验证构建
ls -lh test/extension/ai.duckdb_extension
```

**输出**：
```
-rwxr-xr-x  1 user  staff  7.1M Mar  3 00:00 ai.duckdb_extension
```

### 方法 2：使用独立构建脚本

```bash
# 1. 进入 AI Extension 目录
cd /path/to/duckdb/extension/ai

# 2. 执行构建脚本
bash build.sh

# 3. 验证构建
ls -lh ../../build/test/extension/ai.duckdb_extension
```

## 详细编译流程

### 步骤 1：编译 DuckDB

```bash
cd /path/to/duckdb
mkdir -p build && cd build

# 配置（强制版本号）
cmake .. -DCMAKE_BUILD_TYPE=Release \
          -DOVERRIDE_GIT_DESCRIBE=v1.4.4

# 编译 DuckDB CLI
make -j8

# 验证
./duckdb -c "SELECT version();"
# 输出：v1.4.4
```

### 步骤 2：编译 AI Extension

```bash
# 仍在 build/ 目录内
make ai_extension

# 检查输出
ls -lh test/extension/ai.duckdb_extension
```

**预期输出**：
```
-rwxr-xr-x  1 user  staff  7.1M Mar  3 00:00 ai.duckdb_extension
```

### 步骤 3：验证 Extension 功能

```bash
# 测试 Extension 加载
./duckdb -unsigned -c "LOAD 'test/extension/ai.duckdb_extension';"

# 测试函数注册
./duckdb -unsigned -c \
  "LOAD 'test/extension/ai.duckdb_extension'; \
   SELECT function_name, parameters FROM duckdb_functions() \
   WHERE function_name LIKE 'ai_%';"

# 预期输出：
# ┌─────────────────┬────────────────────┐
# │  function_name  │     parameters     │
# ├─────────────────┼────────────────────┤
# │ ai_filter       │ [col0, col1, col2] │
# │ ai_filter_batch │ [col0, col1, col2] │
# └─────────────────┴────────────────────┘
```

## 安装 Extension

### 方法 1：手动加载（推荐）

```bash
# 使用绝对路径
./duckdb -unsigned -c "LOAD '/absolute/path/to/ai.duckdb_extension';"

# 使用相对路径
./duckdb -unsigned -c "LOAD 'build/test/extension/ai.duckdb_extension';"
```

### 方法 2：安装到系统目录

```bash
# 复制 Extension 到 DuckDB 目录
cp build/test/extension/ai.duckdb_extension \
   ~/.duckdb/extensions/v1.4.4/osx_arm64/

# DuckDB 会自动加载
./duckdb -c "SELECT * FROM duckdb_extensions() WHERE loaded = true;"
```

### 方法 3：Python 集成

```python
import duckdb

# 允许未签名 Extension
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions': True})

# 加载 Extension
con.load_extension('/path/to/ai.duckdb_extension')

# 验证加载
result = con.execute("SELECT function_name FROM duckdb_functions() WHERE function_name LIKE 'ai_%'").fetchall()
print([row[0] for row in result])
# 输出：['ai_filter', 'ai_filter_batch']
```

## 构建配置

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CMAKE_BUILD_TYPE` | `Release` | 构建类型 |
| `OVERRIDE_GIT_DESCRIBE` | - | 强制版本号 |
| `DUCKDB_EXTENSION_DIR` | `extension/ai` | Extension 目录 |

### 构建变量

```bash
# 自定义 Extension 目录
cmake .. -DDUCKDB_EXTENSION_DIR=/path/to/extensions

# 启用调试符号
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 禁用优化
cmake .. -DCMAKE_CXX_FLAGS="-O0"
```

## 常见问题

### 问题 1：编译错误

**错误**：
```
fatal error: 'duckdb.hpp' file not found
```

**解决**：
```bash
# 确保 DuckDB 头文件路径正确
cmake .. -I/path/to/duckdb/src/include
```

### 问题 2：Extension 加载失败

**错误**：
```
InvalidInputException: The file was built specifically for DuckDB version 'v1.4.4'
and can only be loaded with that version. (this version is DuckDB is 'v0.0.1')
```

**解决**：
```bash
# 重新编译 DuckDB，强制版本号
cd build
cmake .. -DOVERRIDE_GIT_DESCRIBE=v1.4.4
make duckdb
```

### 问题 3：符号错误

**错误**：
```
Symbol not found: __ZNK6duckdb...
```

**解决**：
- 确保 Extension 与 DuckDB 版本匹配
- 重新编译 Extension
- 使用正确的 DuckDB 构建系统

### 问题 4：popen 权限拒绝

**错误**：
```
popen failed: Permission denied
```

**解决**：
```bash
# 检查 curl 可执行性
which curl

# 检查路径权限
ls -la $(which curl)

# macOS：修复权限
xcode-select --install
```

## 性能优化

### 编译优化

```bash
# Release 构建（默认）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 启用链接时优化（LTO）
cmake .. -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE

# 本地优化
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native"
```

### 运行时优化

```bash
# 减少超时（快速失败）
export AI_FILTER_TIMEOUT=10

# 减少重试次数
export AI_FILTER_MAX_RETRIES=1

# 使用默认分数（跳过 API 调用）
export AI_FILTER_DEFAULT_SCORE=0.5
```

## 调试

### 启用详细日志

```cpp
// 在 ai_filter.cpp 中添加调试输出
fprintf(stderr, "[DEBUG] Calling AI API with prompt: %s\n", prompt.c_str());
```

### 查看日志

```bash
# DuckDB 日志重定向
./duckdb -unsigned -c "LOAD 'ai.duckdb_extension'; ..." 2>&1 | tee duckdb.log

# 查看重试日志
grep "AI_FILTER" duckdb.log
```

### 测试模式

```bash
# 使用 Mock 服务器测试
cd extension/ai/tests
python3 test_mock_ai_server.py &
MOCK_PID=$!

# 运行测试
cd ../../..
./build/duckdb -unsigned -c \
  "LOAD 'build/test/extension/ai.duckdb_extension'; \
   SELECT ai_filter('test', 'cat', 'clip') FROM range(5);"

# 清理
kill $MOCK_PID
```

## CI/CD 集成

### GitHub Actions

参考 `.github/workflows/extension_test.yml`：

```yaml
- name: Build AI Extension
  run: |
    cd duckdb/build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make ai_extension

- name: Test Extension
  run: |
    cd duckdb
    ./build/duckdb -unsigned -c \
      "LOAD 'build/test/extension/ai.duckdb_extension'; \
       SELECT ai_filter_batch('test', 'cat', 'clip') FROM range(5);"
```

## 更新 Extension

### 修改代码

```bash
# 1. 编辑源代码
vim extension/ai/src/ai_filter.cpp

# 2. 清理旧构建
cd build
rm test/extension/ai.duckdb_extension

# 3. 重新编译
make ai_extension

# 4. 测试
./duckdb -unsigned -c "LOAD 'test/extension/ai.duckdb_extension'; ..."
```

### 增加版本号

```bash
# 编辑 CMakeLists.txt
vim extension/ai/CMakeLists.txt
# 修改 "0.0.1-mvp" 为新版本

# 重新构建
cd build
rm test/extension/ai.duckdb_extension
cmake .. -DOVERRIDE_GIT_DESCRIBE=v1.4.4
make ai_extension
```

## 发布

### 创建发布包

```bash
# 1. 编译 Release 版本
cmake .. -DCMAKE_BUILD_TYPE=Release
make ai_extension

# 2. 验证版本
./duckdb -version
./duckdb -unsigned -c "LOAD 'test/extension/ai.duckdb_extension';"

# 3. 打包
mkdir release
cp build/test/extension/ai.duckdb_extension release/
cp README.md release/
cp USAGE_GUIDE.md release/

# 4. 创建压缩包
tar czf ai-extension-v0.0.1-mvp.tar.gz -C release .
```

## 参考

- [DuckDB 扩展开发](https://duckdb.org/docs/extensions/)
- [CMake 文档](https://cmake.org/documentation/)
- [CLANG 文档](https://clang.llvm.org/docs/)
