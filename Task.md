# 任务看板 — duckdb
> Tech Lead 写入 | Teammate 执行

## 状态：⏳ 待启动 | 🔄 进行中 | 🔍 待巡检 | ✅ 通过 | ❌ 打回

---

## 当前任务

### TASK-K-001：M0 环境验证与架构摸底
**状态**：⏳ 待启动  |  **优先级**：🔴 高（阻塞所有后续任务）

**验收标准**：
- [ ] 开发环境可用（编译/导入命令 + 完整原始日志）
- [ ] 现有测试套件全通过（提供完整输出）
- [ ] ARCH_NOTES.md 内容完整（见下方要求）
- [ ] Discussion.md 中发起架构方案讨论并等待 Tech Lead 确认
- [ ] 在 duckdb/ submodule 内完成 commit，通知 Tech Lead sync

**ARCH_NOTES.md 要求**：
- 目标 DuckDB 版本选型及理由
- Extension API 能力矩阵（Scalar UDF / Aggregate UDF / 自定义类型 / HTTP 调用）
- AI_filter / AI_aggregation / AI_transform 初步设计方案
- 多模态数据类型在 DuckDB 中的表示方案
- Python bindings 接口暴露方案

**预期输出物**：
- `duckdb/ARCH_NOTES.md`
- `duckdb/CHANGES.md` 更新
- duckdb/ submodule 内的 git commit

---

## 历史任务
（暂无）
