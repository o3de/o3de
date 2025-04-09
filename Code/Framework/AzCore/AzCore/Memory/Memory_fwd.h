/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <new>

/**
 * AZ Memory allocation supports all best know allocation schemes. Even though we highly recommend using the
 * class overriding of new/delete operators for which we provide \ref ClassAllocators. We don't restrict to
 * use whatever you need, each way has it's benefits and drawback. Each of those will be described as we go along.
 * In every macro that doesn't require to specify an allocator AZ::SystemAllocator is implied.
 */
#define aznew                                                   new
#define aznewex(_Name)                                          new

/// azmalloc(size)
#define azmalloc_1(_1)                                          static_cast<void*>(AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1))
/// azmalloc(size,alignment)
#define azmalloc_2(_1, _2)                                      static_cast<void*>(AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, _2))
/// azmalloc(size,alignment,Allocator)
#define azmalloc_3(_1, _2, _3)                                  static_cast<void*>(AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2))

/// azcreate(class)
#define azcreate_1(_1)                                          new(azmalloc_3(sizeof(_1), alignof( _1 ), AZ::SystemAllocator)) _1()
/// azcreate(class,params)
#define azcreate_2(_1, _2)                                      new(azmalloc_3(sizeof(_1), alignof( _1 ), AZ::SystemAllocator)) _1 _2
/// azcreate(class,params,Allocator)
#define azcreate_3(_1, _2, _3)                                  new(azmalloc_3(sizeof(_1), alignof( _1 ), _3)) _1 _2

/**
* azmalloc is equivalent to ::malloc(...). It should be used with corresponding azfree call.
* macro signature: azmalloc(size_t byteSize, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator)
*/
#define azmalloc(...)       AZ_MACRO_SPECIALIZE(azmalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azcalloc(size)
#define azcalloc_1(_1)                  ::memset(azmalloc_1(_1), 0, _1)
/// azcalloc(size, alignment)
#define azcalloc_2(_1, _2)              ::memset(azmalloc_2(_1, _2), 0, _1);
/// azcalloc(size, alignment, Allocator)
#define azcalloc_3(_1, _2, _3)          ::memset(azmalloc_3(_1, _2, _3), 0, _1);

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
 * macro signature: azcreate(ClassName, CtorParams = (), AllocatorType = AZ::SystemAllocator)
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

#define azdestroy_1(_1)         do { [](auto* ptr){ using T = AZStd::remove_cvref_t<decltype(*ptr)>; ptr->~T(); }(_1); azfree_1(_1); } while (0)
#define azdestroy_2(_1, _2)      do { [](auto* ptr){ using T = AZStd::remove_cvref_t<decltype(*ptr)>; ptr->~T(); }(_1); azfree_2(_1, _2); } while (0)
#define azdestroy_3(_1, _2, _3)     do { [](auto* ptr){ using T = AZStd::remove_cvref_t<decltype(*ptr)>; ptr->~T(); }(reinterpret_cast<_3*>(_1)); azfree_4(_1, _2, sizeof(_3), alignof( _3 )); } while (0)

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
        AZ_Assert(size == sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));                    \
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
        AZ_Assert(size == sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));           \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, alignof( _Class ));                                                                                        \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE void  operator delete(void* p, std::size_t size) {    /* default operator delete */                                                                             \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, alignof( _Class )); }                                                                               \
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
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(sizeof(_Class), alignof( _Class ));                                                                              \
    }                                                                                                                                                                               \
    AZ_FORCE_INLINE static void AZ_CLASS_ALLOCATOR_DeAllocate(void* object) {                                                                                                       \
        AZ::AllocatorInstance< _Allocator >::Get().deallocate(object, sizeof(_Class), alignof( _Class ));                                                                           \
    }                                                                                                                                                                               \
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
    _AZ_CLASS_ALLOCATOR_DECL_ALIGNED_NEW

#if __cpp_aligned_new
// Defines the C++17 aligned_new operator new/operator delete overloads
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs) \
     AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders  \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator new(std::size_t size, std::align_val_t align) { \
        AZ_Assert(size == sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class)); \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, static_cast<std::size_t>(align)); \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { \
        return operator new(size, align); \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator new[]([[maybe_unused]] std::size_t, [[maybe_unused]] std::align_val_t) { \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n" \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n" \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!"); \
        return AZ_INVALID_POINTER; \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { \
        return operator new[](size, align); \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator delete(void* p, std::size_t size, std::align_val_t align) noexcept { \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, static_cast<std::size_t>(align)); } \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator delete[]([[maybe_unused]] void* p, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align) noexcept { \
        AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n" \
                          "new[] inserts a header (platform dependent) to keep track of the array size!\n" \
                          "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!"); \
    }
