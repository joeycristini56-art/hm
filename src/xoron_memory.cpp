/*
 * xoron_memory.cpp - Memory utilities and anti-detection for executor
 * Provides: Memory scanning, pattern matching, and anti-detection mechanisms
 * Platforms: iOS 15+ (.dylib) and Android 10+ (.so)
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>

/* Platform-specific includes using unified macros from xoron.h */
#if defined(XORON_PLATFORM_IOS)
    #include <Foundation/Foundation.h>
    #include <mach/mach.h>
    #include <mach/vm_map.h>
    #include <mach-o/dyld.h>
    #include <dlfcn.h>
    #include <sys/sysctl.h>
    #include <unistd.h>
    #define MEM_LOG(...) NSLog(@__VA_ARGS__)
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <sys/ptrace.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <elf.h>
    #include <link.h>
    #include <android/log.h>
    #include <sys/system_properties.h>
    #define MEM_LOG_TAG "XoronMemory"
    #define MEM_LOG(...) __android_log_print(ANDROID_LOG_INFO, MEM_LOG_TAG, __VA_ARGS__)
#else
    #include <dlfcn.h>
    #include <unistd.h>
    #define MEM_LOG(...) printf(__VA_ARGS__)
#endif

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

static std::mutex g_mem_mutex;
static bool g_anti_detection_enabled = true;

/* Android 10+ specific: Check API level */
#if defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
static int get_android_api_level() {
    char sdk_ver_str[PROP_VALUE_MAX];
    if (__system_property_get("ro.build.version.sdk", sdk_ver_str)) {
        return atoi(sdk_ver_str);
    }
    return 0;
}

static bool is_android_10_or_higher() {
    static int api_level = get_android_api_level();
    return api_level >= 29; /* Android 10 = API 29 */
}
#endif

/* Anti-detection: Check if being debugged */
static bool is_debugger_present() {
#if defined(XORON_PLATFORM_IOS) || defined(__APPLE__)
    int mib[4];
    struct kinfo_proc info;
    size_t size = sizeof(info);
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();
    
    info.kp_proc.p_flag = 0;
    if (sysctl(mib, 4, &info, &size, NULL, 0) == -1) {
        return false;
    }
    
    return (info.kp_proc.p_flag & P_TRACED) != 0;
    
#elif defined(XORON_PLATFORM_ANDROID) || defined(__ANDROID__)
    /* Method 1: Check TracerPid in /proc/self/status */
    char buf[4096];
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        
        if (n > 0) {
            buf[n] = '\0';
            const char* tracer = strstr(buf, "TracerPid:");
            if (tracer) {
                int pid = atoi(tracer + 10);
                if (pid != 0) return true;
            }
        }
    }
    
    /* Method 2: Check for common debugger processes (Android 10+) */
    if (is_android_10_or_higher()) {
        /* Check /proc for debugger processes */
        const char* debugger_names[] = {
            "gdb", "lldb", "frida", "ida", "radare2", "r2"
        };
        
        FILE* fp = popen("ps -A 2>/dev/null", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                for (const char* name : debugger_names) {
                    if (strstr(line, name)) {
                        pclose(fp);
                        return true;
                    }
                }
            }
            pclose(fp);
        }
    }
    
    return false;
#else
    return false;
#endif
}

/* Anti-detection: Timing check */
static bool timing_check() {
    auto start = std::chrono::high_resolution_clock::now();
    
    /* Do some work */
    volatile int x = 0;
    for (int i = 0; i < 1000; i++) {
        x += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // If it took too long, might be under analysis
    return duration < 10000; // 10ms threshold
}

// Generate random delay to avoid pattern detection
static void random_delay() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 50);
    
    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
}

// Memory protection utilities - iOS and Android implementations

#if defined(XORON_PLATFORM_APPLE)
static bool protect_memory(void* addr, size_t size, vm_prot_t prot) {
    kern_return_t kr = vm_protect(mach_task_self(), 
                                   (vm_address_t)addr, 
                                   size, 
                                   FALSE, 
                                   prot);
    return kr == KERN_SUCCESS;
}

static bool read_memory(void* dest, void* src, size_t size) {
    vm_size_t out_size;
    kern_return_t kr = vm_read_overwrite(mach_task_self(),
                                          (vm_address_t)src,
                                          size,
                                          (vm_address_t)dest,
                                          &out_size);
    return kr == KERN_SUCCESS && out_size == size;
}

