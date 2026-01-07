# 03. xoron_crypto.cpp - Cryptographic Operations

**File Path**: `src/src/xoron_crypto.cpp`  
**Size**: 20,877 bytes  
**Lines**: 707

**Platform**: Cross-platform (requires OpenSSL)

## Overview
Comprehensive cryptographic utilities using OpenSSL. Provides hash functions, encryption/decryption, HMAC, encoding/decoding, and random number generation.

## Includes
- `xoron.h` (local)
- `cstdlib` (system)
- `cstring` (system)
- `cstdio` (system)
- `string` (system)
- `vector` (system)
- `openssl/sha.h` (OpenSSL)
- `openssl/md5.h` (OpenSSL)
- `openssl/evp.h` (OpenSSL)
- `openssl/hmac.h` (OpenSSL)
- `openssl/rand.h` (OpenSSL)
- `openssl/aes.h` (OpenSSL)
- `openssl/err.h` (OpenSSL)
- `lua.h` (local - Luau)
- `lualib.h` (local - Luau)

## C API Functions

### Hash Functions

#### xoron_sha256
```cpp
void xoron_sha256(const void* data, size_t len, uint8_t out[32])
```
Computes SHA-256 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (32 bytes)

#### xoron_sha384
```cpp
void xoron_sha384(const void* data, size_t len, uint8_t out[48])
```
Computes SHA-384 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (48 bytes)

#### xoron_sha512
```cpp
void xoron_sha512(const void* data, size_t len, uint8_t out[64])
```
Computes SHA-512 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (64 bytes)

#### xoron_md5
```cpp
void xoron_md5(const void* data, size_t len, uint8_t out[16])
```
Computes MD5 hash.

**Parameters**:
- `data`: Input data
- `len`: Data length
- `out`: Output buffer (16 bytes)

### Encoding Functions

#### xoron_base64_encode
```cpp
char* xoron_base64_encode(const void* data, size_t len)
```
Encodes data to Base64.

**Parameters**:
- `data`: Input data
- `len`: Data length

**Returns**: 
- Allocated Base64 string (must be freed with `xoron_free()`)
- NULL on allocation failure

#### xoron_base64_decode
```cpp
uint8_t* xoron_base64_decode(const char* str, size_t* out_len)
```
Decodes Base64 string to data.

**Parameters**:
- `str`: Base64 string
- `out_len`: Output for decoded length

**Returns**:
- Allocated decoded data (must be freed with `xoron_free()`)
- NULL on decode failure

#### xoron_hex_encode
```cpp
char* xoron_hex_encode(const void* data, size_t len)
```
Encodes data to hexadecimal string.

**Parameters**:
- `data`: Input data
- `len`: Data length

**Returns**: 
- Allocated hex string (must be freed with `xoron_free()`)
- Format: lowercase hex, no separator

#### xoron_hex_decode
```cpp
uint8_t* xoron_hex_decode(const char* str, size_t* out_len)
```
Decodes hexadecimal string to data.

**Parameters**:
- `str`: Hex string (must be even length)
- `out_len`: Output for decoded length

**Returns**:
- Allocated decoded data (must be freed with `xoron_free()`)
- NULL on invalid hex string

### Memory Management

#### xoron_free
```cpp
void xoron_free(void* ptr)
```
Frees memory allocated by crypto functions.

**Parameters**:
- `ptr`: Memory to free

## Lua API

All functions are registered in the `crypt` global table.

### Hash Functions (Lua)

#### crypt.hash
```lua
crypt.hash(algorithm: string, data: string) -> string
```

**Algorithms**: "sha256", "sha384", "sha512", "md5"

**Example**:
```lua
local hash = crypt.hash("sha256", "hello world")
-- Returns: "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"
```

#### crypt.sha256
```lua
crypt.sha256(data: string) -> string
```
SHA-256 hash shortcut.

#### crypt.sha384
```lua
crypt.sha384(data: string) -> string
```
SHA-384 hash shortcut.

#### crypt.sha512
```lua
crypt.sha512(data: string) -> string
```
SHA-512 hash shortcut.

#### crypt.md5
```lua
crypt.md5(data: string) -> string
```
MD5 hash shortcut.

### HMAC Functions (Lua)

#### crypt.hmac
```lua
crypt.hmac(algorithm: string, data: string, key: string) -> string
```

**Algorithms**: "sha256", "sha384", "sha512", "md5"

**Example**:
```lua
local mac = crypt.hmac("sha256", "message", "secret-key")
```

### Encryption/Decryption (Lua)

#### crypt.encrypt
```lua
crypt.encrypt(algorithm: string, data: string, key: string, iv: string?) -> string
```

**Algorithms**:
- "aes-256-cbc" - AES-256 CBC mode
- "aes-256-gcm" - AES-256 GCM mode

**Parameters**:
- `algorithm`: Encryption algorithm
- `data`: Data to encrypt
- `key`: Encryption key (32 bytes for AES-256)
- `iv`: Initialization vector (16 bytes for CBC, 12 bytes for GCM, optional for GCM)

**Returns**: Base64 encoded encrypted data

**Example**:
```lua
local key = string.rep("A", 32)  -- 32 bytes
local iv = string.rep("B", 16)   -- 16 bytes
local encrypted = crypt.encrypt("aes-256-cbc", "secret message", key, iv)
```

