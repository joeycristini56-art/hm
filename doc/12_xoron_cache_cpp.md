# 12. xoron_cache.cpp - Caching System

**File Path**: `src/src/xoron_cache.cpp`  
**Size**: 19,470 bytes  
**Lines**: 649

**Platform**: Cross-platform

## Overview
Provides an in-memory caching system for the executor. Supports key-value storage with expiration, size limits, and various eviction policies.

## Includes
- `xoron.h` (local)
- `cstdlib`, `cstring`, `cstdio` (system)
- `string`, `vector`, `unordered_map` (system)
- `list`, `algorithm` (system)
- `mutex`, `thread`, `chrono` (system)
- `functional` (system)
- `lua.h`, `lualib.h` (local - Luau)

## Core Architecture

### Cache Entry
```cpp
struct CacheEntry {
    std::string key;
    std::string value;
    std::chrono::steady_clock::time_point created;
    std::chrono::steady_clock::time_point last_access;
    size_t size;
    int hits;
    int ttl;  // Time to live in seconds (0 = no expiration)
    bool pinned;  // Never evict
};
```

### Cache Policy
```cpp
enum class EvictionPolicy {
    LRU,        // Least Recently Used
    LFU,        // Least Frequently Used
    FIFO,       // First In First Out
    TTL,        // Time To Live
    SIZE        // Size-based
};
```

### Cache Manager
```cpp
struct CacheManager {
    std::unordered_map<std::string, CacheEntry> entries;
    std::list<std::string> access_order;  // For LRU
    std::unordered_map<std::string, int> frequency;  // For LFU
    std::mutex mutex;
    size_t max_size;  // Max total size in bytes
    size_t current_size;
    EvictionPolicy policy;
    bool auto_cleanup;
    int default_ttl;
    
    // Statistics
    size_t hits;
    size_t misses;
    size_t evictions;
};
```

## C API Functions

### xoron_cache_set
```c
bool xoron_cache_set(const char* key, const char* value, int ttl)
```
Sets a cache value.

**Parameters**:
- `key`: Cache key
- `value`: Value to store
- `ttl`: Time to live in seconds (0 = no expiration)

**Returns**: `true` on success

### xoron_cache_get
```c
char* xoron_cache_get(const char* key)
```
Gets a cache value.

**Parameters**:
- `key`: Cache key

**Returns**: Value string (must be freed) or NULL

### xoron_cache_has
```c
bool xoron_cache_has(const char* key)
```
Checks if key exists.

**Parameters**:
- `key`: Cache key

**Returns**: `true` if exists

### xoron_cache_delete
```c
bool xoron_cache_delete(const char* key)
```
Deletes a cache entry.

**Parameters**:
- `key`: Cache key

**Returns**: `true` if deleted

### xoron_cache_clear
```c
void xoron_cache_clear()
```
Clears all cache entries.

### xoron_cache_size
```c
size_t xoron_cache_size()
```
Gets current cache size in bytes.

**Returns**: Size in bytes

### xoron_cache_count
```c
size_t xoron_cache_count()
```
Gets number of entries.

**Returns**: Entry count

### xoron_cache_set_max_size
```c
void xoron_cache_set_max_size(size_t max_size)
```
Sets maximum cache size.

**Parameters**:
- `max_size`: Max bytes

### xoron_cache_set_policy
```c
void xoron_cache_set_policy(int policy)
```
Sets eviction policy.

**Parameters**:
- `policy`: 0=LRU, 1=LFU, 2=FIFO, 3=TTL, 4=SIZE

### xoron_cache_set_default_ttl
```c
void xoron_cache_set_default_ttl(int ttl)
```
Sets default TTL for new entries.

**Parameters**:
- `ttl`: Seconds (0 = no default)

### xoron_cache_get_stats
```c
char* xoron_cache_get_stats()
```
Gets cache statistics.

**Returns**: JSON string (must be freed)

### xoron_cache_cleanup
```c
void xoron_cache_cleanup()
```
Manually triggers cleanup.

## Lua API

All functions are registered in the `cache` global table.

### cache.set
```lua
cache.set(key: string, value: any, ttl: number?) -> boolean
```
Sets a cache value.

