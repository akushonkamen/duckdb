# 任务看板 — duckdb
> Tech Lead 写入 | Teammate 执行

## 状态：⏳ 待启动 | 🔄 进行中 | 🔍 待巡检 | ✅ 通过 | ❌ 打回

---

## 当前任务

### ✅ TASK-CI-002：Extension 自动化测试
**状态**：✅ 通过  |  **优先级**：🟡 高（基础设施）

**任务目标**：将 Extension 编译和测试集成到 CI/CD。

**验收标准**：
- [x] GitHub Actions 中编译 DuckDB Extension
- [x] 运行 Extension 单元测试
- [x] 测试基本功能（ai_filter 注册和调用）
- [x] 使用 Mock 服务器避免外部 API 依赖

**交付物**：
- [x] `.github/workflows/extension_test.yml` - Extension 专用 workflow
- [x] `tests/test_mock_ai_server.py` - Mock AI 服务器（Python）
- [x] CI workflow 完整

---

### 🔄 TASK-OPS-001：错误处理和重试机制
**状态**：⏳ 待启动  |  **优先级**：🟢 中（生产稳定性）

**任务目标**：增强错误处理，添加重试机制，提高生产稳定性。

**验收标准**：
- [ ] HTTP 调用失败自动重试（指数退避）
- [ ] 超时处理（可配置超时时间）
- [ ] 降级策略（N 次失败后返回默认值）
- [ ] 结构化错误日志
- [ ] 可配置的重试参数（环境变量）

**交付物**：
- [ ] `http_client.{cpp,hpp}` 更新 - 添加重试逻辑
- [ ] `ai_filter.cpp` - 集成错误处理
- [ ] 错误统计功能（调用次数、失败次数）

---

## 当前任务

### ✅ TASK-PROD-002：AI API批处理优化
**状态**：✅ 通过  |  **优先级**：🟢 中（性能优化）

**任务目标**：实现AI API批处理优化，减少调用延迟，提高吞吐量。

**验收标准**：
- [x] 实现 ai_filter_batch 函数，支持批量处理
- [x] 修复多行处理时的bug（使用unified vector data）
- [x] 验证批处理性能提升
- [x] 更新CHANGES.md和Discussion.md完成报告
- [x] 在 duckdb/ submodule 内完成 commit，通知 Tech Lead sync

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