static bool write_memory(void* dest, const void* src, size_t size) {
    // Make memory writable
    if (!protect_memory(dest, size, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY)) {
        return false;
    }
    
    kern_return_t kr = vm_write(mach_task_self(),
                                 (vm_address_t)dest,
                                 (vm_offset_t)src,
                                 (mach_msg_type_number_t)size);
    
    // Restore protection
    protect_memory(dest, size, VM_PROT_READ | VM_PROT_EXECUTE);
    
    return kr == KERN_SUCCESS;
}

#elif defined(XORON_PLATFORM_ANDROID)

static bool protect_memory(void* addr, size_t size, int prot) {
    // Align to page boundary
    uintptr_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t aligned_addr = (uintptr_t)addr & ~(page_size - 1);
    size_t aligned_size = size + ((uintptr_t)addr - aligned_addr);
    
    return mprotect((void*)aligned_addr, aligned_size, prot) == 0;
}

static bool read_memory(void* dest, void* src, size_t size) {
    // On Android, we can try direct memory access via /proc/self/mem
    int fd = open("/proc/self/mem", O_RDONLY);
    if (fd < 0) {
        // Fallback to direct copy (may crash if memory is not readable)
        memcpy(dest, src, size);
        return true;
    }
    
    if (lseek(fd, (off_t)(uintptr_t)src, SEEK_SET) == -1) {
        close(fd);
        return false;
    }
    
    ssize_t n = read(fd, dest, size);
    close(fd);
    return n == (ssize_t)size;
}

static bool write_memory(void* dest, const void* src, size_t size) {
    // Make memory writable
    if (!protect_memory(dest, size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        return false;
    }
    
    memcpy(dest, src, size);
    
    // Restore protection (read + execute)
    protect_memory(dest, size, PROT_READ | PROT_EXEC);
    
    return true;
}

#else
// Fallback implementations
static bool read_memory(void* dest, void* src, size_t size) {
    (void)dest; (void)src; (void)size;
    return false;
}

static bool write_memory(void* dest, const void* src, size_t size) {
    (void)dest; (void)src; (void)size;
    return false;
}
#endif

// Pattern scanning
static std::vector<uint8_t> parse_pattern(const char* pattern) {
    std::vector<uint8_t> bytes;
    const char* p = pattern;
    
    while (*p) {
        if (*p == ' ') {
            p++;
            continue;
        }
        
        if (*p == '?') {
            bytes.push_back(0);
            p++;
            if (*p == '?') p++;
            continue;
        }
        
        char hex[3] = {0};
        hex[0] = *p++;
        if (*p && *p != ' ') {
            hex[1] = *p++;
        }
        
        bytes.push_back((uint8_t)strtol(hex, nullptr, 16));
    }
    
    return bytes;
}

static std::vector<uint8_t> parse_mask(const char* pattern) {
    std::vector<uint8_t> mask;
    const char* p = pattern;
    
    while (*p) {
        if (*p == ' ') {
            p++;
            continue;
        }
        
        if (*p == '?') {
            mask.push_back(0);
            p++;
            if (*p == '?') p++;
            continue;
        }
        
        mask.push_back(1);
        p++;
        if (*p && *p != ' ') p++;
    }
    
    return mask;
}

// Lua bindings

// isdebuggerpresent() - Check if debugger is attached
static int lua_isdebuggerpresent(lua_State* L) {
    lua_pushboolean(L, is_debugger_present());
    return 1;
}

// getbaseaddress() - Get base address of main executable
static int lua_getbaseaddress(lua_State* L) {
#if defined(XORON_PLATFORM_APPLE)
    const struct mach_header* header = _dyld_get_image_header(0);
    lua_pushinteger(L, (lua_Integer)(uintptr_t)header);
#elif defined(XORON_PLATFORM_ANDROID)
    // Get base address from /proc/self/maps
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        if (fgets(line, sizeof(line), maps)) {
            uintptr_t base = 0;
            sscanf(line, "%lx", &base);
            fclose(maps);
            lua_pushinteger(L, (lua_Integer)base);
            return 1;
        }
        fclose(maps);
    }
    lua_pushinteger(L, 0);
#else
    lua_pushinteger(L, 0);
#endif
    return 1;
}