**Parameters**:
- `key`: Cache key
- `value`: Value (will be serialized)
- `ttl`: Time to live in seconds (optional)

**Returns**: true on success

**Example**:
```lua
cache.set("user_data", {name = "Alice", score = 100}, 300)  -- 5 min TTL
cache.set("config", {debug = true})  -- No expiration
```

### cache.get
```lua
cache.get(key: string) -> any
```
Gets cached value.

**Parameters**:
- `key`: Cache key

**Returns**: Cached value or nil

**Example**:
```lua
local data = cache.get("user_data")
if data then
    print("Found:", data.name)
end
```

### cache.has
```lua
cache.has(key: string) -> boolean
```
Checks if key exists.

**Parameters**:
- `key`: Cache key

**Returns**: true if exists

### cache.delete
```lua
cache.delete(key: string) -> boolean
```
Deletes cache entry.

**Parameters**:
- `key`: Cache key

**Returns**: true if deleted

**Example**:
```lua
cache.delete("temp_data")
```

### cache.clear
```lua
cache.clear() -> void
```
Clears all cache entries.

### cache.size
```lua
cache.size() -> number
```
Gets current cache size in bytes.

**Returns**: Size in bytes

### cache.count
```lua
cache.count() -> number
```
Gets number of entries.

**Returns**: Entry count

### cache.keys
```lua
cache.keys() -> table
```
Gets all cache keys.

**Returns**: Array of keys

**Example**:
```lua
local keys = cache.keys()
for _, key in ipairs(keys) do
    print(key)
end
```

### cache.values
```lua
cache.values() -> table
```
Gets all cache values.

**Returns**: Array of values

### cache.entries
```lua
cache.entries() -> table
```
Gets all cache entries.

**Returns**: Array of {key, value} pairs

### cache.set_max_size
```lua
cache.set_max_size(size: number) -> void
```
Sets maximum cache size.

**Parameters**:
- `size`: Max bytes (0 = unlimited)

**Example**:
```lua
cache.set_max_size(10 * 1024 * 1024)  -- 10 MB
```

### cache.set_policy
```lua
cache.set_policy(policy: string) -> void
```
Sets eviction policy.

**Parameters**:
- `policy`: "lru", "lfu", "fifo", "ttl", "size"

**Example**:
```lua
cache.set_policy("lru")  -- Least Recently Used
```

### cache.set_default_ttl
```lua
cache.set_default_ttl(ttl: number) -> void
```
Sets default TTL for new entries.

**Parameters**:
- `ttl`: Seconds (0 = no default)

**Example**:
```lua
cache.set_default_ttl(60)  -- 1 minute default
```

### cache.get_stats
```lua
cache.get_stats() -> table
```
Gets cache statistics.

**Returns**: Statistics table

**Statistics include**:
```lua
{
    entries = 10,
    size = 524288,
    max_size = 10485760,
    hits = 150,
    misses = 25,
    evictions = 5,
    policy = "lru",
    default_ttl = 60
}
```

### cache.cleanup
```lua
cache.cleanup() -> void
```
Manually triggers cleanup (removes expired entries).

### cache.enable_auto_cleanup
```lua
cache.enable_auto_cleanup(interval: number?) -> void
```
Enables automatic cleanup.

**Parameters**:
- `interval`: Cleanup interval in seconds (default: 60)

**Example**:
```lua
cache.enable_auto_cleanup(30)  -- Cleanup every 30 seconds
```

### cache.disable_auto_cleanup
```lua
cache.disable_auto_cleanup() -> void
```
Disables automatic cleanup.

### cache.get_ttl
```lua
cache.get_ttl(key: string) -> number?
```
Gets remaining TTL for entry.

**Parameters**:
- `key`: Cache key

**Returns**: Remaining seconds or nil if no TTL

### cache.set_ttl
```lua
cache.set_ttl(key: string, ttl: number) -> boolean
```
Sets TTL for existing entry.

**Parameters**:
- `key`: Cache key
- `ttl`: New TTL in seconds

**Returns**: true if updated

### cache.pin
```lua
cache.pin(key: string, pin: boolean?) -> void
```
Pins/unpins entry (prevents eviction).

