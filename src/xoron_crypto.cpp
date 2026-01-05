/*
 * xoron_crypto.cpp - Comprehensive crypto utilities using OpenSSL
 * Provides: SHA256/384/512, MD5, AES-CBC/GCM, HMAC, Base64, Hex, Random
 */

#include "xoron.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/err.h>

#include "lua.h"
#include "lualib.h"

extern void xoron_set_error(const char* fmt, ...);

// ==================== C API ====================

extern "C" {

void xoron_sha256(const void* data, size_t len, uint8_t out[32]) {
    SHA256((const unsigned char*)data, len, out);
}

void xoron_sha384(const void* data, size_t len, uint8_t out[48]) {
    SHA384((const unsigned char*)data, len, out);
}

void xoron_sha512(const void* data, size_t len, uint8_t out[64]) {
    SHA512((const unsigned char*)data, len, out);
}

void xoron_md5(const void* data, size_t len, uint8_t out[16]) {
    MD5((const unsigned char*)data, len, out);
}

char* xoron_base64_encode(const void* data, size_t len) {
    size_t out_len = ((len + 2) / 3) * 4 + 1;
    char* out = (char*)malloc(out_len);
    if (!out) return nullptr;
    EVP_EncodeBlock((unsigned char*)out, (const unsigned char*)data, (int)len);
    return out;
}

uint8_t* xoron_base64_decode(const char* str, size_t* out_len) {
    size_t in_len = strlen(str);
    if (in_len == 0) {
        if (out_len) *out_len = 0;
        return (uint8_t*)calloc(1, 1);
    }
    
    size_t max_out = (in_len / 4) * 3 + 1;
    uint8_t* out = (uint8_t*)malloc(max_out);
    if (!out) return nullptr;
    
    int decoded_len = EVP_DecodeBlock(out, (const unsigned char*)str, (int)in_len);
    if (decoded_len < 0) { free(out); return nullptr; }
    
    // Adjust for padding
    if (in_len > 0 && str[in_len - 1] == '=') decoded_len--;
    if (in_len > 1 && str[in_len - 2] == '=') decoded_len--;
    
    if (out_len) *out_len = decoded_len;
    return out;
}

char* xoron_hex_encode(const void* data, size_t len) {
    char* out = (char*)malloc(len * 2 + 1);
    if (!out) return nullptr;
    
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        sprintf(out + i * 2, "%02x", bytes[i]);
    }
    out[len * 2] = '\0';
    return out;
}

uint8_t* xoron_hex_decode(const char* str, size_t* out_len) {
    size_t in_len = strlen(str);
    if (in_len % 2 != 0) return nullptr;
    
    size_t len = in_len / 2;
    uint8_t* out = (uint8_t*)malloc(len + 1);
    if (!out) return nullptr;
    
    for (size_t i = 0; i < len; i++) {
        unsigned int byte;
        if (sscanf(str + i * 2, "%2x", &byte) != 1) {
            free(out);
            return nullptr;
        }
        out[i] = (uint8_t)byte;
    }
    out[len] = '\0';
    
    if (out_len) *out_len = len;
    return out;
}

void xoron_free(void* ptr) {
    free(ptr);
}

} // extern "C"

// ==================== Internal Helpers ====================

static std::string bytes_to_hex(const uint8_t* data, size_t len) {
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", data[i]);
        result += buf;
    }
    return result;
}

static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> result;
    if (hex.length() % 2 != 0) return result;
    
    result.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned int byte;
        if (sscanf(hex.c_str() + i, "%2x", &byte) == 1) {
            result.push_back((uint8_t)byte);
        }
    }
    return result;
}

// ==================== Lua Bindings ====================

// crypt.generatebytes(count) - Generate random bytes
static int lua_crypt_generatebytes(lua_State* L) {
    int count = luaL_checkinteger(L, 1);
    if (count <= 0 || count > 1024 * 1024) {
        luaL_error(L, "Invalid byte count (must be 1-1048576)");
        return 0;
    }
    
    std::vector<uint8_t> bytes(count);
    if (RAND_bytes(bytes.data(), count) != 1) {
        luaL_error(L, "Failed to generate random bytes");
        return 0;
    }
    
    lua_pushlstring(L, (const char*)bytes.data(), count);
    return 1;
}

