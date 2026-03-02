# 任务看板 — duckdb
> Tech Lead 写入 | Teammate 执行

## 状态：⏳ 待启动 | 🔄 进行中 | 🔍 待巡检 | ✅ 通过 | ❌ 打回

---

## 当前任务

### TASK-PROD-002：AI API批处理优化
**状态**：🔄 进行中  |  **优先级**：🟢 中（性能优化）

**任务目标**：实现AI API批处理优化，减少调用延迟，提高吞吐量。

**验收标准**：
- [ ] 实现 ai_filter_batch 函数，支持批量处理
- [ ] 修复多行处理时的bug（使用unified vector data）
- [ ] 验证批处理性能提升
- [ ] 更新CHANGES.md和Discussion.md完成报告
- [ ] 在 duckdb/ submodule 内完成 commit，通知 Tech Lead sync

**技术方案**：
- 方案A：批处理API调用（推荐）
- 方案B：并发请求（备选）

**风险预警**：
- ScalarFunction向量化限制
- API批量支持限制
- 内存占用增加

---

## 历史任务

### ✅ TASK-K-001：M0 环境验证与架构摸底
**状态**：✅ 通过

**完成内容**：
- ✅ 开发环境可用（DuckDB v1.4.4）
- ✅ 现有测试套件全通过
- ✅ ARCH_NOTES.md 完整
- ✅ Extension API能力矩阵完成
- ✅ HTTP调用可行性验证

### ✅ TASK-K-003：AI_filter MVP实现
**状态**：✅ 通过

### ✅ TASK-15：M3 HTTP AI集成
**状态**：✅ 通过

### ✅ TASK-16：Real HTTP Integration
**状态**：✅ 通过

### ✅ TASK-K-019：修复AI过滤器真实数据集成
**状态**：✅ 通过

### ✅ TASK-K-020：构建DuckDB CLI v1.4.4
**状态**：✅ 通过

### ✅ TASK-FIX-001/TASK-FIX-002：Demo修复
**状态**：✅ 通过
