# DuckDB AI Extension - 错误处理和重试机制

## 概述

本文档详细说明 DuckDB AI Extension 的错误处理和重试机制（TASK-OPS-001）。

## 设计目标

1. **提高可靠性**：临时网络故障不应导致查询失败
2. **快速失败**：永久性错误应快速返回
3. **优雅降级**：服务不可用时返回合理默认值
4. **可观测性**：结构化日志便于故障排查

## 重试机制

### 指数退避算法

```cpp
delay = BASE_DELAY_MS * 2^attempt

# 限制最大延迟
delay = min(delay, MAX_DELAY_MS)

# 添加 Jitter（±20%）避免惊群效应
jitter = delay / 5
delay = delay + random(-jitter, +jitter)
```

**延迟时间表**：

| 尝试次数 | 基础延迟 | Jitter 范围 | 实际延迟范围 |
|---------|---------|-----------|-------------|
| 第 1 次 | 100 ms | ±20 ms | 80-120 ms |
| 第 2 次 | 200 ms | ±40 ms | 160-240 ms |
| 第 3 次 | 400 ms | ±80 ms | 320-480 ms |
| 第 4 次 | 800 ms | ±160 ms | 640-960 ms |

### 重试流程图

```
HTTP 调用
    │
    ├─ 成功
    │     └─ 返回解析分数
    │
    └─ 失败
          │
          ├─ 重试次数 < MAX_RETRIES？
          │     │
          │     ├─ 是 → 等待 delay → 重试
          │     │
          │     └─ 否 → 返回降级分数
          │
          └─ 错误类型：popen 失败 / HTTP 错误 / 超时
```

### 重试配置

| 参数 | 默认值 | 环境变量 | 说明 |
|------|--------|---------|------|
| MAX_RETRIES | 3 | `AI_FILTER_MAX_RETRIES` | 最大重试次数 |
| BASE_DELAY_MS | 100 | - | 初始延迟（不可配置） |
| MAX_DELAY_MS | 5000 | - | 最大延迟（不可配置） |
| HTTP_TIMEOUT_SEC | 30 | `AI_FILTER_TIMEOUT` | HTTP 超时（秒） |

**修改重试次数**：
```bash
# 增加重试次数到 5 次
export AI_FILTER_MAX_RETRIES=5

# 减少到 1 次（快速失败）
export AI_FILTER_MAX_RETRIES=1

# 禁用重试（仅尝试一次）
export AI_FILTER_MAX_RETRIES=0
```

## 超时控制

### 两层超时机制

```bash
curl --connect-timeout 30 --max-time 35
```

- **--connect-timeout 30**：连接超时 30 秒
- **--max-time 35**：总超时 35 秒（连接 + 数据传输）

### 超时配置

```bash
# 默认超时（30 秒）
./duckdb -unsigned -c "..."

# 自定义超时（60 秒）
export AI_FILTER_TIMEOUT=60
./duckdb -unsigned -c "..."

# 快速超时（10 秒）
export AI_FILTER_TIMEOUT=10
./duckdb -unsigned -c "..."
```

### 超时处理

```
HTTP 调用启动
    │
    ├─ 30 秒内响应
    │     └─ 继续处理
    │
    ├─ 连接超时（>30 秒）
    │     └─ 视为失败 → 重试
    │
    └─ 总超时（>35 秒）
          └─ popen 返回 → 视为失败 → 重试
```

## 降级策略

### 降级触发条件

1. **全部重试失败**：达到 MAX_RETRIES 次数
2. **popen 失败**：无法执行 curl 命令
3. **HTTP 错误**：服务器返回错误响应
4. **空响应**：API 返回空字符串
5. **解析失败**：无法从响应中提取分数

### 降级行为

```cpp
// 返回错误标记
return "{\"error\":\"max_retries_exceeded\"}";

// ExtractScore 检测错误标记并返回默认分数
if (json_response.find("\"error\":\"max_retries_exceeded\"") != std::string::npos) {
    return DEFAULT_DEGRADATION_SCORE;  // 默认 0.5
}
```

