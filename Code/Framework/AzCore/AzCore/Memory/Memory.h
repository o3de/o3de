/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/AllocatorWrapper.h>
#include <AzCore/Memory/Config.h>
#include <AzCore/Memory/SimpleSchemaAllocator.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>

#include <AzCore/std/typetraits/has_member_function.h>

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Module/Environment.h>


// Declarations of new overloads, definitions are in NewAndDelete.inl, or you can link your own versions
// AZ::Internal::AllocatorDummy is merely to differentiate our overloads from any other operator new signatures
void* operator new(std::size_t, const AZ::Internal::AllocatorDummy*);
void* operator new[](std::size_t, const AZ::Internal::AllocatorDummy*);

/**
 * AZ Memory allocation supports all best know allocation schemes. Even though we highly recommend using the
 * class overriding of new/delete operators for which we provide \ref ClassAllocators. We don't restrict to
 * use whatever you need, each way has it's benefits and drawback. Each of those will be described as we go along.
 * In every macro that doesn't require to specify an allocator AZ::SystemAllocator is implied.
 */
#if !defined(_RELEASE)
    #define aznew                                                   aznewex((const char*)nullptr)
    #define aznewex(_Name)                                          new(__FILE__, __LINE__, _Name)

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, 0, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, _2, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, "azmalloc", __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4, __FILE__, __LINE__)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4, __FILE__, __LINE__)

/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator,#_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3,#_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#else
    #define aznew           new((const AZ::Internal::AllocatorDummy*)nullptr)
    #define aznewex(_Name)  aznew

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, 0, 0)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().Allocate(_1, _2, 0)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, 0, _4)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().Allocate(_1, _2, _5, _4)

/// azcreate(class)
    #define azcreate_1(_1)                                          new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1()
/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, #_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#endif

/**
* azmalloc is equivalent to ::malloc(...). It should be used with corresponding azfree call.
* macro signature: azmalloc(size_t byteSize, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define azmalloc(...)       AZ_MACRO_SPECIALIZE(azmalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azcalloc(size)
#define azcalloc_1(_1)                  ::memset(azmalloc_1(_1), 0, _1)
/// azcalloc(size, alignment)
#define azcalloc_2(_1, _2)              ::memset(azmalloc_2(_1, _2), 0, _1);
/// azcalloc(size, alignment, Allocator)
#define azcalloc_3(_1, _2, _3)          ::memset(azmalloc_3(_1, _2, _3), 0, _1);
/// azcalloc(size, alignment, allocationName)
#define azcalloc_4(_1, _2, _3, _4)      ::memset(azmalloc_4(_1, _2, _3, _4), 0, _1);
/// azcalloc(size, alignment, allocationName, flags)
#define azcalloc_5(_1, _2, _3, _4, _5)  ::memset(azmalloc_5(_1, _2, _3, _4, _5), 0, _1);

/**
* azcalloc is equivalent to ::memset(azmalloc(...), 0, size);
* macro signature: azcalloc(size, alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define azcalloc(...)       AZ_MACRO_SPECIALIZE(azcalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azrealloc(ptr, size)
#define azrealloc_2(_1, _2)             AZ::AllocatorInstance<AZ::SystemAllocator>::Get().ReAllocate(_1, _2, 0)
/// azrealloc(ptr, size, alignment)
#define azrealloc_3(_1, _2, _3)         AZ::AllocatorInstance<AZ::SystemAllocator>::Get().ReAllocate(_1, _2, _3)
/// azrealloc(ptr, size, alignment, Allocator)
#define azrealloc_4(_1, _2, _3, _4)     AZ::AllocatorInstance<_4>::Get().ReAllocate(_1, _2, _3)

/**
* azrealloc is equivalent to ::realloc(...)
* macro signature: azrealloc(void* ptr, size_t size, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator)
*/
#define azrealloc(...)      AZ_MACRO_SPECIALIZE(azrealloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * azcreate is customized aznew function call. aznew can be used anywhere where we use new, while azcreate has a function call signature.
 * azcreate allows you to override the operator new and by this you can override the allocator per object instance. It should
 * be used with corresponding azdestroy call.
 * macro signature: azcreate(ClassName, CtorParams = (), AllocatorType = AZ::SystemAllocator, AllocationName = "ClassName", int flags = 0)
 */
