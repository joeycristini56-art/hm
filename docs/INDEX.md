# Xoron Documentation Index

## Quick Start

**New to Xoron?** Start here:
1. [Main README](../README.md) - Project overview
2. [Architecture Overview](architecture/README.md) - How it works
3. [Platform Guides](platforms/README.md) - iOS & Android integration

## Documentation Structure

```
docs/
├── README.md                 # Main documentation entry point
├── INDEX.md                  # This file - navigation guide
│
├── architecture/             # Architecture documentation
│   ├── README.md            # Architecture overview
│   ├── component_model.md   # Component relationships
│   ├── execution_flow.md    # Script execution flow
│   ├── memory_model.md      # Memory management
│   └── security_model.md    # Security architecture
│
├── api/                      # API reference
│   ├── README.md            # API overview
│   ├── core_api.md          # C/C++ native API
│   ├── lua_api.md           # Lua scripting API
│   ├── crypto_api.md        # Cryptographic functions
│   ├── http_api.md          # HTTP/WebSocket
│   ├── drawing_api.md       # Drawing engine
│   └── env_api.md           # Environment functions
│
├── platforms/                # Platform-specific guides
│   ├── README.md            # Platform comparison
│   ├── ios.md               # iOS integration
│   ├── android.md           # Android integration
│   └── cross_platform.md    # Cross-platform strategies
│
└── workflows/                # CI/CD documentation
    └── README.md            # Build and test workflows
```

```
src/
├── README.md                 # Source code documentation
├── CMakeLists.txt           # Build configuration
│
├── lua/                     # Lua utility scripts
│   └── README.md            # Script documentation
│
└── tests/                   # Test suite
    ├── README.md            # Test overview
    ├── android/             # Android tests
    │   └── README.md
    └── ios/                 # iOS tests
        └── README.md
```

## By Topic

### Understanding Xoron

**Architecture:**
- [Architecture Overview](architecture/README.md)
- [Component Model](architecture/component_model.md)
- [Execution Flow](architecture/execution_flow.md)
- [Memory Model](architecture/memory_model.md)
- [Security Model](architecture/security_model.md)

