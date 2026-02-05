# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

**Prerequisites:** CMake 3.24+, Visual Studio 2019+ (Windows) or Xcode (Mac), Git LFS

### Configure and Build (using presets)

```bash
# Windows - Configure with Visual Studio 2022
cmake --preset windows-vs2022

# Windows - Build Editor (profile config)
cmake --build build/windows_vs2022 --target Editor --config profile

# Mac - Configure with Xcode
cmake --preset mac-default

# Mac - Build Editor
cmake --build build/mac_xcode --target Editor --config profile
```

### Manual Configuration

```bash
cmake -B <build_path> -S <source_path> -G "Visual Studio 17 2022" -DLY_3RDPARTY_PATH=<packages_path>
cmake --build <build_path> --target Editor --config profile
```

### Engine Registration

```bash
scripts/o3de.bat register --this-engine    # Windows
scripts/o3de.sh register --this-engine     # Linux/Mac
```

## Testing

### Build and Run Tests

```bash
# Build test dependencies
cmake --build <build_path> --target TEST_SUITE_smoke TEST_SUITE_main --config profile

# Run all smoke+main tests via CTest
ctest --preset windows-test-profile   # or mac-test-profile

# Run specific test by name
ctest -R "TestName" --build-config profile

# Run tests by suite label
ctest -L SUITE_smoke --build-config profile
ctest -L SUITE_main --build-config profile
```

### Python Tests (pytest)

```bash
python -m pytest -v --tb=short -c pytest.ini <test_path>

# Run specific test file
python -m pytest AutomatedTesting/Gem/PythonTests/<path>/test_file.py

# Run with specific marker
python -m pytest -m SUITE_smoke
```

**Test Suites:** smoke (fast CI-blocking), main (wider functionality), periodic (low-priority), benchmark, sandbox (flaky), awsi (AWS integration)

## Architecture Overview

### Core Frameworks (Code/Framework/)

- **AzCore:** Foundation - memory allocators, serialization, reflection, math, threading, settings registry
- **AzFramework:** Application layer - entities, assets, input, scene management
- **AzToolsFramework:** Editor tooling - prefabs, asset browser, property editor
- **AzNetworking:** Network abstraction - UDP/TCP, connection management, compression

### EBus (Event Bus System)

Primary inter-component communication mechanism in `Code/Framework/AzCore/AzCore/EBus/`.

```cpp
// Define interface
class MyInterface : public AZ::EBusTraits
{
public:
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    virtual void OnEvent() = 0;
};
using MyBus = AZ::EBus<MyInterface>;

// Broadcast
MyBus::Broadcast(&MyInterface::OnEvent);

// Handle
class Handler : public MyBus::Handler { ... };
```

Address policies: `Single` (singleton), `ById` (addressed by ID), `ByIdAndOrdered`

### Memory Allocators (Code/Framework/AzCore/AzCore/Memory/)

O3DE uses custom memory allocators for performance and tracking. Never use raw `new`/`delete` or `malloc`/`free`.

**Allocator Hierarchy:**
- **SystemAllocator** - Default general-purpose allocator (uses HPHA internally)
- **OSAllocator** - Direct OS allocations, not tracked
- **PoolAllocator** / **ThreadPoolAllocator** - Fast fixed-size allocations
- **ChildAllocator** - Memory categorization wrapper around parent allocator

**Class Allocator Declaration:**

```cpp
class MyClass
{
public:
    AZ_CLASS_ALLOCATOR(MyClass, AZ::SystemAllocator);  // Enables new/delete
    // Or with alignment: AZ_CLASS_ALLOCATOR(MyClass, AZ::SystemAllocator, 16);
};

// Split declaration (header) and implementation (.cpp):
class MyClass
{
public:
    AZ_CLASS_ALLOCATOR_DECL
};
// In .cpp:
AZ_CLASS_ALLOCATOR_IMPL(MyClass, AZ::SystemAllocator);
```

**Allocation Macros:**

```cpp
// Memory allocation (like malloc)
void* buffer = azmalloc(1024);                              // SystemAllocator
void* aligned = azmalloc(512, 32);                          // 32-byte alignment
void* custom = azmalloc(256, 16, AZ::PoolAllocator);        // Custom allocator

// Object creation (handles constructor)
MyData* data = azcreate(MyData, (arg1, arg2), AZ::SystemAllocator);

// Deallocation
azfree(buffer);
azfree(custom, AZ::PoolAllocator, 256, 16);
azdestroy(data, AZ::SystemAllocator, MyData);               // Calls destructor + frees
```

**Child Allocator for Categorization:**

```cpp
// Track allocations separately while using parent's memory
AZ_CHILD_ALLOCATOR_WITH_NAME(
    PhysicsAllocator,
    "Physics",
    "{GUID-HERE}",
    AZ::SystemAllocator
);

class PhysicsObject
{
public:
    AZ_CLASS_ALLOCATOR(PhysicsObject, PhysicsAllocator);
};
```

**STL Container Integration:**

```cpp
AZStd::vector<int, AZStdAlloc<AZ::SystemAllocator>> myVector;
```

### Gems (Gems/)

Modular plugin system. Each Gem has:
- `gem.json` - metadata, dependencies, version
- `CMakeLists.txt` - build targets using `ly_add_target()`
- Source organized by `Code/`, `Editor/`, `Assets/`

Key Gems: Atom (renderer), PhysX (physics), ScriptCanvas (visual scripting), Multiplayer, LyShine (UI)

### Platform Abstraction Layer (PAL)

Cross-platform support via:
- `cmake/Platform/{Windows,Linux,Mac,Android,iOS}/` - platform CMake configs
- Source `Platform/{PlatformName}/` directories for platform-specific implementations
- PAL traits in `cmake/PAL.cmake` control feature availability

### Settings Registry

Runtime configuration via `.setreg` JSON files in `Registry/`. Access through `AZ::SettingsRegistry`.

### Build System Patterns

```cmake
ly_add_target(
    NAME MyTarget STATIC/SHARED/MODULE/EXECUTABLE
    NAMESPACE Gem
    FILES_CMAKE my_files.cmake
    BUILD_DEPENDENCIES
        PUBLIC AZ::AzCore
        PRIVATE Gem::SomeGem
)
```

Use `ly_add_target()` wrapper, not raw CMake. Supports unity builds via `LY_UNITY_BUILD`.

## Code Style

- 4-space indentation, 140-char line limit
- Allman brace style (braces on new lines)
- Pointer alignment left (`int* ptr`)
- See `.clang-format` for full rules

## Commits

Requires DCO sign-off: `git commit -s -m "message"`

## Key Directories

- `Code/Editor/` - O3DE Editor application
- `Code/Tools/` - AssetProcessor, ProjectManager, AzTestRunner
- `Code/LauncherUnified/` - Game launcher
- `AutomatedTesting/` - Test project with Python test suites
- `Templates/` - Project and Gem templates
- `Registry/` - Default .setreg configuration files
