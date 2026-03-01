# DuckDB Extension Architecture Notes
## M0 Phase: Extension API 可行性评估

**日期**: 2026-03-01
**版本**: DuckDB v1.4.4
**Commit**: bda8739920

---

## 1. 版本选型

### 目标版本：DuckDB v1.4.4

**选型理由：**
1. **稳定性**：Python bindings 显示 v1.4.4，这是一个经过验证的稳定版本
2. **Extension API 成熟**：v1.4.x 系列 Extension API 已基本稳定，breaking changes 较少
3. **社区支持**：活跃维护周期，bug 修复及时
4. **依赖完整**：包含 httplib、mbedtls 等网络库

**锁定方式：**
```bash
# 当前状态：基于 feature/multimodal-ai-extension 分支
# Python bindings 实际版本：v1.4.4
# 待确认无误后，创建标签锁定版本
```

**风险提示：**
- Extension API 在大版本更新（v2.0）可能有 breaking changes
- 建议在项目中明确记录 DuckDB 版本依赖

---

## 2. Extension API 能力矩阵

### 2.1 Scalar UDF（标量函数）

**能力评估**: ✅ **完全支持**

**API 结构：**
```cpp
// 示例：hash.cpp
static void HashFunction(DataChunk &args, ExpressionState &state, Vector &result) {
    args.Hash(result);
}

ScalarFunction HashFun::GetFunction() {
    auto hash_fun = ScalarFunction({LogicalType::ANY}, LogicalType::HASH, HashFunction);
    hash_fun.varargs = LogicalType::ANY;  // 支持可变参数
    return hash_fun;
}
```

**适用 AI 算子**：
- `AI_transform`: 逐行转换（如：图像 → embedding）
- 标量推理：单样本预测

**限制**：
- 向量化执行：每次调用处理一批数据（DataChunk）
- 需要注意内存管理和类型转换

---

### 2.2 Aggregate UDF（聚合函数）

**能力评估**: ✅ **完全支持**

**API 结构：**
```cpp
// 示例：sum.cpp
// 聚合函数需要实现 4 个阶段：
// 1. Initialize: 初始化状态
// 2. Update: 处理输入数据
// 3. Combine: 合并部分结果（分布式场景）
// 4. Finalize: 输出最终结果

struct SumSetOperation {
    template <class STATE>
    static void Initialize(STATE &state) {
        state.Initialize();
    }

    template <class STATE>
    static void Combine(const STATE &source, STATE &target, AggregateInputData &) {
        target.Combine(source);
    }
};
```

**适用 AI 算子**：
- `AI_aggregation`: 聚类（如 K-Means）、近似最近邻搜索
- 分布式聚合：多机场景下的状态合并

**优势**：
- 原生支持分布式场景（Combine 操作）
- 支持复杂状态类型（struct 类型）

---

### 2.3 自定义类型（Custom Types）

**能力评估**: ✅ **支持**

**示例**：
```cpp
// JSON 扩展示例
auto json_type = LogicalType::JSON();
loader.RegisterType(LogicalType::JSON_TYPE_NAME, std::move(json_type));
```

**多模态应用**：
- **图像类型**: BLOB + 元数据（宽度、高度、格式）
- **Embedding 类型**: `DOUBLE[]` 数组类型
- **Audio 类型**: BLOB + 采样率、时长等

**实现建议**：
- M0 阶段：使用内置类型（BLOB、DOUBLE[]）
- M1 阶段：如需优化，考虑自定义类型

---

### 2.4 HTTP 调用能力

**能力评估**: ✅ **支持（通过第三方库）**

**可用库**：
- `third_party/httplib`: HTTP/HTTPS 客户端
- `third_party/mbedtls`: TLS 支持（HTTPS）

**集成方式**：
```cpp
// 推荐在 Extension CMakeLists.txt 中添加：
# include_directories(third_party/httplib)
# link_libraries(httplib)

// 示例用法（需验证）：
#include <httplib.h>

void CallAIService(const string &url, const string &payload) {
    httplib::Client cli(url);
    auto res = cli.Post("/api/v1/embed", payload, "application/json");
    // 处理响应...
}
```

**M0 阶段验证目标**：
- ✅ 库文件存在
- ⏸️ 需编写 PoC 验证编译和运行时行为
- ⏸️ 需测试超时控制、错误处理

**风险点**：
- 向量化执行中阻塞式 HTTP 调用可能影响性能
- 需考虑并发控制（线程池、连接池）

---

### 2.5 Python Bindings 暴露

**能力评估**: ✅ **原生支持**

**Python 集成方式**：
```python
import duckdb

# Extension 自动加载
con = duckdb.connect()
con.execute("INSTALL ai;")
con.execute("LOAD ai;")

# 调用 AI 算子
result = con.execute("""
    SELECT ai_filter(image_col, 'cat')
    FROM images
""").fetchall()
```

**Daft 集成建议**：
- **方案 A（推荐）**: Daft 通过 Python API 调用 DuckDB，Extension 在 DuckDB 进程内
- **方案 B**: Daft 嵌入 libduckdb.so，Extension 静态链接
- **方案 C**: 独立 DuckDB 服务，Daft 通过网络连接

**与 Daft 的接口边界（待讨论）**：
- 大对象传输（图像数据）的序列化方式
- 结果集返回格式（Arrow、Pandas、原生）

---

## 3. AI 算子初步设计

### 3.1 AI_filter

**语义**: 基于模型推理的行过滤