// crypt.generatekey() - Generate a random 32-byte key (base64 encoded)
static int lua_crypt_generatekey(lua_State* L) {
    uint8_t key[32];
    if (RAND_bytes(key, 32) != 1) {
        luaL_error(L, "Failed to generate key");
        return 0;
    }
    
    char* encoded = xoron_base64_encode(key, 32);
    if (encoded) {
        lua_pushstring(L, encoded);
        free(encoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// crypt.hash(data, algorithm) - Hash data with specified algorithm
static int lua_crypt_hash(lua_State* L) {
    size_t data_len;
    const char* data = luaL_checklstring(L, 1, &data_len);
    const char* algorithm = luaL_optstring(L, 2, "sha256");
    
    const EVP_MD* md = nullptr;
    
    if (strcasecmp(algorithm, "sha256") == 0) {
        md = EVP_sha256();
    } else if (strcasecmp(algorithm, "sha384") == 0) {
        md = EVP_sha384();
    } else if (strcasecmp(algorithm, "sha512") == 0) {
        md = EVP_sha512();
    } else if (strcasecmp(algorithm, "sha1") == 0) {
        md = EVP_sha1();
    } else if (strcasecmp(algorithm, "md5") == 0) {
        md = EVP_md5();
    } else if (strcasecmp(algorithm, "sha224") == 0) {
        md = EVP_sha224();
    } else {
        luaL_error(L, "Unknown hash algorithm: %s", algorithm);
        return 0;
    }
    
    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        luaL_error(L, "Failed to create hash context");
        return 0;
    }
    
    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
        EVP_DigestUpdate(ctx, data, data_len) != 1 ||
        EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        luaL_error(L, "Hash computation failed");
        return 0;
    }
    
    EVP_MD_CTX_free(ctx);
    
    std::string hex = bytes_to_hex(hash, hash_len);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.hmac(data, key, algorithm) - HMAC
static int lua_crypt_hmac(lua_State* L) {
    size_t data_len, key_len;
    const char* data = luaL_checklstring(L, 1, &data_len);
    const char* key = luaL_checklstring(L, 2, &key_len);
    const char* algorithm = luaL_optstring(L, 3, "sha256");
    
    const EVP_MD* md = nullptr;
    
    if (strcasecmp(algorithm, "sha256") == 0) {
        md = EVP_sha256();
    } else if (strcasecmp(algorithm, "sha384") == 0) {
        md = EVP_sha384();
    } else if (strcasecmp(algorithm, "sha512") == 0) {
        md = EVP_sha512();
    } else if (strcasecmp(algorithm, "sha1") == 0) {
        md = EVP_sha1();
    } else if (strcasecmp(algorithm, "md5") == 0) {
        md = EVP_md5();
    } else {
        luaL_error(L, "Unknown HMAC algorithm: %s", algorithm);
        return 0;
    }
    
    uint8_t result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;
    
    HMAC(md, key, (int)key_len, (const unsigned char*)data, data_len, result, &result_len);
    
    std::string hex = bytes_to_hex(result, result_len);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.encrypt(data, key, iv, algorithm) - AES encryption
static int lua_crypt_encrypt(lua_State* L) {
    size_t data_len, key_len;
    const char* data = luaL_checklstring(L, 1, &data_len);
    const char* key_b64 = luaL_checklstring(L, 2, &key_len);
    const char* iv_b64 = luaL_optstring(L, 3, nullptr);
    const char* algorithm = luaL_optstring(L, 4, "aes-cbc");
    
    // Decode key from base64
    size_t key_decoded_len;
    uint8_t* key_decoded = xoron_base64_decode(key_b64, &key_decoded_len);
    if (!key_decoded || key_decoded_len < 16) {
        if (key_decoded) free(key_decoded);
        luaL_error(L, "Invalid key (must be at least 16 bytes when decoded)");
        return 0;
    }
    
    // Determine cipher
    const EVP_CIPHER* cipher = nullptr;
    int iv_len = 16;
    
    if (strcasecmp(algorithm, "aes-cbc") == 0 || strcasecmp(algorithm, "aes-256-cbc") == 0) {
        cipher = EVP_aes_256_cbc();
    } else if (strcasecmp(algorithm, "aes-128-cbc") == 0) {
        cipher = EVP_aes_128_cbc();
    } else if (strcasecmp(algorithm, "aes-gcm") == 0 || strcasecmp(algorithm, "aes-256-gcm") == 0) {
        cipher = EVP_aes_256_gcm();
        iv_len = 12;
    } else {
        free(key_decoded);
        luaL_error(L, "Unknown encryption algorithm: %s", algorithm);
        return 0;
    }
    
    // Generate or decode IV
    std::vector<uint8_t> iv(iv_len);
    if (iv_b64) {
        size_t iv_decoded_len;
        uint8_t* iv_decoded = xoron_base64_decode(iv_b64, &iv_decoded_len);
        if (iv_decoded && iv_decoded_len >= (size_t)iv_len) {
            memcpy(iv.data(), iv_decoded, iv_len);
            free(iv_decoded);
        } else {
            if (iv_decoded) free(iv_decoded);
            free(key_decoded);
            luaL_error(L, "Invalid IV");
            return 0;
        }
    } else {
        RAND_bytes(iv.data(), iv_len);
    }
    
    // Encrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(key_decoded);
        luaL_error(L, "Failed to create cipher context");
        return 0;
    }
    
    std::vector<uint8_t> ciphertext(data_len + EVP_CIPHER_block_size(cipher));
    int len = 0, ciphertext_len = 0;
    
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key_decoded, iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        luaL_error(L, "Encryption init failed");
        return 0;
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const unsigned char*)data, (int)data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        luaL_error(L, "Encryption update failed");
        return 0;
    }
    ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        luaL_error(L, "Encryption finalize failed");
        return 0;
    }
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    free(key_decoded);
    
    // Prepend IV to ciphertext and encode as base64
    std::vector<uint8_t> result(iv_len + ciphertext_len);
    memcpy(result.data(), iv.data(), iv_len);
    memcpy(result.data() + iv_len, ciphertext.data(), ciphertext_len);
    
    char* encoded = xoron_base64_encode(result.data(), result.size());
    if (encoded) {
        lua_pushstring(L, encoded);
        free(encoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// crypt.decrypt(data, key, iv, algorithm) - AES decryption
static int lua_crypt_decrypt(lua_State* L) {
    size_t data_len, key_len;
    const char* data_b64 = luaL_checklstring(L, 1, &data_len);
    const char* key_b64 = luaL_checklstring(L, 2, &key_len);
    const char* iv_b64 = luaL_optstring(L, 3, nullptr);
    const char* algorithm = luaL_optstring(L, 4, "aes-cbc");
    
    // Decode data from base64
    size_t data_decoded_len;
    uint8_t* data_decoded = xoron_base64_decode(data_b64, &data_decoded_len);
    if (!data_decoded) {
        luaL_error(L, "Invalid encrypted data");
        return 0;
    }
    
    // Decode key from base64
    size_t key_decoded_len;
    uint8_t* key_decoded = xoron_base64_decode(key_b64, &key_decoded_len);
    if (!key_decoded || key_decoded_len < 16) {
        if (key_decoded) free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Invalid key");
        return 0;
    }
    
    // Determine cipher
    const EVP_CIPHER* cipher = nullptr;
    int iv_len = 16;
    
    if (strcasecmp(algorithm, "aes-cbc") == 0 || strcasecmp(algorithm, "aes-256-cbc") == 0) {
        cipher = EVP_aes_256_cbc();
    } else if (strcasecmp(algorithm, "aes-128-cbc") == 0) {
        cipher = EVP_aes_128_cbc();
    } else if (strcasecmp(algorithm, "aes-gcm") == 0 || strcasecmp(algorithm, "aes-256-gcm") == 0) {
        cipher = EVP_aes_256_gcm();
        iv_len = 12;
    } else {
        free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Unknown decryption algorithm: %s", algorithm);
        return 0;
    }
    
    // Extract IV from data or use provided IV
    std::vector<uint8_t> iv(iv_len);
    const uint8_t* ciphertext;
    size_t ciphertext_len;
    
    if (iv_b64) {
        size_t iv_decoded_len;
        uint8_t* iv_decoded = xoron_base64_decode(iv_b64, &iv_decoded_len);
        if (iv_decoded && iv_decoded_len >= (size_t)iv_len) {
            memcpy(iv.data(), iv_decoded, iv_len);
            free(iv_decoded);
        } else {
            if (iv_decoded) free(iv_decoded);
            free(key_decoded);
            free(data_decoded);
            luaL_error(L, "Invalid IV");
            return 0;
        }
        ciphertext = data_decoded;
        ciphertext_len = data_decoded_len;
    } else {
        // IV is prepended to ciphertext
        if (data_decoded_len < (size_t)iv_len) {
            free(key_decoded);
            free(data_decoded);
            luaL_error(L, "Data too short");
            return 0;
        }
        memcpy(iv.data(), data_decoded, iv_len);
        ciphertext = data_decoded + iv_len;
        ciphertext_len = data_decoded_len - iv_len;
    }
    
    // Decrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Failed to create cipher context");
        return 0;
    }
    
    std::vector<uint8_t> plaintext(ciphertext_len + EVP_CIPHER_block_size(cipher));
    int len = 0, plaintext_len = 0;
    
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key_decoded, iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Decryption init failed");
        return 0;
    }
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, (int)ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Decryption update failed");
        return 0;
    }
    plaintext_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(key_decoded);
        free(data_decoded);
        luaL_error(L, "Decryption finalize failed (invalid key or corrupted data)");
        return 0;
    }
    plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    free(key_decoded);
    free(data_decoded);
    
    lua_pushlstring(L, (const char*)plaintext.data(), plaintext_len);
    return 1;
}