#define azcreate(...)       AZ_MACRO_SPECIALIZE(azcreate_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azfree(pointer)
#define azfree_1(_1)                do { if (_1) { AZ::AllocatorInstance< AZ::SystemAllocator >::Get().DeAllocate(_1); } }   while (0)
/// azfree(pointer,allocator)
#define azfree_2(_1, _2)            do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1); } }                    while (0)
/// azfree(pointer,allocator,size)
#define azfree_3(_1, _2, _3)        do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3); } }                while (0)
/// azfree(pointer,allocator,size,alignment)
#define azfree_4(_1, _2, _3, _4)    do { if (_1) { AZ::AllocatorInstance< _2 >::Get().DeAllocate(_1, _3, _4); } }            while (0)

/**
 * azfree is equivalent to ::free(...). Is should be used with corresponding azmalloc call.
 * macro signature: azfree(Pointer* ptr, AllocatorType = AZ::SystemAllocator, size_t byteSize = Unknown, size_t alignment = DefaultAlignment);
 * \note Providing allocation size (byteSize) and alignment is optional, but recommended when possible. It will generate faster code.
 */
#define azfree(...)         AZ_MACRO_SPECIALIZE(azfree_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// Returns allocation size, based on it's pointer \ref AZ::IAllocatorAllocate::AllocationSize.
#define azallocsize(_Ptr, _Allocator)    AZ::AllocatorInstance< _Allocator >::Get().AllocationSize(_Ptr)
/// Returns the new expanded size or 0 if NOT supported by the allocator \ref AZ::IAllocatorAllocate::Resize.
#define azallocresize(_Ptr, _NewSize, _Allocator) AZ::AllocatorInstance< _Allocator >::Get().Resize(_Ptr, _NewSize)

namespace AZ {
    // \note we can use AZStd::Internal::destroy<pointer_type>::single(ptr) if we template the entire function.
    namespace Memory {
        namespace Internal {
            template<class T>
            AZ_FORCE_INLINE void call_dtor(T* ptr)          { (void)ptr; ptr->~T(); }
        }
    }
}

#define azdestroy_1(_1)         do { AZ::Memory::Internal::call_dtor(_1); azfree_1(_1); } while (0)
#define azdestroy_2(_1, _2)      do { AZ::Memory::Internal::call_dtor(_1); azfree_2(_1, _2); } while (0)
#define azdestroy_3(_1, _2, _3)     do { AZ::Memory::Internal::call_dtor(reinterpret_cast<_3*>(_1)); azfree_4(_1, _2, sizeof(_3), AZStd::alignment_of< _3 >::value); } while (0)

/**
 * azdestroy should be used only with corresponding azcreate.
 * macro signature: azdestroy(Pointer*, AllocatorType = AZ::SystemAllocator, ClassName = Unknown)
 * \note Providing ClassName is optional, but recommended when possible. It will generate faster code.
 */
#define azdestroy(...)      AZ_MACRO_SPECIALIZE(azdestroy_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * Class Allocator (override new/delete operators)
 *
 * use this macro inside your objects to define it's default allocation (ex.):
 * class MyObj
 * {
 *   public:
 *          AZ_CLASS_ALLOCATOR(MyObj,SystemAllocator,0) - inline version requires to include the allocator (in this case sysallocator.h)
 *      or
 *          AZ_CLASS_ALLOCATOR_DECL in the header and AZ_CLASS_ALLOCATOR_IMPL(MyObj,SystemAllocator,0) in the cpp file. This way you don't need
 *          to include the allocator header where you decl your class (MyObj).
 *      ...
 * };
 *
 * \note We don't support array operators [] because they insert a compiler/platform
 * dependent array header, which then breaks the alignment in some cases.
 * If you want to use dynamic array use AZStd::vector or AZStd::fixed_vector.
 * Of course you can use placement new and do the array allocation anyway if
 * it's really needed.
 */