**SQL 接口**：
```sql
SELECT * FROM images
WHERE ai_filter(image_col, 'cat', 'clip') > 0.8;
```

**实现方案**：
- **ScalarFunction**，返回 DOUBLE（相似度分数）
- 参数：图像（BLOB）、文本提示（VARCHAR）、模型名（VARCHAR）
- HTTP 调用：调用外部 CLIP API

**M0 验证目标**：
- ✅ API 设计可行性
- ⏸️ HTTP 调用 PoC

---

### 3.2 AI_aggregation

**语义**: 基于聚类的数据聚合

**SQL 接口**：
```sql
SELECT ai_cluster(embedding_col, 10)
FROM embeddings;
```

**实现方案**：
- **AggregateFunction**，返回 INT（簇 ID）
- 参数：embedding 向量（DOUBLE[]）、K 值（INTEGER）
- 算法：K-Means 或 DBSCAN（M1 实现）

**M0 验证目标**：
- ✅ API 设计可行性
- ⏸️ 聚合状态管理（状态序列化）

---

### 3.3 AI_transform

**语义**: 数据转换（图像 → Embedding）

**SQL 接口**：
```sql
SELECT
    id,
    ai_transform(image_col, 'clip', 'embed') AS embedding
FROM images;
```

**实现方案**：
- **ScalarFunction** 或 **TableFunction**
- 参数：输入数据（BLOB）、模型名（VARCHAR）、操作（VARCHAR）
- 返回：DOUBLE[]（embedding 向量）

**M0 验证目标**：
- ✅ API 设计可行性
- ⏸️ 大对象性能

---

## 4. 多模态数据类型表示方案

### 4.1 图像（Image）

**M0 方案**（推荐）：
```sql
CREATE TABLE images (
    id INTEGER,
    image_data BLOB,           -- 原始图像 bytes
    format VARCHAR,            -- 'PNG', 'JPEG', etc.
    width INTEGER,
    height INTEGER
);
```

**M1 优化方案**：
- 自定义类型 `IMAGE_TYPE`，内部结构：
  ```cpp
  struct ImageData {
      string bytes;
      string format;
      int32_t width;
      int32_t height;
  }
  ```

**与 Daft 对齐**：
- Daft 的 `Image` 类型可直接序列化到 BLOB
- 元数据可存储在单独列或 JSON 中

---

### 4.2 Embedding 向量

**M0 方案**：
```sql
CREATE TABLE embeddings (
    id INTEGER,
    embedding DOUBLE[512],      -- 固定长度数组
    model_name VARCHAR
);
```

**M1 优化方案**：
- 自定义类型 `EMBEDDING_TYPE`
- 优化存储：FLOAT16 或 INT8 量化

---

### 4.3 Audio 音频

**M0 方案**：
```sql
CREATE TABLE audio (
    id INTEGER,
    audio_data BLOB,
    sample_rate INTEGER,
    duration FLOAT,
    format VARCHAR              -- 'WAV', 'MP3', etc.
);
```

---

## 5. 与 Daft 集成方案（待 daft-engineer 确认）

### 5.1 数据传输

**当前理解**（需确认）：
1. **大对象传输**：
   - Daft → DuckDB: 内存拷贝 vs 共享内存
   - BLOB 列的数据序列化格式

2. **会话管理**：
   - Daft 创建 DuckDB 实例的生命周期
   - 事务边界控制

### 5.2 接口边界

**待讨论问题**：
- Daft 如何传递图像数据给 DuckDB Extension？
- DuckDB Extension 的推理结果如何返回给 Daft？
- 是否需要支持 Daft 的分布式场景（多节点 DuckDB）？

---

## 6. M0 阶段结论

### ✅ 已验证

| 项目 | 状态 | 说明 |
|------|------|------|
| 编译环境 | ✅ | CMake + Clang 17，编译通过 |
| 测试套件 | ✅ | run_tests.sh 全部通过 |
| Extension 加载 | ✅ | Python 可加载扩展 |
| Scalar UDF | ✅ | API 文档和示例完整 |
| Aggregate UDF | ✅ | 支持状态管理和分布式 |
| 自定义类型 | ✅ | JSON 扩展示例可用 |
| HTTP 库 | ✅ | httplib + mbedtls 存在 |

### ⏸️ M0-M1 需验证

| 项目 | 优先级 | 说明 |
|------|--------|------|
| HTTP 调用 PoC | 🔴 高 | 验证 httplib 实际可用 |
| 大对象性能 | 🟡 中 | BLOB 列的序列化开销 |
| 与 Daft 接口 | 🔴 高 | 需与 daft-engineer 讨论 |
| 超时控制 | 🟡 中 | HTTP 调用的超时和重试 |

### 📋 下一步行动

1. **立即**：与 Tech Lead 和 daft-engineer 同步讨论接口边界
2. **M0 完成前**：编写 HTTP 调用 PoC（100 行 C++ 代码）
3. **M1 开始**：创建 `extension/ai` 目录，实现 AI_filter MVP

---

## 7. 参考文档

- **DuckDB Extension 文档**: `extension/README.md`
- **Extension 构建**: `extension/extension_build_tools.cmake`
- **示例扩展**:
  - `extension/json/`: 自定义类型 + 标量函数
  - `extension/parquet/`: 复杂 I/O 操作
  - `extension/core_functions/aggregate/`: 聚合函数示例

---

**文档状态**: 🟢 **M0 阶段完成，待 Tech Lead 审核后进入 M1**