#### crypt.decrypt
```lua
crypt.decrypt(algorithm: string, data: string, key: string, iv: string?) -> string
```

**Parameters**: Same as encrypt

**Returns**: Decrypted data

**Example**:
```lua
local decrypted = crypt.decrypt("aes-256-cbc", encrypted, key, iv)
```

### Random Generation (Lua)

#### crypt.random
```lua
crypt.random(length: number) -> string
```
Generates cryptographically secure random bytes.

**Parameters**:
- `length`: Number of bytes to generate

**Returns**: Random bytes as string

**Example**:
```lua
local random_bytes = crypt.random(32)
```

#### crypt.generatebytes
```lua
crypt.generatebytes(length: number) -> string
```
Alias for `crypt.random()`.

#### crypt.generatekey
```lua
crypt.generatekey(length: number) -> string
```
Generates a random key (alias for random).

### Encoding/Decoding (Lua)

#### crypt.base64encode
```lua
crypt.base64encode(data: string) -> string
```
Encodes string to Base64.

#### crypt.base64decode
```lua
crypt.base64decode(base64: string) -> string
```
Decodes Base64 to string.

#### crypt.hexencode
```lua
crypt.hexencode(data: string) -> string
```
Encodes string to hexadecimal.

#### crypt.hexdecode
```lua
crypt.hexdecode(hex: string) -> string
```
Decodes hexadecimal to string.

## Implementation Details

### AES Encryption

#### CBC Mode
- Block size: 16 bytes
- Key size: 32 bytes (AES-256)
- IV size: 16 bytes
- Padding: PKCS#7
- Authentication: None (use HMAC for integrity)

#### GCM Mode
- Block size: 16 bytes
- Key size: 32 bytes (AES-256)
- IV size: 12 bytes (recommended)
- Authentication: Built-in (16-byte tag appended)
- Additional authenticated data: Not currently supported

### Error Handling

All functions use `xoron_set_error()` for error reporting.

**Common Errors**:
- Invalid algorithm name
- Wrong key/IV length
- Decryption failure (wrong key/corrupted data)
- Memory allocation failure
- OpenSSL errors

### Security Considerations

1. **MD5**: Cryptographically broken, use only for legacy compatibility
2. **CBC Mode**: No authentication, vulnerable to padding oracle attacks
3. **GCM Mode**: Provides authentication, preferred for new code
4. **Random**: Uses OpenSSL's CSPRNG (cryptographically secure)
5. **Key Management**: Keys should be generated using `crypt.random()` or `crypt.generatekey()`

## Usage Examples

### Basic Hashing
```lua
-- Hash a string
local data = "hello world"
local sha256 = crypt.sha256(data)
print("SHA-256:", sha256)

-- Hash with specific algorithm
local md5 = crypt.hash("md5", data)
print("MD5:", md5)
```

### Encoding
```lua
-- Base64
local encoded = crypt.base64encode("Hello, World!")
local decoded = crypt.base64decode(encoded)

-- Hex
local hex = crypt.hexencode("Hello")
local bytes = crypt.hexdecode(hex)
```

### Encryption
```lua
-- Generate random key and IV
local key = crypt.random(32)  -- AES-256 key
local iv = crypt.random(16)   -- CBC IV

-- Encrypt
local message = "Secret message"
local encrypted = crypt.encrypt("aes-256-cbc", message, key, iv)

-- Decrypt
local decrypted = crypt.decrypt("aes-256-cbc", encrypted, key, iv)
assert(decrypted == message)
```

### HMAC
```lua
-- Message authentication
local message = "Important data"
local key = crypt.random(32)
local mac = crypt.hmac("sha256", message, key)

-- Verify (compare with received MAC)
local expected = crypt.hmac("sha256", message, key)
if mac == expected then
    print("Message authentic")
end
```

### Random Generation
```lua
-- Generate random token
local token = crypt.random(32)
print("Token:", crypt.hexencode(token))

-- Generate password
local password = crypt.generatekey(16)
print("Password:", password)
```

### Complete Example
```lua
-- Secure communication example
local function encrypt_message(message, password)
    -- Derive key from password (simplified)
    local key = crypt.sha256(password)
    local iv = crypt.random(16)
    local encrypted = crypt.encrypt("aes-256-cbc", message, key:sub(1, 32), iv)
    return crypt.base64encode(iv .. encrypted)
end

local function decrypt_message(encrypted_b64, password)
    local decoded = crypt.base64decode(encrypted_b64)
    local iv = decoded:sub(1, 16)
    local encrypted = decoded:sub(17)
    local key = crypt.sha256(password)
    return crypt.decrypt("aes-256-cbc", encrypted, key:sub(1, 32), iv)
end

-- Usage
local message = "Top secret information"
local password = "my-secret-password"

local encrypted = encrypt_message(message, password)
local decrypted = decrypt_message(encrypted, password)

print("Original:", message)
print("Encrypted:", encrypted)
print("Decrypted:", decrypted)
```

## Performance Notes

- Hash functions: Very fast (native OpenSSL)
- Base64: Fast, uses optimized EVP functions
- AES: Hardware-accelerated on most platforms
- Random: Uses OS CSPRNG, fast enough for most use cases

## Related Files
- `xoron.h` (API declarations)
- `xoron_luau.cpp` (basic crypto bindings)
- `xoron_http.cpp` (uses crypto for SSL)