#if __cpp_aligned_new
// C++17 aligned alloc overloads for types with an alignment greater than sizeof(max_align_t)
#define _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)                                                                                                                          \
    /* class-specific allocation functions */                                                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) {                                                                                                             \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                               \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                             \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                               \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) {                                                        \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                    \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, static_cast<std::size_t>(align), _Flags, name ? name : #_Class, fileName, lineNum);                                 \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[]([[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align) {                                                                         \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                             \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                   \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                              \
        return AZ_INVALID_POINTER;                                                                                                                                                           \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                           \
        return operator new[](size, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*) {                                                                            \
        return operator new[](size, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align) noexcept {                                                                                                                         \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                       \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, static_cast<std::size_t>(align)); }                                                                          \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                                  \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {                 \
        return operator delete(p, 0, align);                                                                                                                                    \
    }                                                                                                                                                                                        \
    void operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::align_val_t align) noexcept {                                                                                     \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                             \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                   \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                              \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, [[maybe_unused]] std::size_t size, std::align_val_t align) noexcept {                                                                                    \
        return operator delete[](p, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                                \
        return operator delete[](p, align);                                                                                                                                                  \
    }                                                                                                                                                                                        \
    void operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {               \
        return operator delete[](p, align);                                                                                                                                                  \
    }
#else
#define _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)
#endif

#define AZ_CLASS_ALLOCATOR(_Class, _Allocator, _Flags)                                                                                                                              \
    /* ========== placement operators (default) ========== */                                                                                                                       \
    AZ_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }   /* placement new */                                                                                 \
    AZ_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }   /* placement array new */                                                                           \
    AZ_FORCE_INLINE void  operator delete(void*, void*)         { }             /* placement delete, called when placement new asserts */                                           \
    AZ_FORCE_INLINE void  operator delete[](void*, void*)       { }             /* placement array delete */                                                                        \
    /* ========== standard operator new/delete ========== */                                                                                                                        \
    AZ_FORCE_INLINE void* operator new(std::size_t size) {                      /* default operator new (called with "new _Class()") */                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));           \
        AZ_Warning(0, true/*false*/, "Make sure you use aznew, offers better tracking!" /*Warning temporarily disabled until engine is using AZ allocators.*/);                     \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags,#_Class);                                                     \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, std::size_t size) {    /* default operator delete */                                                                             \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, AZStd::alignment_of< _Class >::value); }                                                            \
    }                                                                                                                                                                               \
    /* ========== aznew (called "aznew _Class()") ========== */                                                                                                                     \
    AZ_FORCE_INLINE void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name) { /* with tracking */                                                 \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);              \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) {                 /* without tracking */                                              \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value);                                                                     \
    }                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                                          \
    AZ_FORCE_INLINE void  operator delete(void* p, const char*, int, const char*) {                                                                                                 \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                        \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, const AZ::Internal::AllocatorDummy*) {                                                                                           \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                        \
    }                                                                                                                                                                               \
    /* ========== Unsupported operators ========== */                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t) {                                         /* default array operator new (called with "new _Class[x]") */                      \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return AZ_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete[](void*) {                                            /* default array operator delete */                                                 \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                      \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*) {          /* array operator aznew with tracking (called with "aznew _Class[x]") */            \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return AZ_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const AZ::Internal::AllocatorDummy*) {    /* array operator aznew without tracking (called with "aznew _Class[x]") */         \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                    \
                         "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                           \
                         "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                      \
        return AZ_INVALID_POINTER;                                                                                                                                                  \
    }                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                              \
    AZ_FORCE_INLINE static void* AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                    \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value, _Flags, #_Class);                                          \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE static void AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                       \
        AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                        \
    }                                                                                                                                                                               \
    template<bool Placeholder = true> void AZ_CLASS_ALLOCATOR_DECLARED();                                                                                                           \
    _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator, _Flags)

// If you want to avoid including the memory manager class in the header file use the _DECL (declaration) and _IMPL (implementations/definition) macros
#if __cpp_aligned_new
// Declares the C++17 aligned_new operator new/operator delete overloads
#define _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW                                                                                                                 \
    /* class-specific allocation functions */                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align);                                                                              \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                              \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name);                         \
    [[nodiscard]] void* operator new[](std::size_t, std::align_val_t);                                                                                       \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                            \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*);                                             \
    void operator delete(void* p, std::align_val_t align) noexcept;                                                                                          \
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept;                                                                        \
    void operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept;                                                                   \
    void operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char* name) noexcept;   \
    void operator delete[](void* p, std::align_val_t align) noexcept;                                                                                        \
    void operator delete[](void* p, std::size_t size, std::align_val_t align) noexcept;                                                                      \
    void operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept;                                                                 \
    void operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char* name) noexcept;
#else
#define _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW
#endif

