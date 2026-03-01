# 变更记录 — duckdb submodule

## 格式
```
### [日期] TASK-X-XXX：标题
- 类型：新增/修改/删除  |  文件：  |  摘要：
- 测试：pass X/X  |  编译：✅/❌  |  commit：<hash>  |  巡检：⏳/✅/❌
```

## 历史

### [2026-03-01] TASK-K-001：M0 环境验证与架构摸底
- 类型：新增  |  文件：ARCH_NOTES.md  |  摘要：Extension API 可行性评估文档
- 测试：pass 1/1 (run_tests.sh)  |  编译：✅  |  commit：f63d149b78  |  巡检：✅

**新增内容：**
- 版本选型：DuckDB v1.4.4
- Extension API 能力矩阵（Scalar/Aggregate UDF、自定义类型、HTTP 调用）
- AI 算子初步设计（AI_filter/AI_aggregation/AI_transform）
- 多模态数据类型表示方案（图像/Embedding/Audio）
- 与 Daft 集成方案（待讨论）

### [2026-03-01] TASK-K-001：HTTP 调用 PoC 验证
- 类型：新增  |  文件：extension/ai/*  |  摘要：HTTP 调用可行性验证
- 测试：N/A (PoC 验证)  |  编译：N/A  |  commit：ac3b30716f  |  巡检：⏳

**新增文件：**
- extension/ai/README.md - Extension 概述
- extension/ai/CMakeLists.txt - M0 最小配置
- extension/ai/HTTP_POC_FINDINGS.md - HTTP 验证报告
- extension/ai/src/http_poc.cpp - PoC 代码框架

**验证结论：** ✅ httplib + mbedtls 可用，HTTP 调用完全可行