// getmodulebase(name) - Get base address of a module
static int lua_getmodulebase(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    
#if defined(XORON_PLATFORM_APPLE)
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; i++) {
        const char* image_name = _dyld_get_image_name(i);
        if (image_name && strstr(image_name, name)) {
            const struct mach_header* header = _dyld_get_image_header(i);
            lua_pushinteger(L, (lua_Integer)(uintptr_t)header);
            return 1;
        }
    }
#elif defined(XORON_PLATFORM_ANDROID)
    // Search in /proc/self/maps for the module
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        while (fgets(line, sizeof(line), maps)) {
            if (strstr(line, name)) {
                uintptr_t base = 0;
                sscanf(line, "%lx", &base);
                fclose(maps);
                lua_pushinteger(L, (lua_Integer)base);
                return 1;
            }
        }
        fclose(maps);
    }
#else
    (void)name;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readbyte(address) - Read a byte from memory
static int lua_readbyte(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    uint8_t value = 0;
    if (read_memory(&value, (void*)addr, 1)) {
        lua_pushinteger(L, value);
        return 1;
    }
#else
    (void)addr;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readint(address) - Read an integer from memory
static int lua_readint(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    int32_t value = 0;
    if (read_memory(&value, (void*)addr, sizeof(value))) {
        lua_pushinteger(L, value);
        return 1;
    }
#else
    (void)addr;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readlong(address) - Read a long from memory
static int lua_readlong(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    int64_t value = 0;
    if (read_memory(&value, (void*)addr, sizeof(value))) {
        lua_pushinteger(L, value);
        return 1;
    }
#else
    (void)addr;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readfloat(address) - Read a float from memory
static int lua_readfloat(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    float value = 0;
    if (read_memory(&value, (void*)addr, sizeof(value))) {
        lua_pushnumber(L, value);
        return 1;
    }
#else
    (void)addr;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readdouble(address) - Read a double from memory
static int lua_readdouble(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    double value = 0;
    if (read_memory(&value, (void*)addr, sizeof(value))) {
        lua_pushnumber(L, value);
        return 1;
    }
#else
    (void)addr;
#endif
    
    lua_pushnil(L);
    return 1;
}

// readstring(address, length) - Read a string from memory
static int lua_readstring(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    size_t len = (size_t)luaL_optinteger(L, 2, 256);
    
    if (len > 65536) len = 65536; // Safety limit
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    std::vector<char> buffer(len + 1, 0);
    if (read_memory(buffer.data(), (void*)addr, len)) {
        lua_pushlstring(L, buffer.data(), strlen(buffer.data()));
        return 1;
    }
#else
    (void)addr;
    (void)len;
#endif
    
    lua_pushnil(L);
    return 1;
}

// writebyte(address, value) - Write a byte to memory
static int lua_writebyte(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    uint8_t value = (uint8_t)luaL_checkinteger(L, 2);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    lua_pushboolean(L, write_memory((void*)addr, &value, 1));
#else
    (void)addr;
    (void)value;
    lua_pushboolean(L, false);
#endif
    return 1;
}

// writeint(address, value) - Write an integer to memory
static int lua_writeint(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    int32_t value = (int32_t)luaL_checkinteger(L, 2);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    lua_pushboolean(L, write_memory((void*)addr, &value, sizeof(value)));
#else
    (void)addr;
    (void)value;
    lua_pushboolean(L, false);
#endif
    return 1;
}

// writelong(address, value) - Write a long to memory
static int lua_writelong(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    int64_t value = (int64_t)luaL_checkinteger(L, 2);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    lua_pushboolean(L, write_memory((void*)addr, &value, sizeof(value)));
#else
    (void)addr;
    (void)value;
    lua_pushboolean(L, false);
#endif
    return 1;
}

// writefloat(address, value) - Write a float to memory
static int lua_writefloat(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    float value = (float)luaL_checknumber(L, 2);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    lua_pushboolean(L, write_memory((void*)addr, &value, sizeof(value)));
#else
    (void)addr;
    (void)value;
    lua_pushboolean(L, false);
#endif
    return 1;
}

// writedouble(address, value) - Write a double to memory
static int lua_writedouble(lua_State* L) {
    uintptr_t addr = (uintptr_t)luaL_checkinteger(L, 1);
    double value = luaL_checknumber(L, 2);
    
#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    lua_pushboolean(L, write_memory((void*)addr, &value, sizeof(value)));
#else
    (void)addr;
    (void)value;
    lua_pushboolean(L, false);
#endif
    return 1;
}

// patternscan(pattern, start, size) - Scan for a pattern in memory
static int lua_patternscan(lua_State* L) {
    const char* pattern = luaL_checkstring(L, 1);
    uintptr_t start = (uintptr_t)luaL_optinteger(L, 2, 0);
    size_t size = (size_t)luaL_optinteger(L, 3, 0x1000000);
    
    std::vector<uint8_t> bytes = parse_pattern(pattern);
    std::vector<uint8_t> mask = parse_mask(pattern);
    
    if (bytes.empty()) {
        lua_pushnil(L);
        return 1;
    }
    
#if defined(XORON_PLATFORM_APPLE)
    if (start == 0) {
        start = (uintptr_t)_dyld_get_image_header(0);
    }
#elif defined(XORON_PLATFORM_ANDROID)
    if (start == 0) {
        // Get base address from /proc/self/maps
        FILE* maps = fopen("/proc/self/maps", "r");
        if (maps) {
            char line[512];
            if (fgets(line, sizeof(line), maps)) {
                sscanf(line, "%lx", &start);
            }
            fclose(maps);
        }
    }
#endif

#if defined(XORON_PLATFORM_APPLE) || defined(XORON_PLATFORM_ANDROID)
    std::vector<uint8_t> buffer(size);
    if (!read_memory(buffer.data(), (void*)start, size)) {
        lua_pushnil(L);
        return 1;
    }
    
    for (size_t i = 0; i <= size - bytes.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < bytes.size(); j++) {
            if (mask[j] && buffer[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            lua_pushinteger(L, start + i);
            return 1;
        }
    }
#else
    (void)start;
    (void)size;
    (void)bytes;
    (void)mask;
#endif
    
    lua_pushnil(L);
    return 1;
}

// Register memory library
void xoron_register_memory(lua_State* L) {
    // Create memory table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_isdebuggerpresent, "isdebuggerpresent");
    lua_setfield(L, -2, "isdebuggerpresent");
    
    lua_pushcfunction(L, lua_getbaseaddress, "getbaseaddress");
    lua_setfield(L, -2, "getbaseaddress");
    
    lua_pushcfunction(L, lua_getmodulebase, "getmodulebase");
    lua_setfield(L, -2, "getmodulebase");
    
    lua_pushcfunction(L, lua_readbyte, "readbyte");
    lua_setfield(L, -2, "readbyte");
    
    lua_pushcfunction(L, lua_readint, "readint");
    lua_setfield(L, -2, "readint");
    
    lua_pushcfunction(L, lua_readlong, "readlong");
    lua_setfield(L, -2, "readlong");
    
    lua_pushcfunction(L, lua_readfloat, "readfloat");
    lua_setfield(L, -2, "readfloat");
    
    lua_pushcfunction(L, lua_readdouble, "readdouble");
    lua_setfield(L, -2, "readdouble");
    
    lua_pushcfunction(L, lua_readstring, "readstring");
    lua_setfield(L, -2, "readstring");
    
    lua_pushcfunction(L, lua_writebyte, "writebyte");
    lua_setfield(L, -2, "writebyte");
    
    lua_pushcfunction(L, lua_writeint, "writeint");
    lua_setfield(L, -2, "writeint");
    
    lua_pushcfunction(L, lua_writelong, "writelong");
    lua_setfield(L, -2, "writelong");
    
    lua_pushcfunction(L, lua_writefloat, "writefloat");
    lua_setfield(L, -2, "writefloat");
    
    lua_pushcfunction(L, lua_writedouble, "writedouble");
    lua_setfield(L, -2, "writedouble");
    
    lua_pushcfunction(L, lua_patternscan, "patternscan");
    lua_setfield(L, -2, "patternscan");
    
    lua_setglobal(L, "memory");
    
    // Also register standalone functions
    lua_pushcfunction(L, lua_isdebuggerpresent, "isdebuggerpresent");
    lua_setglobal(L, "isdebuggerpresent");
}

// C API
extern "C" {

bool xoron_check_environment(void) {
    if (!g_anti_detection_enabled) return true;
    
    // Check for debugger
    if (is_debugger_present()) {
        return false;
    }
    
    // Timing check
    if (!timing_check()) {
        return false;
    }
    
    return true;
}

void xoron_enable_anti_detection(bool enable) {
    g_anti_detection_enabled = enable;
}

}