#define AZ_CLASS_ALLOCATOR_DECL                                                                                                                           \
    /* ========== placement operators (default) ========== */                                                                                             \
    AZ_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }                                                                             \
    AZ_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }                                                                             \
    AZ_FORCE_INLINE void operator delete(void*, void*)          { }                                                                                       \
    AZ_FORCE_INLINE void operator delete[](void*, void*)        { }                                                                                       \
    /* ========== standard operator new/delete ========== */                                                                                              \
    void* operator new(std::size_t size);                                                                                                                 \
    void  operator delete(void* p, std::size_t size);                                                                                                     \
    /* ========== aznew (called "aznew _Class()")========== */                                                                                            \
    void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name);                                                            \
    void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*);                                                                            \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                \
    void  operator delete(void* p, const char*, int, const char*);                                                                                        \
    void  operator delete(void* p, const AZ::Internal::AllocatorDummy*);                                                                                  \
    /* ========== Unsupported operators ========== */                                                                                                     \
    AZ_FORCE_INLINE void* operator new[](std::size_t) {                                                                                                   \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
                        "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                  \
                        "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                             \
        return AZ_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    AZ_FORCE_INLINE void  operator delete[](void*) {                                                                                                      \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
                        "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                  \
                        "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                             \
    }                                                                                                                                                     \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const char*, int, const char*, const AZ::Internal::AllocatorDummy*) {                               \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
            "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                              \
            "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                         \
        return AZ_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    AZ_FORCE_INLINE void* operator new[](std::size_t, const AZ::Internal::AllocatorDummy*) {                                                              \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"          \
            "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                              \
            "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                         \
        return AZ_INVALID_POINTER;                                                                                                                        \
    }                                                                                                                                                     \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                    \
    static void* AZ_CLASS_ALLOCATOR_Allocate();                                                                                                           \
    static void  AZ_CLASS_ALLOCATOR_DeAllocate(void* object);                                                                                             \
    template<bool Placeholder = true> void AZ_CLASS_ALLOCATOR_DECLARED();                                                                                 \
    _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW

#if __cpp_aligned_new
// Defines the C++17 aligned_new operator new/operator delete overloads
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)                                                                                                                     \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) {                                                 \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                               \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, static_cast<std::size_t>(align), _Flags, name ? name : #_Class, fileName, lineNum);                                            \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align) {                                                                                                      \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                                          \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                      \
        return operator new(size, align, nullptr, 0, #_Class);                                                                                                                                          \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[]([[maybe_unused]] std::size_t, [[maybe_unused]] std::align_val_t) {                                                                             \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                                        \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                              \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                         \
        return AZ_INVALID_POINTER;                                                                                                                                                                      \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                    \
        return operator new[](size, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new[](std::size_t size, std::align_val_t align, const char*, int, const char*) {                                                                     \
        return operator new[](size, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, static_cast<std::size_t>(align)); }                                                                                     \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align) noexcept {                                                                                                                  \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                           \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete(void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {          \
        return operator delete(p, 0, align);                                                                                                                                               \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::align_val_t align) noexcept {                                                                              \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                                        \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                              \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                         \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, [[maybe_unused]] std::size_t size, std::align_val_t align) noexcept {                                                                             \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                                         \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[](void* p, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept {        \
        return _Class::operator delete[](p, align);                                                                                                                                                             \
    }
#else
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)
#endif
#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, _Template)                                                                                                                                                                     \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size)                                                                                                                                                                                      \
    {                                                                                                                                                                                                                                               \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_Class): %d", size, sizeof(_Class));                                                                                \
        AZ_Warning(0, false, "Make sure you use aznew, offers better tracking!");                                                                                                                                                                   \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags,#_Class);                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::operator delete(void* p, std::size_t size)  {                                                                                                                                                                                      \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p, size, AZStd::alignment_of< _Class >::value); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== aznew (called "aznew _Class()")========== */                                                                                                                                                                                      \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size, const char* fileName, int lineNum, const char* name) {                                                                                                                               \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value, _Flags, (name == 0) ?#_Class: name, fileName, lineNum);                                                                              \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) {                                                                                                                                               \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_type): %d", size, sizeof(_Class));                                                                                 \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(size, AZStd::alignment_of< _Class >::value);                                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    /* ========== Symetrical delete operators (required incase aznew throws) ========== */                                                                                                                                                          \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const char*, int, const char*) {                                                                                                                                                                         \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void  _Class::operator delete(void* p, const AZ::Internal::AllocatorDummy*) {                                                                                                                                                                   \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(p); }                                                                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                                                                                     \
        return AZ::AllocatorInstance< _Allocator >::Get().Allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value, _Flags, #_Class);                                                                                                          \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                                                                                                      \
        AZ::AllocatorInstance< _Allocator >::Get().DeAllocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Flags, _Template)

#define AZ_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, _Flags)                                                                                                                                                                                         \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags,)

#define AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, _Flags)                                                                                                                                                                                \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Flags, template<>)

//////////////////////////////////////////////////////////////////////////
// new operator overloads

// you can redefine this macro to whatever suits you.
#ifndef AZCORE_GLOBAL_NEW_ALIGNMENT
#   define AZCORE_GLOBAL_NEW_ALIGNMENT 16
#endif


/**
 * By default AZCore doesn't overload operator new and delete. This is a no-no for middle-ware.
 * You are encouraged to do that in your executable. What you need to do is to pipe all allocation trough AZCore memory manager.
 * AZCore relies on \ref AZ_CLASS_ALLOCATOR to specify the class default allocator or on explicit
 * azcreate/azdestroy calls which provide the allocator. If you class doesn't not implement the
 * \ref AZ_CLASS_ALLOCATOR when you call a new/delete they will use the global operator new/delete. In addition
 * if you call aznew on a class without AZ_CLASS_ALLOCATOR you will need to implement new operator specific to
 * aznew call signature.
 * So in an exception free environment (AZLibs don't have exception support) you need to implement the following functions:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*);
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*);
 * void* operator new(std::size_t);
 * void* operator new[](std::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the AZ::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)       { return AZ::OperatorNew(size,fileName,lineNum,name); }
 * void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)     { return AZ::OperatorNewArray(size,fileName,lineNum,name); }
 * void* operator new(std::size_t size)         { return AZ::OperatorNew(size); }
 * void* operator new[](std::size_t size)       { return AZ::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { AZ::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { AZ::OperatorDeleteArray(ptr); }
 */
namespace AZ
{
    namespace AllocatorStorage
    {
        /// A private structure to create heap-storage for an allocator that won't expire until other static module members are destructed.
        struct LazyAllocatorRef
        {
            using CreationFn = IAllocator*(*)(void*);
            using DestructionFn = void(*)(IAllocator&);

            ~LazyAllocatorRef();
            void Init(size_t size, size_t alignment, CreationFn creationFn, DestructionFn destructionFn);

            IAllocator* m_allocator = nullptr;
            DestructionFn m_destructor = nullptr;
        };

        /**
        * A base class for all storage policies. This exists to provide access to private IAllocator methods via template friends.
        */
        template<class Allocator>
        class StoragePolicyBase
        {
        protected:
            static void Create(Allocator& allocator, const typename Allocator::Descriptor& desc, bool lazilyCreated)
            {
                allocator.Create(desc);
                allocator.PostCreate();
                allocator.SetLazilyCreated(lazilyCreated);
            }

            static void SetLazilyCreated(Allocator& allocator, bool lazilyCreated)
            {
                allocator.SetLazilyCreated(lazilyCreated);
            }

            static void Destroy(IAllocator& allocator)
            {
                allocator.PreDestroy();
                allocator.Destroy();
            }
        };

        /**
        * EnvironmentStoragePolicy stores the allocator singleton in the shared Environment.
        * This is the default, preferred method of storing allocators.
        */
        template<class Allocator>
        class EnvironmentStoragePolicy : public StoragePolicyBase<Allocator>
        {
        public:
            static IAllocator& GetAllocator()
            {
                if (!s_allocator)
                {
                    // Assert here before attempting to resolve. Otherwise a module-local
                    // environment will be created which will result in a much more difficult to
                    // locate problem
                    AZ_Assert(AZ::Environment::IsReady(), "Environment has not been attached yet, allocator cannot be created/resolved");
                    if (AZ::Environment::IsReady())
                    {
                        s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                        AZ_Assert(s_allocator, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
                    }
                }
                return *s_allocator;
            }

            static void Create(const typename Allocator::Descriptor& desc)
            {
                if (!s_allocator)
                {
                    s_allocator = Environment::CreateVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                    if (s_allocator->IsReady()) // already created in a different module
                    {
                        return;
                    }
                }
                else
                {
                    AZ_Assert(s_allocator->IsReady(), "Allocator '%s' already created!", AzTypeInfo<Allocator>::Name());
                }

                StoragePolicyBase<Allocator>::Create(*s_allocator, desc, false);
            }

            static void Destroy()
            {
                if (s_allocator)
                {
                    if (s_allocator.IsOwner())
                    {
                        StoragePolicyBase<Allocator>::Destroy(*s_allocator);
                    }
                    s_allocator = nullptr;
                }
                else
                {
                    AZ_Assert(false, "Allocator '%s' NOT ready for use! Call Create first!", AzTypeInfo<Allocator>::Name());
                }
            }

            AZ_FORCE_INLINE static bool IsReady()
            {
                if (Environment::IsReady())
                {
                    if (!s_allocator) // if not there check the environment (if available)
                    {
                        s_allocator = Environment::FindVariable<Allocator>(AzTypeInfo<Allocator>::Name());
                    }
                    return s_allocator && s_allocator->IsReady();
                }
                else
                {
                    return false;
                }
            }

            static EnvironmentVariable<Allocator> s_allocator;
        };

        template<class Allocator>
        EnvironmentVariable<Allocator> EnvironmentStoragePolicy<Allocator>::s_allocator;

        /**
        * ModuleStoragePolicy stores the allocator in a static variable that is local to the module using it.
        * This forces separate instances of the allocator to exist in each module, and permits lazy instantiation.
        * We only tolerate this for some special allocators, primarily to maintain backwards compatibility with CryEngine,
        * since it still allocates outside of code in the data section.
        *
        * It has two ways of storing its allocator: either on the heap, which is the preferred way, since it guarantees
        * the memory for the allocator won't be deallocated (such as in a DLL) before anyone that's using it. If disabled
        * the allocator is stored in a static variable, which should only be used where this isn't a problem a shut-down
        * time, such as on a console.
        */
        template<class Allocator, bool StoreAllocatorOnHeap>
        struct ModuleStoragePolicyBase;
        
        template<class Allocator>
        struct ModuleStoragePolicyBase<Allocator, false>: public StoragePolicyBase<Allocator>
        {
        protected:
            // Use a static instance to store the allocator. This is not recommended when the order of shut-down with the module matters, as the allocator could have its memory destroyed 
            // before the users of it are destroyed. The primary use case for this is allocators that need to support the CRT, as they cannot allocate from the heap.
            static Allocator& GetModuleAllocatorInstance()
            {
                static Allocator* s_allocator = nullptr;
                static typename AZStd::aligned_storage<sizeof(Allocator), AZStd::alignment_of<Allocator>::value>::type s_storage;

                if (!s_allocator)
                {
                    s_allocator = new (&s_storage) Allocator;
                    StoragePolicyBase<Allocator>::Create(*s_allocator, typename Allocator::Descriptor(), true);
                }

                return *s_allocator;
            }
        };

        template<class Allocator>
        struct ModuleStoragePolicyBase<Allocator, true> : public StoragePolicyBase<Allocator>
        {
        protected:
            // Store-on-heap implementation uses the LazyAllocatorRef to create and destroy an allocator using heap-space so there isn't a problem with destruction order within the module.
            static Allocator& GetModuleAllocatorInstance()
            {
                static LazyAllocatorRef s_allocator;

                if (!s_allocator.m_allocator)
                {
                    s_allocator.Init(sizeof(Allocator), AZStd::alignment_of<Allocator>::value, [](void* mem) -> IAllocator* { return new (mem) Allocator; }, &StoragePolicyBase<Allocator>::Destroy);
                    StoragePolicyBase<Allocator>::Create(*static_cast<Allocator*>(s_allocator.m_allocator), typename Allocator::Descriptor(), true);
                }

                return *static_cast<Allocator*>(s_allocator.m_allocator);
            }
        };

        template<class Allocator, bool StoreAllocatorOnHeap = true>
        class ModuleStoragePolicy : public ModuleStoragePolicyBase<Allocator, StoreAllocatorOnHeap>
        {
        public:
            using Base = ModuleStoragePolicyBase<Allocator, StoreAllocatorOnHeap>;

            static IAllocator& GetAllocator()
            {
                return Base::GetModuleAllocatorInstance();
            }

            static void Create(const typename Allocator::Descriptor& desc = typename Allocator::Descriptor())
            {
                StoragePolicyBase<Allocator>::Create(Base::GetModuleAllocatorInstance(), desc, true);
            }

            static void Destroy()
            {
                StoragePolicyBase<Allocator>::Destroy(Base::GetModuleAllocatorInstance());
            }

            static bool IsReady()
            {
                return Base::GetModuleAllocatorInstance().IsReady();
            }
        };
    }

    namespace Internal
    {
        /**
        * The main class that provides access to the allocator singleton, with a customizable storage policy for that allocator.
        */
        template<class Allocator, typename StoragePolicy = AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
        class AllocatorInstanceBase
        {
        public:
            typedef typename Allocator::Descriptor Descriptor;

            AZ_FORCE_INLINE static IAllocatorAllocate& Get()
            {
                return *GetAllocator().GetAllocationSource();
            }

            AZ_FORCE_INLINE static IAllocator& GetAllocator()
            {
                return StoragePolicy::GetAllocator();
            }

            static void Create(const Descriptor& desc = Descriptor())
            {
                StoragePolicy::Create(desc);
            }

            static void Destroy()
            {
                StoragePolicy::Destroy();
            }

            AZ_FORCE_INLINE static bool IsReady()
            {
                return StoragePolicy::IsReady();
            }
        };
    }

    /**
     * Standard allocator singleton, using Environment storage. Specialize this for your
     * allocator if you need to control storage or lifetime, by changing the policy class
     * used in AllocatorInstanceBase.
     *
     * It is preferred that you don't do a complete specialization of AllocatorInstance,
     * as the logic governing creation and destruction of allocators is complicated and
     * susceptible to edge cases across all platforms and build types, and it is best to
     * keep the allocator code flowing through a consistent codepath.
     */
    template<class Allocator>
    class AllocatorInstance : public Internal::AllocatorInstanceBase<Allocator, AllocatorStorage::EnvironmentStoragePolicy<Allocator>>
    {
    };

    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public IAllocatorAllocate
    {
    public:
        // No descriptor is necessary, as the parent allocator is expected to already
        // be created and configured
        struct Descriptor {};
        using Parent = ParentAllocator;

        ChildAllocatorSchema(const Descriptor&) {}

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            return AZ::AllocatorInstance<Parent>::Get().Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            AZ::AllocatorInstance<Parent>::Get().DeAllocate(ptr, byteSize, alignment);
        }

        size_type Resize(pointer_type ptr, size_type newSize) override
        {
            return AZ::AllocatorInstance<Parent>::Get().Resize(ptr, newSize);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            return AZ::AllocatorInstance<Parent>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        size_type AllocationSize(pointer_type ptr) override
        {
            return AZ::AllocatorInstance<Parent>::Get().AllocationSize(ptr);
        }

        void GarbageCollect() override
        {
            AZ::AllocatorInstance<Parent>::Get().GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().NumAllocatedBytes();
        }

        size_type Capacity() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().Capacity();
        }

        size_type GetMaxAllocationSize() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetMaxAllocationSize();
        }

        size_type GetMaxContiguousAllocationSize() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetMaxContiguousAllocationSize();
        }

        size_type               GetUnAllocatedMemory(bool isPrint = false) const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetUnAllocatedMemory(isPrint);
        }

        IAllocatorAllocate*     GetSubAllocator() override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetSubAllocator();
        }
    };

    /**
    * Generic wrapper for binding allocator to an AZStd one.
    * \note AZStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template <class Allocator>
    class AZStdAlloc
    {
    public:
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        AZ_FORCE_INLINE AZStdAlloc()
        {
            if (AllocatorInstance<Allocator>::IsReady())
            {
                m_name = AllocatorInstance<Allocator>::GetAllocator().GetName();
            }
            else
            {
                m_name = AzTypeInfo<Allocator>::Name();
            }
        }
        AZ_FORCE_INLINE AZStdAlloc(const char* name)
            : m_name(name)     {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs)
            : m_name(rhs.m_name)  {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs, const char* name)
            : m_name(name) { (void)rhs; }
        AZ_FORCE_INLINE AZStdAlloc& operator=(const AZStdAlloc& rhs) { m_name = rhs.m_name; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return AllocatorInstance<Allocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type ptr, size_type newSize)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize);
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().DeAllocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const            { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name)  { m_name = name; }
        size_type                   max_size() const            { return AllocatorInstance<Allocator>::Get().GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const  { return AllocatorInstance<Allocator>::Get().NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool is_lock_free()                     { return AllocatorInstance<Allocator>::Get().is_lock_free(); }
        AZ_FORCE_INLINE bool is_stale_read_allowed()            { return AllocatorInstance<Allocator>::Get().is_stale_read_allowed(); }
        AZ_FORCE_INLINE bool is_delayed_recycling()             { return AllocatorInstance<Allocator>::Get().is_delayed_recycling(); }
    private:
        const char* m_name;
    };

    AZ_TYPE_INFO_TEMPLATE(AZStdAlloc, "{42D0AA1E-3C6C-440E-ABF8-435931150470}", AZ_TYPE_INFO_CLASS);

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return true; } // always true since they use the same instance of AllocatorInstance<Allocator>
    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return false; } // always false since they use the same instance of AllocatorInstance<Allocator>

    /**
     * Generic wrapper for binding IAllocator interface allocator.
     * This is basically the same as \ref AZStdAlloc but it allows
     * you to remove the template parameter and set you interface on demand.
     * of course at a cost of a pointer.
     */
    class AZStdIAllocator
    {
    public:
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        AZ_FORCE_INLINE AZStdIAllocator(IAllocatorAllocate* allocator, const char* name = "AZ::AZStdIAllocator")
            : m_allocator(allocator)
            , m_name(name)
        {
            AZ_Assert(m_allocator != NULL, "You must provide a valid allocator!");
        }
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs)
            : m_allocator(rhs.m_allocator)
            , m_name(rhs.m_name)  {}
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs, const char* name)
            : m_allocator(rhs.m_allocator)
            , m_name(name) { (void)rhs; }
        AZ_FORCE_INLINE AZStdIAllocator& operator=(const AZStdIAllocator& rhs) { m_allocator = rhs.m_allocator; m_name = rhs.m_name; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocator->Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocator->Resize(ptr, newSize);
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocator->DeAllocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name) { m_name = name; }
        size_type                   max_size() const { return m_allocator->GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const { return m_allocator->NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool operator==(const AZStdIAllocator& rhs) const { return m_allocator == rhs.m_allocator; }
        AZ_FORCE_INLINE bool operator!=(const AZStdIAllocator& rhs) const { return m_allocator != rhs.m_allocator; }
    private:
        IAllocatorAllocate* m_allocator;
        const char* m_name;
    };

    /**
    * Generic wrapper for binding IAllocator interface allocator.
    * This is basically the same as \ref AZStdAlloc but it allows
    * you to remove the template parameter and retrieve the allocator from a supplied function
    * pointer
    */
    class AZStdFunctorAllocator
    {
    public:
        using pointer_type = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using allow_memory_leaks = AZStd::false_type; ///< Regular allocators should not leak.
        using functor_type = IAllocatorAllocate&(*)(); ///< Function Pointer must return IAllocatorAllocate&.
                                                       ///< function pointers do not support covariant return types

        constexpr AZStdFunctorAllocator(functor_type allocatorFunctor, const char* name = "AZ::AZStdFunctorAllocator")
            : m_allocatorFunctor(allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr AZStdFunctorAllocator(const AZStdFunctorAllocator& rhs, const char* name)
            : m_allocatorFunctor(rhs.m_allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr AZStdFunctorAllocator(const AZStdFunctorAllocator& rhs) = default;
        constexpr AZStdFunctorAllocator& operator=(const AZStdFunctorAllocator& rhs) = default;
        pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocatorFunctor().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocatorFunctor().Resize(ptr, newSize);
        }
        void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocatorFunctor().DeAllocate(ptr, byteSize, alignment);
        }
        constexpr const char* get_name() const { return m_name; }
        void set_name(const char* name) { m_name = name; }
        size_type max_size() const { return m_allocatorFunctor().GetMaxContiguousAllocationSize(); }
        size_type get_allocated_size() const { return m_allocatorFunctor().NumAllocatedBytes(); }

        constexpr bool operator==(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor == rhs.m_allocatorFunctor; }
        constexpr bool operator!=(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor != rhs.m_allocatorFunctor; }
    private:
        functor_type m_allocatorFunctor;
        const char* m_name;
    };

    /**
    * Helper class to determine if type T has a AZ_CLASS_ALLOCATOR defined,
    * so we can safely call aznew on it. -  AZClassAllocator<ClassType>....
    */
    AZ_HAS_MEMBER(AZClassAllocator, AZ_CLASS_ALLOCATOR_DECLARED, void, ());

    // {@ Global New/Delete Operators
    [[nodiscard]] void* OperatorNew(std::size_t size, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNew(std::size_t size);
    void OperatorDelete(void* ptr);
    void OperatorDelete(void* ptr, std::size_t size);

    [[nodiscard]] void* OperatorNewArray(std::size_t size, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNewArray(std::size_t size);
    void OperatorDeleteArray(void* ptr);
    void OperatorDeleteArray(void* ptr, std::size_t size);

#if __cpp_aligned_new
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align);
    [[nodiscard]] void* OperatorNewArray(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name = nullptr);
    [[nodiscard]] void* OperatorNewArray(std::size_t size, std::align_val_t align);
    void OperatorDelete(void* ptr, std::size_t size, std::align_val_t align);
    void OperatorDeleteArray(void* ptr, std::size_t size, std::align_val_t align);
#endif
    // @}
}

#define AZ_PAGE_SIZE AZ_TRAIT_OS_DEFAULT_PAGE_SIZE
#define AZ_DEFAULT_ALIGNMENT (sizeof(void*))

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#define AZ_CORE_MAX_ALLOCATOR_SIZE AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE
