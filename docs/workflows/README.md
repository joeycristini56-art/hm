# CI/CD Workflows

## Overview

Xoron uses GitHub Actions for continuous integration and deployment. The workflows automate building, testing, and releasing for both iOS and Android platforms.

## Workflow Structure

```
src/workflows/
├── build-android.yml      # Android library build
├── build-ios.yml          # iOS library build
├── test-android.yml       # Android integration tests
└── test-ios.yml           # iOS integration tests
```

## Build Workflows

### Android Build (build-android.yml)

**Purpose**: Builds libxoron.so for Android

**Triggers**:
- Push to `main` or `develop` branches
- Pull requests to `main`
- Manual workflow dispatch

**Configuration**:
```yaml
name: Build Android

on:
  push:
    branches: [ main, develop ]
    paths:
      - 'src/**'
      - '.github/workflows/build-android.yml'
  pull_request:
    branches: [ main ]
    paths:
      - 'src/**'
  workflow_dispatch:
    inputs:
      build_type:
        description: 'Build type (Debug/Release)'
        required: true
        default: 'Release'
        type: choice
        options:
          - Debug
          - Release
```

**Environment Variables**:
```yaml
env:
  XORON_VERSION: "2.0.0"
  ANDROID_MIN_API: "29"  # Android 10
  NDK_VERSION: "26.1.10909125"
  ABI: arm64-v8a
  OPENSSL_TARGET: android-arm64
```

**Build Steps**:

1. **Checkout Repository**
```yaml
- name: Checkout repository
  uses: actions/checkout@v4
  with:
    submodules: recursive
```

2. **Setup Android NDK**
```yaml
- name: Setup Android NDK
  uses: android-actions/setup-android@v3
  with:
    packages: 'ndk;${{ env.NDK_VERSION }}'
```

3. **Cache OpenSSL**
```yaml
- name: Cache OpenSSL
  id: cache-openssl
  uses: actions/cache@v4
  with:
    path: ${{ github.workspace }}/openssl-android-${{ env.ABI }}
    key: openssl-3.2.0-android-${{ env.ABI }}-api${{ env.ANDROID_MIN_API }}-v2
```

4. **Build OpenSSL** (if not cached)
```yaml
- name: Download and Build OpenSSL for Android
  if: steps.cache-openssl.outputs.cache-hit != 'true'
  run: |
    OPENSSL_VERSION="3.2.0"
    INSTALL_DIR="${{ github.workspace }}/openssl-android-${{ env.ABI }}/install"
    mkdir -p "$INSTALL_DIR"
    
    cd /tmp
    curl -LO https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    tar xzf openssl-${OPENSSL_VERSION}.tar.gz
    cd openssl-${OPENSSL_VERSION}
    
    export ANDROID_NDK_ROOT=${{ env.NDK_PATH }}
    export PATH=$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
    
    ./Configure ${{ env.OPENSSL_TARGET }} \
      -D__ANDROID_API__=${{ env.ANDROID_MIN_API }} \
      --prefix="$INSTALL_DIR" \
      --openssldir="$INSTALL_DIR/ssl" \
      no-shared \
      no-tests \
      no-ui-console \
      no-engine \
      no-comp \
      no-dso
    
    make -j$(nproc)
    make install_sw
```

5. **Configure CMake**
```yaml
- name: Configure CMake for Android
  run: |
    cmake -B build -S src \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=${{ env.NDK_PATH }}/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=${{ env.ABI }} \
      -DANDROID_PLATFORM=android-${{ env.ANDROID_MIN_API }} \
      -DANDROID_STL=c++_shared \
      -DXORON_ANDROID_BUILD=ON \
      -DXORON_ANDROID_ABI=${{ env.ABI }} \
      -DXORON_ANDROID_PLATFORM=android-${{ env.ANDROID_MIN_API }} \
      -DXORON_OPENSSL_ROOT=${{ github.workspace }}/openssl-android-${{ env.ABI }}/install
```

6. **Build**
```yaml
- name: Build
  run: |
    cmake --build build --config Release -j$(nproc)
```

