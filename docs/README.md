# Xoron Documentation

## Overview

This directory contains comprehensive documentation for the Xoron Executor Engine, covering architecture, APIs, platform-specific implementations, and integration guides.

## Documentation Structure

```
docs/
├── README.md              # This file - navigation guide
├── api/                   # API reference documentation
│   ├── README.md         # API overview
│   ├── core_api.md       # Core C API
│   ├── lua_api.md        # Lua API reference
│   ├── crypto_api.md     # Cryptographic functions
│   ├── http_api.md       # HTTP/WebSocket APIs
│   ├── drawing_api.md    # Drawing engine API
│   └── env_api.md        # Environment functions
├── architecture/          # Architecture documentation
│   ├── README.md         # Architecture overview
│   ├── component_model.md # Component relationships
│   ├── execution_flow.md  # Script execution flow
│   ├── memory_model.md    # Memory management
│   └── security_model.md  # Security architecture
└── platforms/             # Platform-specific docs
    ├── README.md         # Platform overview
    ├── ios.md            # iOS implementation details
    ├── android.md        # Android implementation details
    └── cross_platform.md # Cross-platform considerations
```

## Quick Navigation

### For Developers

**Understanding the Codebase:**
1. Start with [Architecture Overview](architecture/README.md)
2. Read [Component Model](architecture/component_model.md)
3. Review [Execution Flow](architecture/execution_flow.md)

**API Reference:**
1. [Core C API](api/core_api.md) - Native API
2. [Lua API](api/lua_api.md) - Script-level API
3. [Platform APIs](platforms/) - Platform-specific features

### For Integrators

**Platform Integration:**
1. [iOS Integration](platforms/ios.md)
2. [Android Integration](platforms/android.md)
3. [Cross-Platform Guide](platforms/cross_platform.md)

**Build System:**
- See `src/CMakeLists.txt` for build configuration
- See `src/workflows/` for CI/CD examples

### For Script Writers

**Lua API Usage:**
1. [Lua API Reference](api/lua_api.md)
2. [Environment Functions](api/env_api.md)
3. [Examples](../src/tests/common/)

## Key Topics

### Architecture
- **Component Model**: How components interact and depend on each other
- **Execution Flow**: From script compilation to runtime execution
- **Memory Model**: Memory management and safety
- **Security Model**: Anti-detection and security features

### APIs
- **Core API**: C/C++ native interface
- **Lua API**: Script-level interface
- **HTTP/WebSocket**: Network operations
- **Crypto**: Cryptographic functions
- **Drawing**: 2D rendering
- **Environment**: Lua environment management

### Platforms
- **iOS**: Objective-C++, CoreGraphics, UIKit
- **Android**: JNI, Android NDK, native canvas
- **Cross-Platform**: Abstraction layers and compatibility

## Getting Started

### New to Xoron?

1. **Read the main README**: `../README.md` for overview
2. **Architecture**: `architecture/README.md` for design principles
3. **API Reference**: `api/lua_api.md` for script usage

### Contributing

1. **Code Structure**: `../src/README.md` for source organization
2. **Architecture**: `architecture/component_model.md` for dependencies
3. **Testing**: `../src/tests/README.md` for test guidelines

### Integration

1. **Platform Guides**: `platforms/ios.md` or `platforms/android.md`
2. **Build System**: `../src/CMakeLists.txt`
3. **Workflows**: `../src/workflows/README.md`

## Documentation Files

### Architecture Documentation

**[architecture/README.md](architecture/README.md)**
- High-level architecture overview
- Design patterns and principles
- System boundaries and layers

**[architecture/component_model.md](architecture/component_model.md)**
- Component relationships and dependencies
- Data flow between components
- Integration points

**[architecture/execution_flow.md](architecture/execution_flow.md)**
- Script compilation process
- Runtime execution flow
- Error handling paths

**[architecture/memory_model.md](architecture/memory_model.md)**
- Memory allocation strategies
- RAII patterns
- Resource lifecycle

**[architecture/security_model.md](architecture/security_model.md)**
- Security features and mechanisms
- Anti-detection strategies
- Best practices

### API Documentation

**[api/README.md](api/README.md)**
- API overview and organization
- Quick reference
- Usage patterns

**[api/core_api.md](api/core_api.md)**
- C/C++ API functions
- Type definitions
- Error codes
- Platform-specific notes

**[api/lua_api.md](api/lua_api.md)**
- Lua library functions
- Usage examples
- Common patterns
- Integration examples

**[api/crypto_api.md](api/crypto_api.md)**
- Hash functions (SHA256, SHA384, SHA512, MD5)
- Encoding (Base64, Hex)
- Encryption (AES)
- HMAC

**[api/http_api.md](api/http_api.md)**
- HTTP GET/POST
- WebSocket connections
- Callbacks and events
- Error handling

**[api/drawing_api.md](api/drawing_api.md)**
- Shape primitives
- Text rendering
- Color management
- Layer management

**[api/env_api.md](api/env_api.md)**
- Environment functions (getgenv, etc.)
- Function hooking
- Signal connections
- Utility functions

### Platform Documentation

**[platforms/README.md](platforms/README.md)**
- Platform comparison
- Feature availability
- Portability considerations

**[platforms/ios.md](platforms/ios.md)**
- iOS-specific implementation
- Objective-C++ integration
- Framework dependencies
- Build requirements

**[platforms/android.md](platforms/android.md)**
- Android-specific implementation
- JNI bridge
- NDK requirements
- Build configuration

**[platforms/cross_platform.md](platforms/cross_platform.md)**
- Abstraction strategies
- Conditional compilation
- Platform detection
- Testing across platforms

## Related Documentation

### Source Code
- `../src/README.md` - Source code organization
- `../src/CMakeLists.txt` - Build configuration
- `../src/xoron.h` - Main header with all declarations

### Tests
- `../src/tests/README.md` - Testing framework
- `../src/tests/common/test_utils.h` - Test utilities

### Workflows
- `../src/workflows/README.md` - CI/CD documentation
- `../src/workflows/build-android.yml` - Android build
- `../src/workflows/build-ios.yml` - iOS build

### Lua Scripts
- `../src/lua/README.md` - Lua utilities
- `../src/lua/saveinstance.lua` - SaveInstance documentation

## Documentation Standards

All documentation follows these principles:

1. **Clarity**: Clear, concise explanations
2. **Completeness**: Cover all public APIs
3. **Examples**: Provide practical code examples
4. **Cross-References**: Link related documentation
5. **Updates**: Keep synchronized with code changes

## Contributing to Documentation

When updating code, update documentation:
1. Update API documentation for new functions
2. Update architecture docs for structural changes
3. Add examples for new features
4. Update cross-references
5. Review for accuracy and clarity

## Getting Help

- **Issues**: Check existing documentation first
- **Questions**: Review architecture and API docs
- **Examples**: See `../src/tests/` for working examples
- **Community**: Check project issues and discussions

## Version

**Xoron Version**: 2.0.0  
**Documentation Version**: 2.0.0  
**Last Updated**: 2026-01-07

---

**Next Steps:**
- [Architecture Overview](architecture/README.md)
- [API Reference](api/README.md)
- [Platform Guides](platforms/README.md)