**Parameters**:
- `key`: Cache key
- `pin`: true to pin (default: true)

**Example**:
```lua
cache.pin("important_data")  -- Never evict
```

### cache.is_pinned
```lua
cache.is_pinned(key: string) -> boolean
```
Checks if entry is pinned.

**Parameters**:
- `key`: Cache key

**Returns**: true if pinned

### cache.touch
```lua
cache.touch(key: string) -> boolean
```
Updates last access time.

**Parameters**:
- `key`: Cache key

**Returns**: true if updated

### cache.get_info
```lua
cache.get_info(key: string) -> table?
```
Gets detailed info about entry.

**Parameters**:
- `key`: Cache key

**Returns**: Info table or nil

**Info includes**:
```lua
{
    key = "user_data",
    size = 128,
    created = 1234567890,
    last_access = 1234567900,
    hits = 5,
    ttl = 300,
    remaining = 250,
    pinned = false
}
```

### cache.prune
```lua
cache.prune(count: number) -> number
```
Removes specified number of entries using current policy.

**Parameters**:
- `count`: Number of entries to remove

**Returns**: Number actually removed

### cache.resize
```lua
cache.resize(new_size: number) -> void
```
Resizes cache and evicts if needed.

**Parameters**:
- `new_size`: New max size in bytes

## Eviction Policies

### LRU (Least Recently Used)
Removes entries that haven't been accessed for the longest time.

**Use case**: General purpose, good for temporal locality

**Example**:
```lua
cache.set_policy("lru")
cache.set("a", 1)
cache.set("b", 2)
cache.get("a")  -- "a" is now most recent
cache.set("c", 3)  -- "b" might be evicted if at limit
```

### LFU (Least Frequently Used)
Removes entries with lowest access frequency.

**Use case**: Keep popular items, good for skewed access patterns

**Example**:
```lua
cache.set_policy("lfu")
cache.set("popular", 1)
cache.get("popular")  -- 100 times
cache.set("rare", 2)
cache.get("rare")  -- 1 time
-- "rare" will be evicted first
```

### FIFO (First In First Out)
Removes oldest entries regardless of access.

**Use case**: Simple, predictable behavior

**Example**:
```lua
cache.set_policy("fifo")
cache.set("first", 1)
cache.set("second", 2)
cache.set("third", 3)  -- "first" evicted if at limit
```

### TTL (Time To Live)
Removes expired entries first.

**Use case**: Temporary data, session storage

**Example**:
```lua
cache.set_policy("ttl")
cache.set("session", data, 300)  -- 5 min TTL
-- After 5 minutes, automatically removed
```

### SIZE
Removes largest entries first.

**Use case**: Memory-constrained environments

**Example**:
```lua
cache.set_policy("size")
cache.set("small", "x")
cache.set("large", string.rep("y", 10000))
-- "large" evicted first if space needed
```

## Usage Examples

### Simple Key-Value Store
```lua
-- Store configuration
cache.set("config", {
    debug = true,
    timeout = 30,
    retries = 3
})

-- Retrieve later
local config = cache.get("config")
if config.debug then
    print("Debug mode enabled")
end
```

### Session Management
```lua
-- User session with TTL
function create_session(user_id)
    local session = {
        user_id = user_id,
        login_time = os.time(),
        last_activity = os.time()
    }
    
    cache.set("session:" .. user_id, session, 3600)  -- 1 hour
    return session
end

function get_session(user_id)
    local session = cache.get("session:" .. user_id)
    if session then
        session.last_activity = os.time()
        cache.touch("session:" .. user_id)
    end
    return session
end
```

### API Response Caching
```lua
-- Cache API responses
function fetch_data(endpoint)
    local cache_key = "api:" .. endpoint
    
    -- Check cache
    local cached = cache.get(cache_key)
    if cached then
        print("Using cached data")
        return cached
    end
    
    -- Fetch from API
    print("Fetching from API")
    local response = http.get(endpoint)
    local data = json.decode(response)
    
    -- Cache for 5 minutes
    cache.set(cache_key, data, 300)
    
    return data
end

-- Usage
local user_data = fetch_data("https://api.example.com/users/123")
```