#else
#define _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs)
#endif
#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs) \
    /* ========== standard operator new/delete ========== */ \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator new(std::size_t size) \
    { \
        AZ_Assert(size == sizeof(_Class), "Size mismatch! Did you forget to declare the macro in derived class? Size: %d sizeof(_Class): %d", size, sizeof(_Class)); \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(size, alignof( _Class )); \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::operator delete(void* p, std::size_t size) { \
        if (p) { AZ::AllocatorInstance< _Allocator >::Get().deallocate(p, size, alignof( _Class )); } \
    } \
    /* ========== AZ_CLASS_ALLOCATOR API ========== */ \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void* _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::AZ_CLASS_ALLOCATOR_Allocate() { \
        return AZ::AllocatorInstance< _Allocator >::Get().allocate(sizeof(_Class), alignof( _Class )); \
    } \
    AZ_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    _FuncSpecs void _Class AZ_TEMPLATE_ARGUMENT_LIST _TemplatePlaceholders ::AZ_CLASS_ALLOCATOR_DeAllocate(void* object) { \
        AZ::AllocatorInstance< _Allocator >::Get().deallocate(object, sizeof(_Class), alignof( _Class )); \
    } \
    _AZ_CLASS_ALLOCATOR_IMPL_ALIGNED_NEW(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs)

#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL_HELPER(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs) \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _TemplatePlaceholders, _FuncSpecs)

#define AZ_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, ...) \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL( \
        AZ_USE_FIRST_ARG(AZ_UNWRAP(_Class)), \
        _Allocator, \
        AZ_WRAP(AZ_SKIP_FIRST_ARG(AZ_UNWRAP(_Class))), \
        ) \

#define AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, ...) \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL( \
        AZ_USE_FIRST_ARG(AZ_UNWRAP(_Class)), \
        _Allocator, \
        AZ_WRAP(AZ_SKIP_FIRST_ARG(AZ_UNWRAP(_Class))),\
        template<>) \

//! This performs the same transformation as the AZ_CLASS_ALLOCATOR_IMPL macro
//! with the addition that it adds the function specifier of `inline` in front
//! of each function definition
//! This macro is suitable for use with template classes member definitions in header or .inl files
#define AZ_CLASS_ALLOCATOR_IMPL_INLINE(_Class, _Allocator) \
    AZ_CLASS_ALLOCATOR_IMPL_INTERNAL( \
        AZ_USE_FIRST_ARG(AZ_UNWRAP(_Class)), \
        _Allocator, \
        AZ_WRAP(AZ_SKIP_FIRST_ARG(AZ_UNWRAP(_Class))), \
        inline) \

//////////////////////////////////////////////////////////////////////////
// new operator overloads

// you can redefine this macro to whatever suits you.
#ifndef AZCORE_GLOBAL_NEW_ALIGNMENT
#   define AZCORE_GLOBAL_NEW_ALIGNMENT 16
#endif


#define AZ_PAGE_SIZE AZ_TRAIT_OS_DEFAULT_PAGE_SIZE
#define AZ_DEFAULT_ALIGNMENT (sizeof(void*))

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#define AZ_CORE_MAX_ALLOCATOR_SIZE AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE

namespace AZ
{
    /**
    * Helper class to determine if type T specializes AZ Allocators,
    * so we can safely call aznew on it. -  HasAZClassAllocator<ClassType>...
    * In order to avoid reduce include dependencies, constructs such as
    * std::integral_constant, std::enable_if_t and std::is_same_v are not being used
    * This allows this header to be used without the need to include <type_traits>
    */
    template<class T, class = void>
    struct HasAZClassAllocator
    {
        using type = HasAZClassAllocator;
        static constexpr bool value = false;
        constexpr operator bool() const
        {
            return value;
        }
        constexpr bool operator()() const
        {
            return value;
        }
    };

    template <class T>
    inline constexpr bool HasAZClassAllocator_v = AZ::HasAZClassAllocator<T>::value;

    template<class T>
    struct HasAZClassAllocator<T, decltype(T::AZ_CLASS_ALLOCATOR_Allocate(), void())>
    {
        using type = HasAZClassAllocator;
        static constexpr bool value = true;
        constexpr operator bool() const
        {
            return value;
        }
        constexpr bool operator()() const
        {
            return value;
        }
    };
}