### 默认分数配置

```bash
# 默认降级分数（0.5）
./duckdb -unsigned -c "..."

# 自定义降级分数（0.3）
export AI_FILTER_DEFAULT_SCORE=0.3
./duckdb -unsigned -c "..."

# 总是返回 0.0（悲观）
export AI_FILTER_DEFAULT_SCORE=0.0
./duckdb -unsigned -c "..."
```

### 降级策略选择

| 场景 | 推荐策略 | 默认分数 | 理由 |
|------|---------|---------|------|
| 图像过滤 | 悲观 | 0.0 | 宁可漏选，不过滤 |
| 内容推荐 | 乐观 | 0.8 | 保证内容展示 |
| 测试环境 | 中性 | 0.5 | 不影响测试结果 |
| 生产环境 | 自定义 | 根据业务 | 符合业务逻辑 |

## 错误日志

### 日志格式

```cpp
fprintf(stderr, "[AI_FILTER_RETRY] Attempt %d: popen failed\n", attempt);
fprintf(stderr, "[AI_FILTER_RETRY] Attempt %d: HTTP error (status=%d, len=%zu)\n", ...);
fprintf(stderr, "[AI_FILTER_RETRY] Attempt %d: Success after retry\n", attempt);
```

### 日志示例

**成功调用**：
```
（无日志输出）
```

**首次失败，重试成功**：
```
[AI_FILTER_RETRY] Attempt 0: HTTP error (status=19, len=0)
[AI_FILTER_RETRY] Attempt 1: Success after retry
```

**全部失败**：
```
[AI_FILTER_RETRY] Attempt 0: popen failed
[AI_FILTER_RETRY] Attempt 1: popen failed
[AI_FILTER_RETRY] Attempt 2: popen failed
[AI_FILTER] All retries exhausted, using degradation score
```

### 查看日志

```bash
# 实时查看
./duckdb -unsigned -c "LOAD 'ai.duckdb_extension'; ..." 2>&1 | grep AI_FILTER

# 保存到文件
./duckdb -unsigned -c "LOAD 'ai.duckdb_extension'; ..." 2>&1 | tee duckdb.log

# 仅查看错误
./duckdb -unsigned -c "LOAD 'ai.duckdb_extension'; ..." 2>/dev/null
```

## 错误类型

### 1. popen 失败

**原因**：
- curl 不存在或无执行权限
- 系统资源不足
- Shell 受限

**日志**：
```
[AI_FILTER_RETRY] Attempt 0: popen failed
```

**处理**：
- 重试（最多 3 次）
- 全部失败 → 返回降级分数

### 2. HTTP 错误

**原因**：
- 网络不可达
- DNS 解析失败
- 连接被拒绝

**日志**：
```
[AI_FILTER_RETRY] Attempt 0: HTTP error (status=19, len=0)
[AI_FILTER_RETRY] Attempt 1: HTTP error (status=19, len=0)
...
```

**状态码**：
- `status=19`：curl 无法连接
- `status=7`：无法解析主机
- `status=0`：空响应

**处理**：
- 重试（最多 3 次）
- 全部失败 → 返回降级分数

### 3. 超时错误

**原因**：
- API 响应缓慢
- 网络延迟高
- 服务器负载高

**日志**：
```
[AI_FILTER_RETRY] Attempt 0: HTTP error (status=28, len=0)
```

**状态码**：
- `status=28`：操作超时

**处理**：
- 重试（最多 3 次）
- 调整 `AI_FILTER_TIMEOUT`

### 4. API 错误

**原因**：
- API 密钥无效
- 请求格式错误
- 服务器错误（5xx）

**日志**：
```
[AI_FILTER_RETRY] Attempt 0: HTTP error (status=200, len=150)
```

**响应示例**：
```json
{"error":{"message":"Invalid API key"}}
```

**处理**：
- 客户端错误（4xx）→ 不重试，返回降级分数
- 服务器错误（5xx）→ 重试

