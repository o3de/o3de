# ALLOCATION.md

Analysis of memory allocator usage in the O3DE (Open 3D Engine) codebase.

## Overview

O3DE uses a custom memory allocation system. Raw `new`/`delete` and `malloc`/`free` should not be used directly. All allocations flow through the `AZ::IAllocator` interface with tracking, profiling, and debugging support.

Core files: `Code/Framework/AzCore/AzCore/Memory/`

## Allocator Hierarchy

```
IAllocator (interface)
└── AllocatorBase (adds profiling/tracking)
    ├── SystemAllocator      - Default general-purpose (uses HphaSchema internally)
    ├── OSAllocator          - Direct OS allocations, untracked
    ├── PoolAllocator        - Fixed-size pool, non-thread-safe
    ├── ThreadPoolAllocator  - Fixed-size pool, thread-local pools
    └── SimpleSchemaAllocator<T> - Generic wrapper for custom schemas

ChildAllocatorSchema<Parent> - Pass-through for memory categorization
```

## Allocator Types

### SystemAllocator

Default allocator for most allocations. Internally uses **HPHA** (High Performance Heap Allocator), based on Dimitar Lazarov's algorithm.

```cpp
// Implicitly used by default
void* p = azmalloc(1024);
azfree(p);
```

### OSAllocator

Direct OS heap allocations (`malloc`/`free`). Not tracked by the allocator manager. Used for debug infrastructure and bootstrap allocations.

### PoolAllocator / ThreadPoolAllocator

Optimized for many small, fixed-size allocations. `ThreadPoolAllocator` maintains per-thread pools to avoid lock contention.

```cpp
// Good for frequently allocated small objects
class SmallObject
{
    AZ_CLASS_ALLOCATOR(SmallObject, AZ::PoolAllocator);
};
```

### ChildAllocatorSchema

Creates a named allocator that delegates to a parent but tracks allocations separately. Useful for per-subsystem memory accounting.

```cpp
AZ_CHILD_ALLOCATOR_WITH_NAME(
    PhysicsAllocator,
    "PhysicsMemory",
    "{GUID}",
    AZ::SystemAllocator
);

class RigidBody
{
    AZ_CLASS_ALLOCATOR(RigidBody, PhysicsAllocator);
};
// Allocations tracked under "PhysicsMemory" but use SystemAllocator's heap
```

## Arena/Linear Allocators

O3DE includes arena-style allocators where individual frees are disabled:

### RHI LinearAllocator

`Gems/Atom/RHI/Code/Include/Atom/RHI/LinearAllocator.h`

Used for per-frame GPU resource allocations in the Atom renderer.

```cpp
class LinearAllocator final : public Allocator
{
    VirtualAddress Allocate(size_t byteCount, size_t byteAlignment) override;
    void DeAllocate(VirtualAddress offset) override;  // NO-OP
    void GarbageCollect() override;       // Reset after N cycles
    void GarbageCollectForce() override;  // Immediate reset
};
```

- `Allocate()` bumps a cursor forward
- `DeAllocate()` does nothing (individual frees ignored)
- `GarbageCollect()` resets cursor after `m_garbageCollectLatency` cycles
- `GarbageCollectForce()` immediately resets cursor to 0

Supports deferred reclamation for GPU resources still in-flight.

### RapidjsonStackAllocator

`Code/Framework/AzCore/AzCore/JSON/RapidjsonAllocatorAdapter.h`

Fixed-size stack buffer for temporary JSON parsing.

```cpp
template<size_t SizeN, size_t AlignN = alignof(AZStd::byte)>
class RapidjsonStackAllocator
{
    static constexpr bool kNeedFree = false;
    void* Malloc(size_t size);        // Bump cursor
    void* Realloc(...);               // Extend or copy
    static void Free(void*) { }       // No-op
};
```

## Class Allocator Declaration

Classes must declare their allocator to use O3DE's memory system:

```cpp
class MyClass
{
public:
    AZ_CLASS_ALLOCATOR(MyClass, AZ::SystemAllocator);
    // Optional alignment: AZ_CLASS_ALLOCATOR(MyClass, AZ::SystemAllocator, 16);
};
```

This macro generates:
- `operator new` / `operator delete` using the specified allocator
- `AZ_CLASS_ALLOCATOR_Allocate()` / `AZ_CLASS_ALLOCATOR_DeAllocate()` static helpers
- Disabled array `new[]`/`delete[]` (asserts if called)

### Split Declaration/Implementation

For header/source separation:

```cpp
// Header
class MyClass
{
public:
    AZ_CLASS_ALLOCATOR_DECL
};

// Source (.cpp)
AZ_CLASS_ALLOCATOR_IMPL(MyClass, AZ::SystemAllocator);
```

## Allocation Macros

### Memory Allocation

