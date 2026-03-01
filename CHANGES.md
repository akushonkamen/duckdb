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

### [2026-03-01] TASK-K-003：AI_filter 算子 MVP
- 类型：新增  |  文件：ai_extension_loadable.cpp  |  摘要：第一个可工作的 AI 算子
- 测试：✅ 手动测试通过  |  编译：✅  |  commit：e482276c1b  |  巡检：⏳

**新增文件：**
- extension/ai/ai_extension_loadable.cpp - AI_filter 实现（MVP mock 版本）
- test/extension/CMakeLists.txt - 添加 AI extension 到构建系统
- extension/ai/test_ai_extension.py - Python 测试脚本
- test/extension/ai.duckdb_extension - 编译产物 (25MB)

**功能验证：**
- ✅ 单行调用：`ai_filter(image_blob, 'cat', 'clip')`
- ✅ 多行处理：`FROM range(5)`
- ✅ WHERE 子句：`WHERE ai_filter(...) > 0.3`
- ✅ 返回类型：DOUBLE (0.0-1.0 随机值)

**MVP 实现细节：**
- 使用 FunctionLocalState 管理随机数生成器
- FunctionStability::VOLATILE (非确定性)
- M2 将替换为真实 HTTP AI 调用

---