## 性能分析

### 延迟分析

**成功调用**：
```
延迟 = API 响应时间（~100-500ms）
```

**失败重试**：
```
延迟 = 第 1 次调用（超时） + 100ms + 第 2 次调用（超时） + 200ms + 第 3 次调用（超时） + 400ms + 降级返回
     = 30s + 100ms + 30s + 200ms + 30s + 400ms
     ≈ 90 秒 + 700ms
```

### 优化建议

**1. 快速失败模式**

```cpp
// 检测连续失败
static int consecutive_failures = 0;

if (consecutive_failures >= 5) {
    // 跳过重试，直接返回降级分数
    return DEFAULT_DEGRADATION_SCORE;
}

// 更新计数器
consecutive_failures++;
```

**2. 断路器模式**

```cpp
// 时间窗口内失败率过高时跳过重试
static time_t last_failure_time = 0;
static int failure_count_in_window = 0;

if (current_time - last_failure_time < 60) {  // 60 秒窗口
    failure_count_in_window++;
    if (failure_count_in_window >= 10) {
        // 断路器触发，跳过重试
        return DEFAULT_DEGRADATION_SCORE;
    }
} else {
    // 重置窗口
    failure_count_in_window = 0;
}
```

**3. 自适应延迟**

```cpp
// 根据历史成功率调整延迟
static float success_rate = 1.0;

if (success_rate < 0.5) {
    // 成功率低，增加延迟
    delay = delay * 2;
} else if (success_rate > 0.9) {
    // 成功率高，减少延迟
    delay = delay / 2;
}
```

## 测试和验证

### 单元测试

```bash
# 测试重试机制
cd extension/ai/tests
python3 test_retry_mechanism.py
```

### 集成测试

```bash
# 测试真实重试场景
./build/duckdb -unsigned -c \
  "LOAD 'build/test/extension/ai.duckdb_extension'; \
   SELECT ai_filter('test', 'cat', 'clip') FROM range(5);"
```

### 压力测试

```bash
# 模拟网络故障
# 1. 阻止 API 访问
sudo pfctl block chatapi.littlewheat.com

# 2. 运行查询
./build/duckdb -unsigned -c \
  "LOAD 'build/test/extension/ai.duckdb_extension'; \
   SELECT ai_filter('test', 'cat', 'clip') FROM range(3);"

# 3. 恢复访问
sudo pfctl unblock chatapi.littlewheat.com
```

## 最佳实践

### 1. 合理配置重试次数

```bash
# 生产环境：3 次重试（默认）
export AI_FILTER_MAX_RETRIES=3

# 开发环境：1 次重试（快速反馈）
export AI_FILTER_MAX_RETRIES=1

# 内网环境：5 次重试（网络稳定）
export AI_FILTER_MAX_RETRIES=5
```

### 2. 监控错误日志

```bash
# 收集错误日志
./duckdb ... 2>&1 | grep "AI_FILTER" > errors.log

# 统计错误类型
grep "popen failed" errors.log | wc -l
grep "HTTP error" errors.log | wc -l
grep "Success after retry" errors.log | wc -l
```

### 3. 调整超时时间

```bash
# 根据网络环境调整
export AI_FILTER_TIMEOUT=60  # 慢速网络
export AI_FILTER_TIMEOUT=10  # 快速网络
```

### 4. 设置合理的默认分数

```bash
# 根据业务场景设置
export AI_FILTER_DEFAULT_SCORE=0.0  # 悲观（严格过滤）
export AI_FILTER_DEFAULT_SCORE=0.5  # 中性（平衡）
export AI_FILTER_DEFAULT_SCORE=0.8  # 乐观（宽松过滤）
```

## 参考

- [重试模式](https://en.wikipedia.org/wiki/Exponential_backoff)
- [断路器模式](https://martinfowler.com/bliki/CircuitBreakerPattern.html)
- [优雅降级](https://aws.amazon.com/cn/blogs/architecture/graceful-degradation/