### Computation Cache
```lua
-- Cache expensive computations
function fibonacci(n)
    if n <= 1 then return n end
    
    local cache_key = "fib:" .. n
    local cached = cache.get(cache_key)
    if cached then return cached end
    
    local result = fibonacci(n-1) + fibonacci(n-2)
    cache.set(cache_key, result)
    return result
end

print(fibonacci(50))  -- Fast with caching
```

### Rate Limiting
```lua
-- Track API calls per user
function check_rate_limit(user_id)
    local key = "rate_limit:" .. user_id
    local count = cache.get(key) or 0
    
    if count >= 100 then
        return false  -- Limit exceeded
    end
    
    cache.set(key, count + 1, 60)  -- 1 minute window
    return true
end
```

### Temporary Storage
```lua
-- Store temporary data
cache.set("temp_upload", file_data, 300)  -- 5 min
-- Process upload...
cache.delete("temp_upload")
```

### Cache with Size Limit
```lua
-- Limit cache to 5MB
cache.set_max_size(5 * 1024 * 1024)
cache.set_policy("lru")

-- Store large objects
for i = 1, 100 do
    cache.set("data:" .. i, string.rep("x", 100000))
end
-- Old entries automatically evicted
```

### Pinned Entries
```lua
-- Important data never evicted
cache.set("critical_config", config)
cache.pin("critical_config")

-- Unpin when no longer needed
cache.pin("critical_config", false)
```

### Cache Statistics
```lua
-- Monitor cache performance
local stats = cache.get_stats()
print(string.format(
    "Hits: %d, Misses: %d, Hit Rate: %.2f%%",
    stats.hits,
    stats.misses,
    (stats.hits / (stats.hits + stats.misses)) * 100
))
```

### Auto Cleanup
```lua
-- Enable automatic cleanup
cache.enable_auto_cleanup(60)  -- Every minute

-- Store with TTL
cache.set("short_lived", data, 120)  -- 2 minutes
-- Will be automatically removed after expiration
```

### Multi-level Cache
```lua
-- L1 (fast, small) and L2 (slow, large)
local L1 = cache  -- In-memory
local L2 = fs     -- Disk

function get_data(key)
    -- Try L1
    local data = L1.get(key)
    if data then return data end
    
    -- Try L2
    local path = "workspace/cache/" .. key
    if fs.isfile(path) then
        data = json.decode(fs.readfile(path))
        L1.set(key, data, 60)  -- Cache in L1 for 1 min
        return data
    end
    
    return nil
end

function set_data(key, value)
    L1.set(key, value, 60)
    fs.writefile("workspace/cache/" .. key, json.encode(value))
end
```

### Cache Warming
```lua
-- Pre-populate cache
function warm_cache()
    local important_keys = {"config", "user_list", "settings"}
    
    for _, key in ipairs(important_keys) do
        local data = load_from_db(key)
        cache.set(key, data, 3600)  -- 1 hour
        cache.pin(key)  -- Keep in cache
    end
end
```

### Dependency Cache
```lua
-- Cache with dependencies
local dependencies = {}

function get_with_deps(key, deps)
    -- Check if any dependency changed
    for _, dep in ipairs(deps) do
        local dep_time = cache.get("timestamp:" .. dep)
        local cached_time = cache.get("timestamp:" .. key)
        
        if not cached_time or dep_time > cached_time then
            -- Recompute
            local value = compute(key, deps)
            cache.set(key, value)
            cache.set("timestamp:" .. key, os.time())
            return value
        end
    end
    
    return cache.get(key)
end
```

### Cache Preloading
```lua
-- Preload data on startup
function preload_data()
    local data = fetch_bulk_data()
    
    for key, value in pairs(data) do
        cache.set(key, value, 3600)
    end
    
    print("Preloaded", cache.count(), "entries")
end
```

### Cache Export/Import
```lua
-- Export cache to file
function export_cache()
    local entries = cache.entries()
    local export = {}
    
    for _, entry in ipairs(entries) do
        table.insert(export, {
            key = entry.key,
            value = entry.value,
            ttl = cache.get_ttl(entry.key)
        })
    end
    
    fs.writefile("cache_backup.json", json.encode(export))
end

-- Import cache from file
function import_cache()
    local data = fs.readfile("cache_backup.json")
    local entries = json.decode(data)
    
    for _, entry in ipairs(entries) do
        cache.set(entry.key, entry.value, entry.ttl)
    end
end
```

