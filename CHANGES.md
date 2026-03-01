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
- 测试：pass 1/1 (run_tests.sh)  |  编译：✅  |  commit：(待commit)  |  巡检：⏳

**新增内容：**
- 版本选型：DuckDB v1.4.4
- Extension API 能力矩阵（Scalar/Aggregate UDF、自定义类型、HTTP 调用）
- AI 算子初步设计（AI_filter/AI_aggregation/AI_transform）
- 多模态数据类型表示方案（图像/Embedding/Audio）
- 与 Daft 集成方案（待讨论）
