# DuckDB AI Extension CMakeLists.txt
# M0 阶段：最小化配置，仅验证 httplib 链接

# 添加 httplib 头文件路径
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/httplib")

# HTTP PoC 测试程序（独立可执行文件）
add_executable(http_poc_test
    src/http_poc.cpp
)

# 链接必要的库
# 注意：httplib 是 header-only，不需要链接
# 如需 HTTPS 支持，需要链接 mbedtls

# M0 阶段：仅编译验证，不作为扩展加载
# TODO M1: 添加完整的 Extension 配置
