/*
 * test_utils.h - Common test utilities for Xoron integration tests
 * Cross-platform utilities for iOS and Android
 */

#ifndef XORON_TEST_UTILS_H
#define XORON_TEST_UTILS_H

#include <string>
#include <vector>
#include <chrono>
#include <sstream>

// Platform detection for test utilities
#if defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #define TEST_LOG(...) NSLog(@__VA_ARGS__)
#elif defined(XORON_PLATFORM_ANDROID)
    #include <android/log.h>
    #define TEST_LOG(...) __android_log_print(ANDROID_LOG_INFO, "XoronTest", __VA_ARGS__)
#else
    #include <cstdio>
    #define TEST_LOG(...) printf(__VA_ARGS__); printf("\n")
#endif

// Test result structure
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double duration_ms;
};

// Test suite class
class TestSuite {
private:
    std::vector<TestResult> results;
    std::string suiteName;
    
public:
    TestSuite(const std::string& name) : suiteName(name) {}
    
    void recordResult(const std::string& testName, bool passed, 
                     const std::string& message = "", double duration = 0.0) {
        results.push_back({testName, passed, message, duration});
        
        if (passed) {
            TEST_LOG("[PASS] %s: %s (%.2f ms)", suiteName.c_str(), testName.c_str(), duration);
        } else {
            TEST_LOG("[FAIL] %s: %s - %s (%.2f ms)", suiteName.c_str(), testName.c_str(), 
                    message.c_str(), duration);
        }
    }
    
    void printSummary() {
        int passed = 0;
        int failed = 0;
        
        for (const auto& result : results) {
            if (result.passed) passed++;
            else failed++;
        }
        
        TEST_LOG("========================================");
        TEST_LOG("Test Suite: %s", suiteName.c_str());
        TEST_LOG("Passed: %d, Failed: %d, Total: %d", passed, failed, results.size());
        TEST_LOG("========================================");
    }
    
    const std::vector<TestResult>& getResults() const {
        return results;
    }
};

// Timer utility
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
    }
    
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
};

// String utilities
namespace StringUtils {
    inline std::string format(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return std::string(buffer);
    }
    
    inline bool contains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }
}

// Assertion utilities
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        TEST_LOG("ASSERTION FAILED: %s", message); \
        return false; \
    }

#define TEST_ASSERT_EQ(actual, expected, message) \
    if ((actual) != (expected)) { \
        TEST_LOG("ASSERTION FAILED: %s (expected %d, got %d)", message, expected, actual); \
        return false; \
    }

#define TEST_ASSERT_STR_EQ(actual, expected, message) \
    if (strcmp(actual, expected) != 0) { \
        TEST_LOG("ASSERTION FAILED: %s (expected '%s', got '%s')", message, expected, actual); \
        return false; \
    }

// Memory test utilities
namespace MemoryUtils {
    inline bool verifyPattern(void* ptr, size_t size, unsigned char pattern) {
        unsigned char* bytePtr = (unsigned char*)ptr;
        for (size_t i = 0; i < size; i++) {
            if (bytePtr[i] != pattern) return false;
        }
        return true;
    }
    
    inline void fillPattern(void* ptr, size_t size, unsigned char pattern) {
        memset(ptr, pattern, size);
    }
}

// File test utilities
namespace FileUtils {
    inline bool fileExists(const char* path) {
        FILE* fp = fopen(path, "r");
        if (fp) {
            fclose(fp);
            return true;
        }
        return false;
    }
    
    inline std::string readFile(const char* path) {
        FILE* fp = fopen(path, "r");
        if (!fp) return "";
        
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        std::string content(size, '\0');
        fread(&content[0], 1, size, fp);
        fclose(fp);
        
        return content;
    }
}

#endif // XORON_TEST_UTILS_H
