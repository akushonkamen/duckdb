# DuckDB AI_filter Extension 使用指南

## 扩展信息

**版本：** 0.0.1-mvp
**类型：** Loadable Extension
**状态：** MVP (返回随机 0.0-1.0 分数，M2 将集成真实 AI 调用)

---

## 快速开始

### 1. 扩展位置
```bash
# 相对于 DuckDB 仓库根目录
test/extension/ai.duckdb_extension

# 绝对路径（示例）
/Users/yp1017/development/daft-duckdb-multimodal/duckdb/test/extension/ai.duckdb_extension
```

### 2. 加载扩展

#### 方式 A：Python API
```python
import duckdb

con = duckdb.connect(':memory:')
con.load_extension('path/to/ai.duckdb_extension')
```

#### 方式 B：DuckDB CLI
```bash
# 需要使用 -unsigned 参数加载未签名扩展
./duckdb -unsigned -c "LOAD 'test/extension/ai.duckdb_extension';"
```

### 3. 使用 AI_filter 函数

**语法：**
```sql
ai_filter(image_blob BLOB, prompt VARCHAR, model VARCHAR) -> DOUBLE
```

**参数：**
- `image_blob`: 图像数据（BLOB 类型）
- `prompt`: 文本提示（VARCHAR）
- `model`: 模型名称（VARCHAR），当前仅用于标识

**返回：**
- DOUBLE 类型，范围 0.0-1.0
- MVP 版本：随机分数
- M2 版本：真实 AI 相似度分数

---

## 示例查询

### 示例 1：单行调用
```sql
SELECT ai_filter('test_image_data'::BLOB, 'a cat', 'clip') AS similarity;
```

### 示例 2：批量处理
```sql
SELECT
    id,
    image_name,
    ai_filter(image_data, 'a dog', 'clip') AS similarity
FROM images
WHERE similarity > 0.5;
```

### 示例 3：多模态过滤
```sql
-- 查找包含猫的图像
SELECT id, filename
FROM images
WHERE ai_filter(image_data, 'cat', 'clip') > 0.8;

-- 查找包含狗或猫的图像
SELECT id, filename,
       ai_filter(image_data, 'dog', 'clip') AS dog_score,
       ai_filter(image_data, 'cat', 'clip') AS cat_score
FROM images
WHERE dog_score > 0.6 OR cat_score > 0.6;
```

### 示例 4：与 Daft 集成（预期）
```python
import duckdb
import daft

# Daft DataFrame
df = daft.read_parquet("images.parquet")

# 通过 DuckDB 执行 AI_filter
con = duckdb.connect(":memory:")
con.register("images_df", df)

result = con.execute("""
    SELECT *
    FROM images_df
    WHERE ai_filter(image_data, 'cat', 'clip') > 0.8
""").df()

# 转回 Daft
result_df = daft.from_pydict(result)
```

---

## Python 集成示例

### 完整工作流
```python
import duckdb

# 创建连接
con = duckdb.connect(':memory:')

# 加载扩展
ext_path = "/path/to/test/extension/ai.duckdb_extension"
con.load_extension(ext_path)

# 创建测试表
con.execute("""
    CREATE TABLE images AS
    SELECT
        id,
        'image_' || id::VARCHAR AS filename,
        CAST('binary_data_' || id::VARCHAR AS BLOB) AS image_data
    FROM range(10)
""")

# 使用 AI_filter
result = con.execute("""
    SELECT
        id,
        filename,
        ai_filter(image_data, 'a cat', 'clip') AS similarity
    FROM images
    ORDER BY similarity DESC
    LIMIT 5
""").fetchall()

for row in result:
    print(f"ID: {row[0]}, File: {row[1]}, Score: {row[2]:.4f}")
```

---

## 注意事项

### ⚠️ 未签名扩展警告
```
IO Error: Extension "..." could not be loaded because its signature
is either missing or invalid and unsigned extensions are disabled.
```

**解决方案：**
- CLI: 使用 `duckdb -unsigned`
- Python: 扩展会自动加载（Python duckdb 库允许未签名扩展）

### ⚠️ 版本兼容性
- 当前扩展针对 DuckDB v0.0.1 构建（from build）
- Python duckdb 库版本：v1.4.4
- 如遇版本不匹配，需要重新构建扩展

### ⚠️ Mock 实现
- 当前返回随机分数（0.0-1.0）
- 每次调用结果不同（VOLATILE）
- M2 将替换为真实 HTTP AI 调用

---

## 故障排除

### 问题：扩展未找到
```bash
# 检查文件存在
ls -lh test/extension/ai.duckdb_extension

# 使用绝对路径
con.load_extension("/absolute/path/to/ai.duckdb_extension")
```

### 问题：函数不存在
```sql
-- 确认扩展已加载
SELECT * FROM duckdb_functions() WHERE function_name = 'ai_filter';

-- 手动加载
LOAD '/path/to/ai.duckdb_extension';
```

### 问题：类型错误
```sql
-- 确保使用 BLOB 类型
SELECT ai_filter(CAST('data' AS BLOB), 'prompt', 'model');
```

---

## 测试清单

集成测试时请验证：
- [ ] 扩展成功加载
- [ ] ai_filter 函数存在于 duckdb_functions()
- [ ] 单行调用返回 DOUBLE 类型
- [ ] 批量处理（FROM range(N)）正常工作
- [ ] WHERE 子句过滤正常
- [ ] 返回值在 [0.0, 1.0] 范围内

---

**更新日期：** 2026-03-01
**状态：** ✅ MVP 就绪，等待集成测试
