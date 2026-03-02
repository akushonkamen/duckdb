# DuckDB AI Extension - API 文档

## 概述

DuckDB AI Extension 提供多模态 AI 算子，支持图像相似度过滤、批处理等功能。

### 版本信息

- **Extension 名称**：ai
- **版本**：0.0.1-mvp
- **DuckDB 版本**：v1.4.4
- **依赖**：无外部依赖（使用 popen + curl）

## 函数列表

### 1. ai_filter

**功能**：计算图像与文本描述的相似度分数

**语法**：
```sql
ai_filter(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
```

**参数**：
- `image`（VARCHAR）：图像数据（base64 编码或 URL）
- `prompt`（VARCHAR）：描述文本（如 "cat"、"dog"）
- `model`（VARCHAR）：AI 模型名称（如 "clip"、"chatgpt-4o-latest"）

**返回值**：
- `DOUBLE`：相似度分数（0.0 - 1.0）

**示例**：
```sql
-- 基本用法
SELECT ai_filter('base64_image_data', 'a cat sitting on a couch', 'clip') AS similarity;

-- 在 WHERE 子句中使用
SELECT * FROM images
WHERE ai_filter(image_column, 'dog', 'clip') > 0.8;

-- 添加相似度分数列
SELECT
    id,
    image_url,
    ai_filter(image_data, 'beach sunset', 'clip') AS beach_score
FROM photos;
```

**特性**：
- ✅ 自动重试（最多 3 次）
- ✅ 超时控制（30 秒）
- ✅ 错误降级（失败返回 0.5）
- ✅ HTTP 调用（OpenAI 兼容 API）

### 2. ai_filter_batch

**功能**：批量计算图像相似度分数（内部委托给 ai_filter）

**语法**：
```sql
ai_filter_batch(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
```

**参数**：同 `ai_filter`

**返回值**：同 `ai_filter`

**示例**：
```sql
-- 批量处理（性能优化）
SELECT ai_filter_batch('base64_image_data', 'cat', 'clip') AS similarity
FROM images;

-- 多行处理
SELECT
    id,
    ai_filter_batch(image, 'animal', 'clip') AS score
FROM test_data
WHERE score > 0.5;
```

**特性**：
- ✅ 多行处理支持（unified vector data）
- ✅ 批处理 API 预留（未来优化）
- ✅ 与 ai_filter 相同的重试和错误处理

## 使用场景

### 场景 1：图像过滤

```sql
-- 筛选包含猫的图像
SELECT
    filename,
    ai_filter(image_base64, 'cat', 'clip') AS cat_score
FROM images
WHERE ai_filter(image_base64, 'cat', 'clip') > 0.8
ORDER BY cat_score DESC;
```

### 场景 2：相似度排序

```sql
-- 按与查询图像的相似度排序
SELECT
    product_name,
    product_image,
    ai_filter(product_image, 'red sports car', 'clip') AS similarity
FROM products
ORDER BY similarity DESC
LIMIT 10;
```

### 场景 3：多标签分类

```sql
-- 多类别相似度评分
SELECT
    image_id,
    ai_filter(image, 'cat', 'clip') AS cat_score,
    ai_filter(image, 'dog', 'clip') AS dog_score,
    ai_filter(image, 'bird', 'clip') AS bird_score
FROM animal_photos;
```

## 性能特性

### 并发执行

- **单次调用**：~100-500ms（API 延迟）
- **批量处理**：逐行执行（当前实现）
- **未来优化**：真实并发批处理

### 重试机制

- **重试次数**：最多 3 次
- **退避策略**：指数退避（100ms → 200ms → 400ms）
- **超时时间**：30 秒连接 + 5 秒读取
- **Jitter**：±20% 避免惊群效应

### 错误处理

- **网络错误**：自动重试
- **API 错误**：返回默认分数 0.5
- **超时**：自动重试
- **全部失败**：返回降级分数

## 配置参数

### 环境变量

| 变量名 | 默认值 | 说明 |
|--------|--------|------|
| `AI_FILTER_MAX_RETRIES` | 3 | 最大重试次数 |
| `AI_FILTER_TIMEOUT` | 30 | HTTP 超时（秒） |
| `AI_FILTER_DEFAULT_SCORE` | 0.5 | 降级分数（0.0-1.0） |

**设置示例**：
```bash
export AI_FILTER_MAX_RETRIES=5
export AI_FILTER_TIMEOUT=60
export AI_FILTER_DEFAULT_SCORE=0.3

./duckdb -unsigned -c "LOAD 'ai.duckdb_extension'; ..."
```

## 错误代码

| 错误 | 说明 | 处理 |
|------|------|------|
| `max_retries_exceeded` | 全部重试失败 | 返回默认分数 |
| HTTP 4xx | 客户端错误 | 返回默认分数 |
| HTTP 5xx | 服务器错误 | 自动重试 |
| Timeout | 超时 | 自动重试 |

## 限制和注意事项

### 当前限制

1. **图像格式**：需要 base64 编码或 URL
2. **并发限制**：当前逐行执行，不支持真实并发
3. **网络依赖**：需要外部 AI API 服务
4. **API 密钥**：硬编码（生产环境需修改）

### 最佳实践

1. **小批量测试**：先在少量数据上验证
2. **监控错误日志**：查看 `[AI_FILTER_RETRY]` 日志
3. **调整超时**：根据网络环境调整 `AI_FILTER_TIMEOUT`
4. **使用 Mock 服务器**：开发测试时使用 `test_mock_ai_server.py`

## 集成示例

### Python 集成

```python
import duckdb

# 连接数据库
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions': True})

# 加载 Extension
con.load_extension('/path/to/ai.duckdb_extension')

# 执行查询
result = con.execute("""
    SELECT ai_filter('base64_image', 'cat', 'clip') AS score
""").fetchall()

print(f"Similarity score: {result[0][0]}")
```

### CLI 使用

```bash
# 加载 Extension
./duckdb -unsigned -c "LOAD 'build/test/extension/ai.duckdb_extension';"

# 执行查询
./duckdb -unsigned -c \
  "LOAD 'build/test/extension/ai.duckdb_extension'; \
   SELECT ai_filter('test_image', 'cat', 'clip') AS score;"

# 批量查询
./duckdb -unsigned -c \
  "LOAD 'build/test/extension/ai.duckdb_extension'; \
   SELECT ai_filter_batch('test', 'cat', 'clip') FROM range(10);"
```

## 参考

- [DuckDB Extension 开发指南](https://duckdb.org/docs/extensions/)
- [OpenAI API 文档](https://platform.openai.com/docs/api-reference)
- [CLIP 模型](https://openai.com/research/clip/)
