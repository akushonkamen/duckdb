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

### ✅ TASK-OPS-001：错误处理和重试机制
**状态**：✅ 通过  |  **优先级**：🟢 中（生产稳定性）

**任务目标**：增强错误处理，添加重试机制，提高生产稳定性。

**验收标准**：
- [x] HTTP 调用失败自动重试（指数退避）
- [x] 超时处理（可配置超时时间）
- [x] 降级策略（N 次失败后返回默认值）
- [x] 结构化错误日志
- [x] 可配置的重试参数（环境变量）

**交付物**：
- [x] `ai_filter.cpp` - 集成重试逻辑和错误处理
- [x] `CallAI_API_WithRetry()` - 新的重试包装函数
- [x] 环境变量支持（AI_FILTER_DEFAULT_SCORE）

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

### ✅ TASK-DOC-002：Extension 文档
**状态**：✅ 通过  |  **优先级**：🟢 中（文档）

**任务目标**：完善 DuckDB Extension 文档。

**验收标准**：
- [x] Extension API 文档（ai_filter、ai_filter_batch）
- [x] 编译和安装指南
- [x] 配置参数说明（环境变量）
- [x] 错误处理和重试机制说明
- [x] Mock 服务器使用指南（包含在 API 文档中）

**交付物**：
- [x] `docs/AI_EXTENSION_API.md` - Extension API 文档
- [x] `docs/BUILD_GUIDE.md` - 编译指南
- [x] `docs/RETRY_LOGIC.md` - 重试机制说明

---

### ✅ TASK-TEST-002：Extension 测试覆盖率提升
**状态**：✅ 通过  |  **优先级**：🔴 高（质量保证）
**巡检日期**：2026-03-03
**巡检人**：Tech Lead

**任务目标**：提升 Extension 测试覆盖率，达到目标。

**当前状态**：
- 源文件：ai_filter.cpp, ai_extension.cpp
- 现有测试：从 4 个增加到 10 个（新增 6 个增强测试）
- 测试通过率：100% (10/10)

**验收标准**：
- [x] 重试逻辑测试（Test 4: 降级分数验证）
- [x] 错误处理测试（Test 4, Test 10: NULL 处理）
- [x] 边界条件测试（Test 5: 空字符串、超长字符串）
- [x] 所有函数分支覆盖
- [x] 性能基准测试完成

**交付物**：
- [x] `tests/test_enhanced_coverage.py` - Python 版本增强测试
- [x] `tests/test_enhanced_coverage.sh` - Shell 版本（避免版本冲突）
- [x] 巡检报告：`development/inspections/TASK-TEST-002.md`

**巡检结论**：
- ✅ 测试通过率：100% (10/10)
- ✅ 重试逻辑、错误处理、边界条件全覆盖
- ⏸️ C++ 代码行覆盖率（需 gcov 集成，延后处理）

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