```cpp
// Basic allocation (SystemAllocator)
void* p = azmalloc(size);
void* p = azmalloc(size, alignment);
void* p = azmalloc(size, alignment, AllocatorType);

// Zero-initialized
void* p = azcalloc(size);
void* p = azcalloc(size, alignment);
void* p = azcalloc(size, alignment, AllocatorType);

// Reallocation
void* p = azrealloc(ptr, newSize);
void* p = azrealloc(ptr, newSize, alignment);
void* p = azrealloc(ptr, newSize, alignment, AllocatorType);

// Deallocation
azfree(ptr);
azfree(ptr, AllocatorType);
azfree(ptr, AllocatorType, size, alignment);  // Full info for debugging
```

### Object Creation/Destruction

```cpp
// Create object (calls constructor)
MyClass* obj = azcreate(MyClass, (ctorArg1, ctorArg2), AZ::SystemAllocator);

// Destroy object (calls destructor + frees)
azdestroy(obj, AZ::SystemAllocator, MyClass);

// Shorthand for SystemAllocator
MyClass* obj = azcreate(MyClass, (args));
azdestroy(obj);
```

### Query Allocation Size

```cpp
size_t size = azallocsize(ptr, AllocatorType);
```

## STL Container Integration

Use `AZStdAlloc` wrapper with AZStd containers:

```cpp
// Compile-time allocator binding
AZStd::vector<int, AZStdAlloc<AZ::SystemAllocator>> vec;
AZStd::list<Entity, AZStdAlloc<AZ::PoolAllocator>> entities;

// Runtime allocator binding
AZStd::vector<int, AZStdIAllocator> vec(&myAllocatorInstance);

// Functor-based (deferred allocator lookup)
AZStd::vector<int, AZStdFunctorAllocator> vec(&GetMyAllocator);
```

## Allocator Manager

Singleton managing all registered allocators:

```cpp
AZ::AllocatorManager& mgr = AZ::AllocatorManager::Instance();

// Iterate allocators
for (int i = 0; i < mgr.GetNumAllocators(); ++i)
{
    AZ::IAllocator* alloc = mgr.GetAllocator(i);
    size_t used = alloc->NumAllocatedBytes();
}

// Force garbage collection on all allocators
mgr.GarbageCollect();

// Dump statistics
mgr.DumpAllocators();

// Out-of-memory callback
mgr.AddOutOfMemoryListener([](IAllocator* alloc, size_t size, size_t align) {
    // Handle OOM
});
```

## Allocator Instance Access

Get singleton instance of any allocator:

```cpp
AZ::IAllocator& alloc = AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
alloc.allocate(1024, 16);
```

## Debug Configuration

```cpp
struct AllocatorDebugConfig
{
    AllocatorDebugConfig& StackRecordLevels(int levels);      // Callstack capture depth
    AllocatorDebugConfig& ExcludeFromDebugging(bool exclude); // Skip tracking
    AllocatorDebugConfig& UsesMemoryGuards(bool use);         // Buffer overrun detection
    AllocatorDebugConfig& MarksUnallocatedMemory(bool marks); // Pattern fill
};
```

## Allocation Records

For debugging, allocators can track detailed allocation info:

```cpp
const AZ::Debug::AllocationRecords* records = allocator->GetRecords();
// Contains: size, alignment, callstack, thread ID, timestamp
```

Enable tracking:
```cpp
AZ::AllocatorManager::Instance().SetTrackingMode(AZ::Debug::AllocationRecords::Mode::Full);
```

## IAllocator Interface

Core interface all allocators implement:

```cpp
class IAllocator
{
    virtual AllocateAddress allocate(size_type byteSize, align_type alignment) = 0;
    virtual size_type deallocate(pointer ptr, size_type byteSize, align_type alignment) = 0;
    virtual AllocateAddress reallocate(pointer ptr, size_type newSize, align_type newAlignment) = 0;
    virtual size_type get_allocated_size(pointer ptr, align_type alignment) const = 0;

    virtual void GarbageCollect() { }
    virtual size_type NumAllocatedBytes() const { return 0; }
    virtual const char* GetName() const;
    virtual bool IsReady() const { return true; }

    // Debug/profiling
    virtual AllocatorDebugConfig GetDebugConfig() { return {}; }
    virtual void SetProfilingActive(bool active) { }
    virtual bool IsProfilingActive() const { return false; }
};
```

## Best Practices

1. **Always use AZ_CLASS_ALLOCATOR** for classes that will be heap-allocated
2. **Use PoolAllocator** for small, frequently allocated objects of uniform size
3. **Use ChildAllocator** to track memory usage by subsystem without overhead
4. **Use azcreate/azdestroy** for objects, **azmalloc/azfree** for raw memory
5. **Avoid array new[]** — use `AZStd::vector` instead
6. **Call GarbageCollect()** periodically to return unused memory to OS
