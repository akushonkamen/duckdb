#!/bin/bash
# DuckDB 扩展全量测试 — 在 duckdb/ submodule 内运行
# 可从 submodule 内直接运行，也可被协调仓库 pre-commit hook 调用
set -e

# 定位到 submodule 根目录（无论从哪里调用都正确）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== DuckDB Extension Full Test Suite ==="
echo "Branch : $(git branch --show-current 2>/dev/null || echo 'unknown')"
echo "Commit : $(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"
echo "WorkDir: $SCRIPT_DIR"
echo ""

# M0 阶段：检查 Python 环境与 duckdb 安装状态
PYTHON=$(command -v python3 || command -v python || echo "")
if [ -z "$PYTHON" ]; then
    echo "⚠️  python3 未找到，跳过 import 检查（请先配置 Python 环境）"
else
    $PYTHON -c "import duckdb; print('✅ duckdb import OK, version:', duckdb.__version__)"         && echo ""         || echo "⚠️  duckdb python 包未安装（M0 阶段正常，可 pip install duckdb）"
fi

# 实现后逐步取消注释：
# cd build && ctest --output-on-failure 2>&1 | tee "$SCRIPT_DIR/test_results.log"
# $PYTHON -m pytest "$SCRIPT_DIR/tests/extension/" -v --tb=short 2>&1 | tee -a "$SCRIPT_DIR/test_results.log"

echo "RESULT: M0 environment check done"
