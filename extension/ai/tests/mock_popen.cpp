/*
 * Mock popen for coverage testing
 * 编译: clang++ -shared -fPIC -D_GNU_SOURCE mock_popen.cpp -o mock_popen.dylib
 * 使用: LD_PRELOAD=./mock_popen.dylib <test_command>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

// 原始 popen 函数
FILE* (*real_popen)(const char*, const char*) = NULL;

// 初始化时获取真实 popen
__attribute__((constructor))
void init_mock_popen() {
    real_popen = (FILE*(*)(const char*, const char*))dlsym(RTLD_NEXT, "popen");
}

// Mock 环境变量控制
// MOCK_POPEN_FAIL=num: 前 num 次 popen 调用失败
// MOCK_POPEN_RETURN_NULL=1: 所有 popen 返回 NULL

static int fail_count = 0;
static int fail_limit = 0;

FILE* popen(const char* command, const char* mode) {
    // 检查环境变量
    const char* fail_env = getenv("MOCK_POPEN_FAIL");
    const char* null_env = getenv("MOCK_POPEN_RETURN_NULL");
    
    // 初始化失败计数
    if (fail_env && fail_limit == 0) {
        fail_limit = atoi(fail_env);
        fail_count = 0;
    }
    
    // 检查是否应该返回 NULL
    if (null_env || (fail_env && fail_count < fail_limit)) {
        if (fail_env) {
            fail_count++;
            fprintf(stderr, "[MOCK_POPEN] Simulating failure (%d/%d)\\n", fail_count, fail_limit);
        }
        return NULL;
    }
    
    // 调用真实 popen
    return real_popen(command, mode);
}

// 同样 mock pclose
int (*real_pclose)(FILE*) = NULL;

int pclose(FILE* stream) {
    if (!real_pclose) {
        real_pclose = (int(*)(FILE*))dlsym(RTLD_NEXT, "pclose");
    }
    
    const char* fail_env = getenv("MOCK_PCLOSE_FAIL");
    if (fail_env && atoi(fail_env) > 0) {
        fprintf(stderr, "[MOCK_PCLOSE] Simulating failure\\n");
        return -1;  // 模拟失败
    }
    
    return real_pclose(stream);
}