// crypt.random(min, max) - Generate random integer
static int lua_crypt_random(lua_State* L) {
    int min_val = luaL_optinteger(L, 1, 0);
    int max_val = luaL_optinteger(L, 2, INT_MAX);
    
    if (min_val > max_val) {
        int temp = min_val;
        min_val = max_val;
        max_val = temp;
    }
    
    uint32_t random_val;
    RAND_bytes((unsigned char*)&random_val, sizeof(random_val));
    
    int range = max_val - min_val + 1;
    int result = min_val + (random_val % range);
    
    lua_pushinteger(L, result);
    return 1;
}

// crypt.base64encode(data) - Base64 encode
static int lua_crypt_base64encode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    char* encoded = xoron_base64_encode(data, len);
    if (encoded) {
        lua_pushstring(L, encoded);
        free(encoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// crypt.base64decode(data) - Base64 decode
static int lua_crypt_base64decode(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    
    size_t len;
    uint8_t* decoded = xoron_base64_decode(str, &len);
    if (decoded) {
        lua_pushlstring(L, (const char*)decoded, len);
        free(decoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// crypt.sha256(data) - SHA256 hash
static int lua_crypt_sha256(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    uint8_t hash[32];
    xoron_sha256(data, len, hash);
    
    std::string hex = bytes_to_hex(hash, 32);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.sha384(data) - SHA384 hash
static int lua_crypt_sha384(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    uint8_t hash[48];
    xoron_sha384(data, len, hash);
    
    std::string hex = bytes_to_hex(hash, 48);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.sha512(data) - SHA512 hash
static int lua_crypt_sha512(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    uint8_t hash[64];
    xoron_sha512(data, len, hash);
    
    std::string hex = bytes_to_hex(hash, 64);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.md5(data) - MD5 hash
static int lua_crypt_md5(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    uint8_t hash[16];
    xoron_md5(data, len, hash);
    
    std::string hex = bytes_to_hex(hash, 16);
    lua_pushstring(L, hex.c_str());
    return 1;
}

// crypt.hexencode(data) - Hex encode
static int lua_crypt_hexencode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    char* encoded = xoron_hex_encode(data, len);
    if (encoded) {
        lua_pushstring(L, encoded);
        free(encoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// crypt.hexdecode(data) - Hex decode
static int lua_crypt_hexdecode(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    
    size_t len;
    uint8_t* decoded = xoron_hex_decode(str, &len);
    if (decoded) {
        lua_pushlstring(L, (const char*)decoded, len);
        free(decoded);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// Register crypt library (called from xoron_luau.cpp)
void xoron_register_crypt(lua_State* L) {
    lua_newtable(L);
    
    // Hash functions
    lua_pushcfunction(L, lua_crypt_sha256, "sha256");
    lua_setfield(L, -2, "sha256");
    
    lua_pushcfunction(L, lua_crypt_sha384, "sha384");
    lua_setfield(L, -2, "sha384");
    
    lua_pushcfunction(L, lua_crypt_sha512, "sha512");
    lua_setfield(L, -2, "sha512");
    
    lua_pushcfunction(L, lua_crypt_md5, "md5");
    lua_setfield(L, -2, "md5");
    
    lua_pushcfunction(L, lua_crypt_hash, "hash");
    lua_setfield(L, -2, "hash");
    
    // HMAC
    lua_pushcfunction(L, lua_crypt_hmac, "hmac");
    lua_setfield(L, -2, "hmac");
    
    // Encryption/Decryption
    lua_pushcfunction(L, lua_crypt_encrypt, "encrypt");
    lua_setfield(L, -2, "encrypt");
    
    lua_pushcfunction(L, lua_crypt_decrypt, "decrypt");
    lua_setfield(L, -2, "decrypt");
    
    // Key/Random generation
    lua_pushcfunction(L, lua_crypt_generatekey, "generatekey");
    lua_setfield(L, -2, "generatekey");
    
    lua_pushcfunction(L, lua_crypt_generatebytes, "generatebytes");
    lua_setfield(L, -2, "generatebytes");
    
    lua_pushcfunction(L, lua_crypt_random, "random");
    lua_setfield(L, -2, "random");
    
    // Encoding
    lua_pushcfunction(L, lua_crypt_base64encode, "base64encode");
    lua_setfield(L, -2, "base64encode");
    lua_pushcfunction(L, lua_crypt_base64encode, "base64_encode");
    lua_setfield(L, -2, "base64_encode");
    
    lua_pushcfunction(L, lua_crypt_base64decode, "base64decode");
    lua_setfield(L, -2, "base64decode");
    lua_pushcfunction(L, lua_crypt_base64decode, "base64_decode");
    lua_setfield(L, -2, "base64_decode");
    
    lua_pushcfunction(L, lua_crypt_hexencode, "hexencode");
    lua_setfield(L, -2, "hexencode");
    lua_pushcfunction(L, lua_crypt_hexencode, "hex_encode");
    lua_setfield(L, -2, "hex_encode");
    
    lua_pushcfunction(L, lua_crypt_hexdecode, "hexdecode");
    lua_setfield(L, -2, "hexdecode");
    lua_pushcfunction(L, lua_crypt_hexdecode, "hex_decode");
    lua_setfield(L, -2, "hex_decode");
    
    lua_setglobal(L, "crypt");
    
    // Also set as 'crypto' for compatibility
    lua_getglobal(L, "crypt");
    lua_setglobal(L, "crypto");

    // sUNC - Register base64encode/base64decode as global functions
    lua_pushcfunction(L, lua_crypt_base64encode, "base64encode");
    lua_setglobal(L, "base64encode");
    lua_pushcfunction(L, lua_crypt_base64encode, "base64_encode");
    lua_setglobal(L, "base64_encode");

    lua_pushcfunction(L, lua_crypt_base64decode, "base64decode");
    lua_setglobal(L, "base64decode");
    lua_pushcfunction(L, lua_crypt_base64decode, "base64_decode");
    lua_setglobal(L, "base64_decode");
}