### Cache Monitoring
```lua
-- Monitor cache health
function cache_health_check()
    local stats = cache.get_stats()
    
    if stats.size > stats.max_size * 0.9 then
        warn("Cache at 90% capacity")
    end
    
    local hit_rate = stats.hits / (stats.hits + stats.misses)
    if hit_rate < 0.5 then
        warn("Low hit rate: " .. hit_rate)
    end
    
    if stats.evictions > 100 then
        info("High eviction count: " .. stats.evictions)
    end
end

-- Run periodically
spawn(function()
    while true do
        cache_health_check()
        wait(60)
    end
end)
```

### Cache-based Deduplication
```lua
-- Prevent duplicate operations
function do_once(key, operation)
    if cache.has("done:" .. key) then
        return false  -- Already done
    end
    
    operation()
    cache.set("done:" .. key, true, 0)  -- Permanent
    return true
end

-- Usage
do_once("update_config", function()
    -- Update config
end)
```

### Memory-efficient Caching
```lua
-- Store only what's needed
cache.set_max_size(2 * 1024 * 1024)  -- 2 MB
cache.set_policy("lru")

-- Compress large data
local compressed = lz4_compress(large_data)
cache.set("compressed_data", compressed)

-- Retrieve
local data = lz4_decompress(cache.get("compressed_data"), original_size)
```

### Cache with Versioning
```lua
-- Versioned cache entries
function set_versioned(key, value, version)
    cache.set(key .. ":" .. version, value)
    cache.set("latest:" .. key, version)
end

function get_versioned(key, version)
    if not version then
        version = cache.get("latest:" .. key)
    end
    return cache.get(key .. ":" .. version)
end

-- Usage
set_versioned("config", {setting = 1}, "v1")
set_versioned("config", {setting = 2}, "v2")
local latest = get_versioned("config")  -- v2
local old = get_versioned("config", "v1")  -- v1
```

### Cache-based Rate Limiting
```lua
-- Advanced rate limiting
function is_rate_limited(user_id, limit, window)
    local key = "rl:" .. user_id
    local data = cache.get(key) or {count = 0, start = os.time()}
    
    -- Reset if window expired
    if os.time() - data.start > window then
        data.count = 0
        data.start = os.time()
    end
    
    if data.count >= limit then
        return true
    end
    
    data.count = data.count + 1
    cache.set(key, data, window)
    return false
end

-- Usage
for i = 1, 20 do
    if is_rate_limited("user1", 10, 60) then
        print("Rate limited!")
        break
    end
    print("Request", i, "allowed")
end
```

### Cache-based Memoization
```lua
-- Memoize any function
function memoize(func, ttl)
    return function(...)
        local key = tostring(func) .. ":" .. table.concat({...}, ",")
        local cached = cache.get(key)
        
        if cached then return cached end
        
        local result = func(...)
        cache.set(key, result, ttl or 60)
        return result
    end
end

-- Usage
local expensive_func = memoize(function(x)
    -- Expensive computation
    return x * x
end, 300)  -- 5 min TTL

print(expensive_func(5))  -- Computed
print(expensive_func(5))  -- Cached
```

## Performance Characteristics

- **Set/Get**: O(1) average case
- **Eviction**: O(1) for LRU/FIFO, O(n) for LFU
- **Memory**: ~100 bytes overhead per entry
- **Thread Safety**: Full mutex protection
- **Cleanup**: O(n) where n = expired entries

## Memory Management

- Automatic size tracking
- Eviction when max_size exceeded
- TTL-based expiration
- Manual cleanup available
- Pinned entries never evicted

## Thread Safety

All operations are thread-safe:
- Mutex-protected access
- Atomic counters
- Safe concurrent reads
- Exclusive writes

## Error Handling

- Invalid keys return nil/false
- Out of memory returns false
- TTL violations logged
- Policy errors default to LRU

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (Lua bindings)
- `xoron_filesystem.cpp` (persistence)