7. **Verify Build**
```yaml
- name: Verify build output
  run: |
    SO_PATH=$(find build -name "libxoron.so" -type f | head -1)
    
    if [ -z "$SO_PATH" ]; then
      echo "ERROR: libxoron.so not found!"
      exit 1
    fi
    
    echo "=== Library Info ==="
    file "$SO_PATH"
    ls -lh "$SO_PATH"
    readelf -h "$SO_PATH" | head -20
    readelf -d "$SO_PATH" | grep NEEDED
    nm -D "$SO_PATH" | grep " T " | head -20
```

8. **Create Artifact**
```yaml
- name: Create artifact directory
  run: |
    mkdir -p artifacts/android-${{ env.ABI }}
    cp build/libxoron.so artifacts/android-${{ env.ABI }}/
    
    cat > artifacts/android-${{ env.ABI }}/BUILD_INFO.txt << EOF
    Xoron Android Build Information
    ===============================
    Version: ${{ env.XORON_VERSION }}
    Build Type: Release
    ABI: ${{ env.ABI }}
    Minimum Android API: ${{ env.ANDROID_MIN_API }} (Android 10)
    NDK Version: ${{ env.NDK_VERSION }}
    Build Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
    Commit: ${{ github.sha }}
    Runner: ${{ runner.os }}
    EOF

- name: Upload Android artifact
  uses: actions/upload-artifact@v4
  with:
    name: xoron-android-${{ env.ABI }}
    path: artifacts/android-${{ env.ABI }}/
    retention-days: 30
```

**Outputs**:
- `libxoron.so` (shared library)
- `BUILD_INFO.txt` (build metadata)
- Artifacts uploaded to GitHub

**Build Variants**:
- **arm64-v8a**: 64-bit ARM (most common)
- **armeabi-v7a**: 32-bit ARM (legacy)
- **x86_64**: 64-bit x86 (emulator)
- **x86**: 32-bit x86 (legacy emulator)

---

### iOS Build (build-ios.yml)

**Purpose**: Builds libxoron.dylib for iOS

**Triggers**:
- Push to `main` or `develop` branches
- Pull requests to `main`
- Manual workflow dispatch

**Configuration**:
```yaml
name: Build iOS

on:
  push:
    branches: [ main, develop ]
    paths:
      - 'src/**'
      - '.github/workflows/build-ios.yml'
  pull_request:
    branches: [ main ]
    paths:
      - 'src/**'
  workflow_dispatch:
    inputs:
      build_type:
        description: 'Build type (Debug/Release)'
        required: true
        default: 'Release'
        type: choice
        options:
          - Debug
          - Release
```

**Environment Variables**:
```yaml
env:
  XORON_VERSION: "2.0.0"
  IOS_DEPLOYMENT_TARGET: "15.0"
  OPENSSL_TARGET: "ios64-xcrun"
```

**Build Steps**:

1. **Setup Xcode**
```yaml
- name: Setup Xcode
  uses: maxim-lobanov/setup-xcode@v1
  with:
    xcode-version: '14.3'
```

2. **Cache OpenSSL**
```yaml
- name: Cache OpenSSL
  id: cache-openssl
  uses: actions/cache@v4
  with:
    path: ${{ github.workspace }}/openssl-ios
    key: openssl-3.2.0-ios-v2
```

3. **Build OpenSSL**
```yaml
- name: Build OpenSSL for iOS
  if: steps.cache-openssl.outputs.cache-hit != 'true'
  run: |
    OPENSSL_VERSION="3.2.0"
    INSTALL_DIR="${{ github.workspace }}/openssl-ios"
    mkdir -p "$INSTALL_DIR"
    
    cd /tmp
    curl -LO https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    tar xzf openssl-${OPENSSL_VERSION}.tar.gz
    cd openssl-${OPENSSL_VERSION}
    
    ./Configure ios64-xcrun \
      --prefix="$INSTALL_DIR" \
      no-shared \
      no-tests \
      no-ui-console \
      no-engine
    
    make -j$(nproc)
    make install_sw
```

4. **Configure CMake**
```yaml
- name: Configure CMake for iOS
  run: |
    cmake -B build -S src \
      -G Xcode \
      -DCMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/ios-cmake/ios.toolchain.cmake \
      -DPLATFORM=OS64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${{ env.IOS_DEPLOYMENT_TARGET }} \
      -DXORON_IOS_BUILD=ON \
      -DXORON_IOS_DEPLOYMENT_TARGET=${{ env.IOS_DEPLOYMENT_TARGET }} \
      -DXORON_OPENSSL_ROOT=${{ github.workspace }}/openssl-ios
```