**Source Code:**
- [Source Overview](../src/README.md)
- [Core Components](../src/README.md#core-components)
- [Integration Flow](../src/README.md#integration-flow)

### Using Xoron

**C/C++ API:**
- [Core API Reference](api/core_api.md)
- [Initialization](api/core_api.md#initialization)
- [VM Management](api/core_api.md#vm-management)
- [Execution](api/core_api.md#execution)
- [HTTP](api/core_api.md#http-api)
- [Crypto](api/core_api.md#crypto-api)

**Lua API:**
- [Lua API Reference](api/lua_api.md)
- [HTTP Library](api/lua_api.md#http-library)
- [Crypto Library](api/lua_api.md#crypto-library)
- [Drawing Library](api/lua_api.md#drawing-library)
- [WebSocket Library](api/lua_api.md#websocket-library)
- [Environment Functions](api/lua_api.md#environment-functions)

**Lua Scripts:**
- [Script Documentation](../src/lua/README.md)
- [SaveInstance](../src/lua/README.md#saveinstancelua)

### Platform Integration

**iOS:**
- [iOS Guide](platforms/ios.md)
- [Prerequisites](platforms/ios.md#prerequisites)
- [Building](platforms/ios.md#building-xoron-for-ios)
- [Integration Methods](platforms/ios.md#integration-methods)
- [SwiftUI Integration](platforms/ios.md#swiftui-integration)
- [UIKit Integration](platforms/ios.md#uikit-integration)

**Android:**
- [Android Guide](platforms/android.md)
- [Prerequisites](platforms/android.md#prerequisites)
- [Building](platforms/android.md#building-xoron-for-android)
- [CMake Integration](platforms/android.md#method-1-cmake-integration)
- [Kotlin Integration](platforms/android.md#kotlin-integration)
- [Activity Integration](platforms/android.md#activity-integration)

**Cross-Platform:**
- [Cross-Platform Guide](platforms/cross_platform.md)
- [Architecture](platforms/cross_platform.md#architecture-overview)
- [Code Organization](platforms/cross_platform.md#code-organization-strategy)
- [Build System](platforms/cross_platform.md#build-system)
- [Testing](platforms/cross_platform.md#testing-strategy)

### Testing

**Test Suite:**
- [Test Overview](../src/tests/README.md)
- [Running Tests](../src/tests/README.md#running-tests)
- [Writing Tests](../src/tests/README.md#writing-new-tests)
- [Performance Benchmarks](../src/tests/README.md#performance-benchmarks)

**Platform Tests:**
- [Android Tests](../src/tests/android/README.md)
- [iOS Tests](../src/tests/ios/README.md)

### Build & Deployment

**Build System:**
- [CMake Configuration](../src/CMakeLists.txt)
- [Build Options](../src/CMakeLists.txt#build-options)
- [Platform Detection](../src/CMakeLists.txt#platform-detection)

**CI/CD:**
- [Workflows Overview](workflows/README.md)
- [Android Build](workflows/README.md#android-build)
- [iOS Build](workflows/README.md#ios-build)
- [Android Tests](workflows/README.md#android-tests)
- [iOS Tests](workflows/README.md#ios-tests)
- [Release Process](workflows/README.md#create-release)

## Getting Started Paths

### For Developers

**I want to integrate Xoron into my app:**
1. [Platform Comparison](platforms/README.md)
2. Choose your platform:
   - iOS: [iOS Integration Guide](platforms/ios.md)
   - Android: [Android Integration Guide](platforms/android.md)
3. [API Reference](api/README.md) for detailed functions

**I want to understand how Xoron works:**
1. [Architecture Overview](architecture/README.md)
2. [Component Model](architecture/component_model.md)
3. [Source Code Overview](../src/README.md)

**I want to write Lua scripts:**
1. [Lua API Reference](api/lua_api.md)
2. [Script Examples](../src/lua/README.md)
3. [Environment Functions](api/env_api.md)

### For Contributors

**I want to contribute code:**
1. [Source Code Overview](../src/README.md)
2. [Architecture](architecture/README.md)
3. [Cross-Platform Guide](platforms/cross_platform.md)
4. [Test Documentation](../src/tests/README.md)

**I want to add a new feature:**
1. [Component Model](architecture/component_model.md)
2. [Cross-Platform Guide](platforms/cross_platform.md)
3. [Test Framework](../src/tests/README.md)

**I want to improve documentation:**
1. Review existing docs
2. Follow documentation standards
3. Update INDEX.md if adding new sections

### For Maintainers

**Build & Release:**
1. [Build Configuration](../src/CMakeLists.txt)
2. [CI/CD Workflows](workflows/README.md)
3. [Platform Guides](platforms/)

**Testing:**
1. [Test Suite](../src/tests/README.md)
2. [Platform Tests](../src/tests/android/README.md) & [../src/tests/ios/README.md]
3. [Performance Benchmarks](../src/tests/README.md#performance-benchmarks)

## Key Files

### Core Documentation
- **README.md** - Project entry point
- **docs/README.md** - Documentation entry point
- **src/README.md** - Code documentation

### API References
- **docs/api/core_api.md** - C/C++ API
- **docs/api/lua_api.md** - Lua API

### Platform Guides
- **docs/platforms/ios.md** - iOS integration
- **docs/platforms/android.md** - Android integration
- **docs/platforms/cross_platform.md** - Cross-platform strategies

### Architecture
- **docs/architecture/README.md** - Overview
- **docs/architecture/component_model.md** - Components
- **docs/architecture/execution_flow.md** - Execution
- **docs/architecture/memory_model.md** - Memory
- **docs/architecture/security_model.md** - Security

### Build & Tests
- **src/CMakeLists.txt** - Build system
- **src/tests/README.md** - Test suite
- **docs/workflows/README.md** - CI/CD

## Documentation Standards

All documentation follows these principles:
1. **Clarity**: Clear, concise explanations
2. **Completeness**: Cover all public APIs
3. **Examples**: Provide practical code examples
4. **Cross-References**: Link related documentation
5. **Updates**: Keep synchronized with code

## Getting Help

**Documentation Issues:**
- Check this INDEX first
- Use search (Ctrl+F) in relevant files
- Review cross-references

**Code Issues:**
- Check [Architecture docs](architecture/)
- Review [API references](api/)
- See [Platform guides](platforms/)

**Build Issues:**
- Check [Build configuration](../src/CMakeLists.txt)
- Review [CI/CD workflows](workflows/)
- See [Platform guides](platforms/)

## Version Information

**Xoron Version**: 2.0.0  
**Documentation Version**: 2.0.0  
**Last Updated**: 2026-01-07

---

**Next Steps:**
- [Main README](../README.md) - Project overview
- [Architecture Overview](architecture/README.md) - How it works
- [Platform Guides](platforms/README.md) - Integration guides
