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
#include <AzCore/Memory/AllocatorInstance.h>


/**
 * AZ Memory allocation supports all best know allocation schemes. Even though we highly recommend using the
 * class overriding of new/delete operators for which we provide \ref ClassAllocators. We don't restrict to
 * use whatever you need, each way has it's benefits and drawback. Each of those will be described as we go along.
 * In every macro that doesn't require to specify an allocator AZ::SystemAllocator is implied.
 */
#if !defined(_RELEASE)
    #define aznew                                                   new
    #define aznewex(_Name)                                          new

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)

/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator,#_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3,#_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#else
    #define aznew           new
    #define aznewex(_Name)  new

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)

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
#define azrealloc_2(_1, _2)             AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(_1, _2)
/// azrealloc(ptr, size, alignment)
#define azrealloc_3(_1, _2, _3)         AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(_1, _2, _3)
/// azrealloc(ptr, size, alignment, Allocator)
#define azrealloc_4(_1, _2, _3, _4)     AZ::AllocatorInstance<_4>::Get().reallocate(_1, _2, _3)

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
#define azfree_1(_1)                do { if (_1) { AZ::AllocatorInstance< AZ::SystemAllocator >::Get().deallocate(_1); } }   while (0)
/// azfree(pointer,allocator)
#define azfree_2(_1, _2)            do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1); } }                    while (0)
/// azfree(pointer,allocator,size)
#define azfree_3(_1, _2, _3)        do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1, _3); } }                while (0)
/// azfree(pointer,allocator,size,alignment)
#define azfree_4(_1, _2, _3, _4)    do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1, _3, _4); } }            while (0)

/**
 * azfree is equivalent to ::free(...). Is should be used with corresponding azmalloc call.
 * macro signature: azfree(Pointer* ptr, AllocatorType = AZ::SystemAllocator, size_t byteSize = Unknown, size_t alignment = DefaultAlignment);
 * \note Providing allocation size (byteSize) and alignment is optional, but recommended when possible. It will generate faster code.
 */
#define azfree(...)         AZ_MACRO_SPECIALIZE(azfree_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// Returns allocation size, based on it's pointer \ref AZ::IAllocator::AllocationSize.
#define azallocsize(_Ptr, _Allocator)    AZ::AllocatorInstance< _Allocator >::Get().AllocationSize(_Ptr)
/// Returns the new expanded size or 0 if NOT supported by the allocator \ref AZ::IAllocator::Resize.
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
#define _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator)                                                                                                                          \
    /* class-specific allocation functions */                                                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) {                                                                                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                    \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, static_cast<std::size_t>(align));                                                                                   \
    }                                                                                                                                                                                        \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                             \
        return operator new(size, align);                                                                                                                               \
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
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                       \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, static_cast<std::size_t>(align)); }                                                                          \
    }                                                                                                                                                                                        \
    void operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align) noexcept {                                                  \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                             \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                   \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                              \
    }
#else
#define _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator)
#endif

#define AZ_CLASS_ALLOCATOR(_Class, _Allocator, ...)                                                                                                                              \
    /* ========== placement operators (default) ========== */                                                                                                                       \
    AZ_FORCE_INLINE void* operator new(std::size_t, void* p)    { return p; }   /* placement new */                                                                                 \
    AZ_FORCE_INLINE void* operator new[](std::size_t, void* p)  { return p; }   /* placement array new */                                                                           \
    AZ_FORCE_INLINE void  operator delete(void*, void*)         { }             /* placement delete, called when placement new asserts */                                           \
    AZ_FORCE_INLINE void  operator delete[](void*, void*)       { }             /* placement array delete */                                                                        \
    /* ========== standard operator new/delete ========== */                                                                                                                        \
    AZ_FORCE_INLINE void* operator new(std::size_t size) {                      /* default operator new (called with "new _Class()") */                                             \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));           \
        AZ_Warning(0, true/*false*/, "Make sure you use aznew, offers better tracking! (%s)", #_Class /*Warning temporarily disabled until engine is using AZ allocators.*/);       \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, AZStd::alignment_of< _Class >::value);                                                                     \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, std::size_t size) {    /* default operator delete */                                                                             \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, AZStd::alignment_of< _Class >::value); }                                                            \
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
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                              \
    AZ_FORCE_INLINE static void* AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                    \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                           \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE static void AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                       \
        AZ::AllocatorInstance< _Allocator >::Get().deallocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                        \
    }                                                                                                                                                                               \
    template<bool Placeholder = true> void AZ_CLASS_ALLOCATOR_DECLARED();                                                                                                           \
    _AZ_CLASS_ALLOCATOR_ALIGNED_NEW(_Class, _Allocator)