5. **Build**
```yaml
- name: Build
  run: |
    cmake --build build --config Release
```

6. **Verify and Create Artifact**
```yaml
- name: Create artifact directory
  run: |
    mkdir -p artifacts/ios-arm64
    cp build/libxoron.dylib artifacts/ios-arm64/
    
    # Verify architecture
    lipo -info artifacts/ios-arm64/libxoron.dylib
    
    cat > artifacts/ios-arm64/BUILD_INFO.txt << EOF
    Xoron iOS Build Information
    ===========================
    Version: ${{ env.XORON_VERSION }}
    Build Type: Release
    Architecture: arm64
    iOS Deployment Target: ${{ env.IOS_DEPLOYMENT_TARGET }}
    Build Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
    Commit: ${{ github.sha }}
    Runner: ${{ runner.os }}
    EOF

- name: Upload iOS artifact
  uses: actions/upload-artifact@v4
  with:
    name: xoron-ios-arm64
    path: artifacts/ios-arm64/
    retention-days: 30
```

**Outputs**:
- `libxoron.dylib` (shared library)
- `BUILD_INFO.txt` (build metadata)
- Artifacts uploaded to GitHub

**Build Variants**:
- **arm64**: Physical devices
- **arm64e**: Newer devices (A12+)
- **x86_64**: Simulator (for testing)

---

## Test Workflows

### Android Tests (test-android.yml)

**Purpose**: Run integration tests on Android

**Triggers**:
- After successful build
- Manual trigger
- Scheduled (nightly)

**Configuration**:
```yaml
name: Test Android

on:
  workflow_run:
    workflows: ["Build Android"]
    types:
      - completed
  workflow_dispatch:
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM UTC
```

**Test Steps**:

1. **Download Artifacts**
```yaml
- name: Download Android artifact
  uses: actions/download-artifact@v4
  with:
    name: xoron-android-arm64-v8a
    path: artifacts/
```

2. **Setup Emulator**
```yaml
- name: Setup Android Emulator
  uses: reactivecircus/android-emulator-runner@v2
  with:
    api-level: 29
    arch: arm64-v8a
    profile: pixel_4
    ram-size: 4096
    disk-size: 8192
    script: |
      # Wait for emulator
      adb wait-for-device
      
      # Install test app
      cd src/tests/android
      ./gradlew installDebug
      adb shell am start -n com.xoron.test/.TestRunner
      
      # Wait for tests
      sleep 30
      
      # Capture results
      adb logcat -d | grep XoronTest > test_results.txt
      
      # Check for failures
      if grep -q "Failed: 0" test_results.txt; then
        echo "All tests passed!"
        exit 0
      else
        echo "Some tests failed!"
        cat test_results.txt
        exit 1
      fi
```

3. **Upload Results**
```yaml
- name: Upload test results
  uses: actions/upload-artifact@v4
  if: always()
  with:
    name: android-test-results
    path: test_results.txt
```

**Test Coverage**:
- VM operations
- HTTP/HTTPS
- Crypto functions
- File operations
- Platform features
- Performance benchmarks

---

### iOS Tests (test-ios.yml)

**Purpose**: Run integration tests on iOS

**Triggers**:
- After successful build
- Manual trigger
- Scheduled (nightly)

**Configuration**:
```yaml
name: Test iOS

on:
  workflow_run:
    workflows: ["Build iOS"]
    types:
      - completed
  workflow_dispatch:
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM UTC
```

**Test Steps**:

1. **Download Artifacts**
```yaml
- name: Download iOS artifact
  uses: actions/download-artifact@v4
  with:
    name: xoron-ios-arm64
    path: artifacts/
```

2. **Build Test App**
```yaml
- name: Build Test App
  run: |
    cd src/tests/ios
    mkdir build && cd build
    cmake .. \
      -DCMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/ios-cmake/ios.toolchain.cmake \
      -DPLATFORM=SIMULATOR64 \
      -DXORON_LIBRARY=${{ github.workspace }}/artifacts/libxoron.dylib
    cmake --build . --config Release
```

3. **Run in Simulator**
```yaml
- name: Run Tests in Simulator
  run: |
    # Boot simulator
    SIMULATOR_ID=$(xcrun simctl create iPhone-Test "iPhone 14" "iOS16.4")
    xcrun simctl boot $SIMULATOR_ID
    
    # Install and run
    cd src/tests/ios/build
    xcrun simctl install $SIMULATOR_ID Release-iphonesimulator/xoron_test_ios.app
    xcrun simctl launch $SIMULATOR_ID com.xoron.test
    
    # Wait and capture logs
    sleep 20
    xcrun simctl spawn $SIMULATOR_ID log stream \
      --predicate 'subsystem == "com.xoron.test"' > test_results.log &
    LOG_PID=$!
    
    sleep 10
    kill $LOG_PID
    
    # Check results
    grep "Passed:" test_results.log
    
    # Cleanup
    xcrun simctl shutdown $SIMULATOR_ID
    xcrun simctl delete $SIMULATOR_ID
```

4. **Upload Results**
```yaml
- name: Upload test results
  uses: actions/upload-artifact@v4
  if: always()
  with:
    name: ios-test-results
    path: test_results.log
```

---

## Release Workflow

### Create Release (release.yml)

**Purpose**: Create GitHub release with compiled libraries

**Triggers**:
- Tag push (e.g., `v2.0.0`)

**Configuration**:
```yaml
name: Create Release

on:
  push:
    tags:
      - 'v*'
```

**Steps**:

1. **Download All Artifacts**
```yaml
- name: Download Android artifacts
  uses: actions/download-artifact@v4
  with:
    name: xoron-android-arm64-v8a
    path: release/android/

- name: Download iOS artifacts
  uses: actions/download-artifact@v4
  with:
    name: xoron-ios-arm64
    path: release/ios/
```

2. **Create Release Package**
```yaml
- name: Create Release Package
  run: |
    cd release
    zip -r xoron-${{ github.ref_name }}.zip android/ ios/
    
    # Create checksums
    sha256sum xoron-${{ github.ref_name }}.zip > checksums.txt
```

3. **Create GitHub Release**
```yaml
- name: Create GitHub Release
  uses: actions/create-release@v1
  with:
    tag_name: ${{ github.ref }}
    release_name: Xoron ${{ github.ref }}
    body: |
      ## Xoron ${{ github.ref }}
      
      ### Included
      - Android: libxoron.so (arm64-v8a)
      - iOS: libxoron.dylib (arm64)
      
      ### Build Info
      - Version: ${{ env.XORON_VERSION }}
      - Commit: ${{ github.sha }}
      
      ### Installation
      See documentation in docs/platforms/
    draft: false
    prerelease: false
```

4. **Upload Assets**
```yaml
- name: Upload Release Asset
  uses: actions/upload-release-asset@v1
  with:
    upload_url: ${{ steps.create_release.outputs.upload_url }}
    asset_path: release/xoron-${{ github.ref_name }}.zip
    asset_name: xoron-${{ github.ref_name }}.zip
    asset_content_type: application/zip
```

---

## Workflow Configuration

### Common Patterns

#### 1. Conditional Execution

```yaml
# Only run on specific paths
on:
  push:
    paths:
      - 'src/**'
      - 'CMakeLists.txt'
      - '.github/workflows/*.yml'
```

#### 2. Manual Triggers with Inputs

```yaml
on:
  workflow_dispatch:
    inputs:
      build_type:
        description: 'Build type'
        required: true
        default: 'Release'
        type: choice
        options:
          - Debug
          - Release
      abi:
        description: 'Android ABI'
        required: true
        default: 'arm64-v8a'
        type: choice
        options:
          - arm64-v8a
          - armeabi-v7a
          - x86_64
          - x86
```

#### 3. Caching Strategy

```yaml
- name: Cache dependencies
  uses: actions/cache@v4
  with:
    path: |
      ~/.cache/ccache
      build/
    key: ${{ runner.os }}-cmake-${{ hashFiles('CMakeLists.txt') }}
    restore-keys: |
      ${{ runner.os }}-cmake-
```

#### 4. Matrix Builds

```yaml
strategy:
  matrix:
    abi: [arm64-v8a, armeabi-v7a, x86_64, x86]
    build_type: [Debug, Release]
    
steps:
  - name: Build ${{ matrix.abi }} ${{ matrix.build_type }}
    run: |
      cmake -B build -S src \
        -DANDROID_ABI=${{ matrix.abi }} \
        -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
      cmake --build build
```