// If you want to avoid including the memory manager class in the header file use the _DECL (declaration) and _IMPL (implementations/definition) macros
#if __cpp_aligned_new
// Declares the C++17 aligned_new operator new/operator delete overloads
#define _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW                                                                                                                 \
    /* class-specific allocation functions */                                                                                                                \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align);                                                                              \
    [[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                              \
    [[nodiscard]] void* operator new[](std::size_t, std::align_val_t);                                                                                       \
    [[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept;                                            \
    void operator delete(void* p, std::size_t size, std::align_val_t align) noexcept;                                                                        \
    void operator delete[](void* p, std::size_t size, std::align_val_t align) noexcept;
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
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                    \
    static void* AZ_CLASS_ALLOCATOR_Allocate();                                                                                                           \
    static void  AZ_CLASS_ALLOCATOR_DeAllocate(void* object);                                                                                             \
    template<bool Placeholder = true> void AZ_CLASS_ALLOCATOR_DECLARED();                                                                                 \
    _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW

#if __cpp_aligned_new
// Defines the C++17 aligned_new operator new/operator delete overloads
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Template)                                                                                                                     \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align) {                                                                                                      \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                               \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, static_cast<std::size_t>(align));                                                                                              \
    }                                                                                                                                                                                                   \
    _Template [[nodiscard]] void* _Class::operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {                                                                      \
        return operator new(size, align);                                                                                                                                                               \
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
    _Template void _Class::operator delete(void* p, std::size_t size, std::align_val_t align) noexcept {                                                                                                \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, static_cast<std::size_t>(align)); }                                                                                     \
    }                                                                                                                                                                                                   \
    _Template void _Class::operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align) noexcept {                                           \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"                                                        \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n"                                                                                              \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");                                                                         \
    }
#else
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Template)
#endif
#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Template)                                                                                                                                                                     \
    /* ========== standard operator new/delete ========== */                                                                                                                                                                                        \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::operator new(std::size_t size)                                                                                                                                                                                      \
    {                                                                                                                                                                                                                                               \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_Class): %d", size, sizeof(_Class));                                                                                \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, AZStd::alignment_of< _Class >::value);                                                                                                                     \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::operator delete(void* p, std::size_t size)  {                                                                                                                                                                                      \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, AZStd::alignment_of< _Class >::value); }                                                                                                                            \
    }                                                                                                                                                                                                                                               \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */                                                                                                                                                                                              \
    _Template                                                                                                                                                                                                                                       \
    [[nodiscard]] void* _Class::AZ_CLASS_ALLOCATOR_Allocate() {                                                                                                                                                                                     \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                                                                          \
    }                                                                                                                                                                                                                                               \
    _Template                                                                                                                                                                                                                                       \
    void _Class::AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                                                                                                      \
        AZ::AllocatorInstance< _Allocator >::Get().deallocate(object, sizeof(_Class), AZStd::alignment_of< _Class >::value);                                                                                                                        \
    }                                                                                                                                                                                                                                               \
    _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _Template)

#define AZ_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, ...)                                                                                                                                                                                         \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, )

#define AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, ...)                                                                                                                                                                                \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, template<>)

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
 * void* operator new(std::size_t);
 * void* operator new[](std::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the AZ::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(std::size_t size)         { return AZ::OperatorNew(size); }
 * void* operator new[](std::size_t size)       { return AZ::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { AZ::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { AZ::OperatorDeleteArray(ptr); }
 */
namespace AZ
{
    /**
    * Generic wrapper for binding allocator to an AZStd one.
    * \note AZStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template <class Allocator>
    class AZStdAlloc
    {
    public:
        AZ_ALLOCATOR_DEFAULT_TRAITS

        AZ_FORCE_INLINE AZStdAlloc()
        {
            if (AllocatorInstance<Allocator>::IsReady())
            {
                m_name = AllocatorInstance<Allocator>::Get().GetName();
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
        AZ_FORCE_INLINE pointer allocate(size_t byteSize, size_t alignment)
        {
            return AllocatorInstance<Allocator>::Get().allocate(byteSize, alignment);
        }
        AZ_FORCE_INLINE pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize, newAlignment);
        }
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().deallocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const            { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name)  { m_name = name; }
        size_type                   max_size() const            { return AllocatorInstance<Allocator>::Get().max_size(); }
        size_type                   get_allocated_size() const  { return AllocatorInstance<Allocator>::Get().get_allocated_size(); }

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
        AZ_ALLOCATOR_DEFAULT_TRAITS

        AZ_FORCE_INLINE AZStdIAllocator(IAllocator* allocator, const char* name = "AZ::AZStdIAllocator")
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
        AZ_FORCE_INLINE pointer allocate(size_t byteSize, size_t alignment)
        {
            return m_allocator->allocate(byteSize, alignment);
        }
        AZ_FORCE_INLINE pointer reallocate(pointer ptr, size_t newSize, align_type newAlignment = 1)
        {
            return m_allocator->reallocate(ptr, newSize, newAlignment);
        }
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_t byteSize, size_t alignment)
        {
            m_allocator->deallocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE const char* get_name() const { return m_name; }
        AZ_FORCE_INLINE void        set_name(const char* name) { m_name = name; }
        size_type                   max_size() const { return m_allocator->GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const { return m_allocator->NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool operator==(const AZStdIAllocator& rhs) const { return m_allocator == rhs.m_allocator; }
        AZ_FORCE_INLINE bool operator!=(const AZStdIAllocator& rhs) const { return m_allocator != rhs.m_allocator; }
    private:
        IAllocator* m_allocator;
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
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using functor_type = IAllocator&(*)(); ///< Function Pointer must return IAllocator&.
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
        pointer allocate(size_t byteSize, size_t alignment)
        {
            return m_allocatorFunctor().allocate(byteSize, alignment);
        }
        pointer reallocate(pointer ptr, size_t newSize, size_t newAlignment = 1)
        {
            return m_allocatorFunctor().reallocate(ptr, newSize, newAlignment);
        }
        void deallocate(pointer ptr, size_t byteSize, size_t alignment)
        {
            m_allocatorFunctor().deallocate(ptr, byteSize, alignment);
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
    [[nodiscard]] void* OperatorNew(std::size_t size);
    void OperatorDelete(void* ptr);
    void OperatorDelete(void* ptr, std::size_t size);

    [[nodiscard]] void* OperatorNewArray(std::size_t size);
    void OperatorDeleteArray(void* ptr);
    void OperatorDeleteArray(void* ptr, std::size_t size);

#if __cpp_aligned_new
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align);
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