---

## Environment Setup

### Required Secrets

Add these to GitHub repository settings:

1. **N/A** - All dependencies are public

### Required Variables

Repository variables:
- `XORON_VERSION`: Current version (e.g., "2.0.0")

---

## Troubleshooting

### Common Issues

#### 1. OpenSSL Build Fails

**Error**: `Configure: command not found`

**Solution**: Ensure OpenSSL version is compatible with platform

#### 2. Android Emulator Timeout

**Error**: `Emulator did not start within 600 seconds`

**Solution**: Increase timeout, use hardware acceleration

```yaml
- name: Setup Emulator
  with:
    script: |
      adb wait-for-device
      sleep 10
      # ... rest of script
```

#### 3. iOS Simulator Not Available

**Error**: `Simulator unavailable`

**Solution**: Specify exact simulator version

```bash
xcrun simctl create iPhone-Test "iPhone 14" "iOS16.4"
```

#### 4. Cache Miss

**Error**: `Cache not found`

**Solution**: Check cache key, ensure dependencies haven't changed

---

## Performance Optimization

### Build Time

1. **Use ccache**
```yaml
- name: Setup ccache
  run: |
    sudo apt-get install ccache
    echo "/usr/lib/ccache" >> $GITHUB_PATH
```

2. **Parallel Builds**
```bash
cmake --build build -j$(nproc)
```

3. **Incremental Builds**
```yaml
- name: Cache build artifacts
  uses: actions/cache@v4
  with:
    path: build/
    key: build-${{ runner.os }}-${{ hashFiles('src/**') }}
```

### Test Time

1. **Parallel Test Execution**
```yaml
strategy:
  matrix:
    test-group: [core, network, crypto, drawing]
```

2. **Selective Testing**
```yaml
# Only run tests for changed files
paths:
  - 'src/xoron_http.cpp'
  - 'src/tests/**'
```

---

## Monitoring

### Build Status

- **Badge**: Add to README.md
```markdown
![Build Status](https://github.com/username/xoron/actions/workflows/build-android.yml/badge.svg)
```

### Test Results

- **Artifacts**: Download test results
- **Logs**: Check workflow run logs
- **Notifications**: Configure GitHub notifications

---

## Best Practices

1. **Version Pinning**: Pin action versions (`@v4` not `@main`)
2. **Cache Dependencies**: Always cache external dependencies
3. **Fail Fast**: Use `continue-on-error: false` (default)
4. **Artifact Retention**: Set appropriate retention (30 days)
5. **Security**: Use minimal permissions, avoid secrets in logs
6. **Documentation**: Keep workflow files well-commented
7. **Testing**: Test workflows on feature branches before merging

---

## Advanced Workflows

### 1. Nightly Builds

```yaml
name: Nightly Build

on:
  schedule:
    - cron: '0 3 * * *'  # 3 AM UTC

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          ref: develop
      
      - name: Build
        run: |
          # Build with nightly version
          cmake -DXORON_VERSION="2.0.0-nightly-$(date +%Y%m%d)" ...
```

### 2. PR Validation

```yaml
name: PR Validation

on:
  pull_request:
    branches: [main]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Check PR size
        run: |
          LINES=$(git diff --stat origin/main | tail -1 | awk '{print $4}')
          if [ "$LINES" -gt 1000 ]; then
            echo "PR too large!"
            exit 1
          fi
      
      - name: Run clang-format
        run: |
          find src -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### 3. Documentation Deployment

```yaml
name: Deploy Documentation

on:
  push:
    branches: [main]
    paths:
      - 'docs/**'

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Build docs
        run: |
          # Build documentation site
          cd docs
          # Use your documentation generator
          
      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./docs/_site
```

---

## Summary

The CI/CD workflows provide:
- **Automated builds** for iOS and Android
- **Integration testing** on emulators/simulators
- **Artifact management** for easy distribution
- **Release automation** with GitHub releases
- **Scheduled builds** for nightly validation

**Key Benefits**:
- Consistent builds across platforms
- Automated testing
- Version tracking
- Easy deployment
- Quality assurance

---

**Next:**
- [Platform Documentation](../platforms/README.md)
- [Build Configuration](../src/CMakeLists.txt)
- [Test Documentation](../src/tests/README.md)
