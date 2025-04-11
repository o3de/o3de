/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/typetraits/is_enum.h>

namespace AZ
{
    /**
    * Used to reference unique types by their unique id.
    */
    using TypeId = AZ::Uuid;

    // Storage type for the type name
    using TypeNameString = AZStd::fixed_string<512>;

    /**
    * Type Trait structure for tracking of C++ type trait data
    * within the SerializeContext ClassData structure;
    */
    enum class TypeTraits : AZ::u64
    {
        //! Stores the result of the std::is_signed check for a registered type
        is_signed = 0b1,
        //! Stores the result of the std::is_unsigned check for a registered type
        //! NOTE: While this might seem to be redundant due to is_signed, it is NOT
        //! Only numeric types have a concept of signedness
        is_unsigned = 0b10,
        //! Stores the result of the std::is_enum check
        is_enum = 0b100,
        //! Stores if the type represents an object pointer type
        //! NOTE: This should not be set for a member pointer(i.e T::*)
        is_pointer = 0b1000,
        //! Stores if the type represents a template
        is_template = 0b10000
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::TypeTraits);

    //! An identifier which used to reference a Class Template
    using TemplateId = AZ::Uuid;
} // namespace AZ

namespace AZ::Internal
{
    // Represents an identifier that can modify an existing typeid "T" to form a new "T*" typeid
    inline constexpr AZ::TypeId PointerId_v{ "{35C8A027-FE00-4769-AE36-6997CFFAF8AE}" };

    // Sizeof helper to deal with incomplete types and the void type
    template <class T, class = void>
    inline constexpr size_t TypeInfoSizeof = 0;

    // In this case that sizeof(T) is a compilable, the type is complete
    template <class T>
    inline constexpr size_t TypeInfoSizeof<T, decltype(sizeof(T), void())> = sizeof(T);
}

namespace AZ
{
    // Tag Type inside the AZ Namespace, which is added supplied as the first argument to function
    // calls of GetO3deTypeName, GetO3deTypeId, GetO3deClassTemplateId
    // This allow Argument Dependent Lookup(ADL) to find overloads of these functions in the AZ namespace
    // Therefore allowing for existing overloads for types not declared in the AZ namespace to be picked up
    // Example:
    // AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(AZStd::allocator, "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}", "allocator");
    // The AZStd::allocator class is declared in the AZStd namespace
    // Therefore the AZ_TYPE_INFO_SPECIALIZE_WITH_NAME up above would have to be put in the AZStd namespace to be
    // picked up by ADL normally.
    // But by adding an argument within the AZ namespace, for the `GetO3deTypeName` and `GetO3deTypeId` functions,
    // the AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(AZStd::allocator...) macro can also be placed in the AZ namespace
    struct Adl {};
}

namespace AZ
{
    // AzTypeInfo specialization helper for non-intrusive TypeInfo
    #define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_HELPER(_ClassName, _DisplayName, _ClassUuid, _Inline) \
        _Inline AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<_ClassName>) \
        { \
            constexpr AZ::TypeNameString displayName(_DisplayName); \
            return displayName; \
        } \
        _Inline AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<_ClassName>) \
        { \
            constexpr AZ::TypeId typeId{ _ClassUuid }; \
            return typeId; \
        }

    // Helper Macros which provides declaration of TypeInfo methods
    // Useful for reducing build times by moving implementation to a translation unit
    #define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(_ClassName) \
        AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<_ClassName>); \
        AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<_ClassName>);

    #define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(_ClassName, _DisplayName, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_HELPER(_ClassName, _DisplayName, _ClassUuid, constexpr)

    // Provides the definitions for the TypeInfo Methods
    // This should be used in a translation unit(.cpp) file
    #define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_IMPL(_ClassName, _DisplayName, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_HELPER(_ClassName, _DisplayName, _ClassUuid,)

    // Provides the definitions for the TypeInfo methods with the addition of the inline specifier
    // Useful for defining these functions in a header or .inl file when dealing with a class template
    #define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_IMPL_INLINE(_ClassName, _DisplayName, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_HELPER(_ClassName, _DisplayName, _ClassUuid, inline)

    //! Add GetO3deTypeName and GetO3deTypeId overloads for built-in types
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(char);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(AZ::s8);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(short);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(int);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(long);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(AZ::s64);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(unsigned char);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(unsigned short);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(unsigned int);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(unsigned long);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(AZ::u64);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(float);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(double);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(bool);
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(void);

    /**
    * Use this macro outside a class to allow it to be identified across modules and serialized (in different contexts).
    * The expected input is the class and the assigned uuid as a string or an instance of a uuid.
    * Note that the AZ_TYPE_INFO_SPECIALIZE does NOT need to be declared in "namespace AZ".
    * It can be declared outside the namespace as mechanism for adding TypeInfo uses function overloading
    * instead of template specialization
    * Example:
    *   class MyClass
    *   {
    *   public:
    *       ...
    *   };
    *
    *   AZ_TYPE_INFO_SPECIALIZE(MyClass, "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}");
    */
    #define AZ_TYPE_INFO_SPECIALIZE(_ClassType, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(_ClassType, #_ClassType, _ClassUuid)

    // Adds support for specifying a different display name for the Class type being specialized for TypeInfo
    // This is useful when wanting to remove a namespace from the TypeInfo name such as when reflecting to scripting
    // i.e AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(AZ::Metrics::MyClass, "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}", MyClass)
    #define AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(_ClassType, _ClassUuid, _DisplayName) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(_ClassType, _DisplayName, _ClassUuid)

    // Adds declaration TypeInfo function overloads for a type(class, enum or fundamental)
    #define AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_DECL(_ClassName) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_DECL(_ClassName)

    // Adds function definition for TypeInfo functions
    // NOTE: This needs to be in the same namespace as the declaration
    // The functions do not have the inline attached, so this macro is only suitable for use
    // in a single translation unit
    #define AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL(_ClassName, _DisplayName, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_IMPL(_ClassName, _DisplayName, _ClassUuid)

    // Adds inline function definition for TypeInfo functions
    // This macro can be used in a header or inline file and is suitable for use with class template types
    #define AZ_TYPE_INFO_SPECIALIZE_WITH_NAME_IMPL_INLINE(_ClassName, _DisplayName, _ClassUuid) \
        AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME_IMPL_INLINE(_ClassName, _DisplayName, _ClassUuid)

}

namespace AZ
{
    //! TypeName

    // Unimplemented GetO3deTypeName(...) function to provide the symbol for a function called GetO3deTypeName
    // whenever the provided type does not have an overload
    void GetO3deTypeName(...) = delete;

    // Base variable template which is false whenever a type doesn't have a GetO3deTypeName
    // function or functor not callable in the current scope
    template <class T, class = void>
    constexpr bool HasUnqualifiedGetO3deTypeName_v = false;

    // Check if there is a function available using ADL called R GetO3deTypeName(AZ::Adl, type_identity<T>)
    // This is used as a customization point to allow code to opt in to AzTypeInfo using non-member functions
    // This specialization must be after the GetO3deTypeName function declarations for built-in types
    // as the overload set for non-class types are formed as soon as the specialization template is defined
    // For class types, they can implement the GetO3deTypeName function a later point in code
    // and argument dependent lookup will pick them up
    template <class T>
    constexpr bool HasUnqualifiedGetO3deTypeName_v<T, AZStd::enable_if_t<
        AZStd::is_same_v<decltype(GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<T>{})), AZ::TypeNameString>> > = true;

    // True if the class has an overload available via ordinary name nookup or ADL
    template <class T>
    inline constexpr bool HasGetO3deTypeName_v = HasUnqualifiedGetO3deTypeName_v<T>;
}

namespace AZ
{
    //! Type Uuid

    // Unimplemented GetO3deTypeId(...) function to provide the symbol for a function called GetO3deTypeId
    // whenever the provided type does not have an overload
    void GetO3deTypeId(...) = delete;

    // Base variable template which is false whenever a type doesn't have a GetO3deTypeId
    // function or functor not callable in the current scope
    template <class T, class = void>
    constexpr bool HasUnqualifiedGetO3deTypeId_v = false;

    // Check if there is a function available using ADL called R GetO3deTypeId(AZ::Adl, type_identity<T>)
    // This is used as a customization point to allow code to opt in to AzTypeInfo using non-member functions
    // This specialization must be after the GetO3deTypeId function declarations for built-in types
    // as the overload set for non-class types are formed as soon as the specialization template is defined
    // For class types, they can implement the GetO3deTypeId function a later point in code
    // and argument dependent lookup will pick them up
    template <class T>
    constexpr bool HasUnqualifiedGetO3deTypeId_v<T, AZStd::enable_if_t<
        AZStd::is_same_v<decltype(GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<T>{})), AZ::TypeId>> > = true;

    // True if the class has an overload available via ordinary name nookup or ADL
    template <class T>
    inline constexpr bool HasGetO3deTypeId_v = HasUnqualifiedGetO3deTypeId_v<T>;
}

namespace AZ
{
    //! Template Uuid

    //! Unimplemented  of GetO3deClassTemplateId function
    //! which provides a symbolfor name lookup when GetAzTemplateId() function overload is not available
    //! The return type is void to distinguish it from actual overloads which return AZ::Uuid
    //! This function should be overloaded for class template (not types)
    //! using the signature of `AZ::TemplateId GetO3deClassTemplateId(decltype(GetTemplateIdentity<ActualType>()))`
    //! For example to overload GetO3deClassTemplateId for AZStd::vector the following signature should be used
    //! `AZ::TemplateId GetO3deClassTemplateId(AZ::Adl{}, decltype(GetTemplateIdentity<AZStd::vector>()))`
    void GetO3deClassTemplateId(...);

    template <class T, class = void>
    inline constexpr bool HasUnqualifiedGetO3deClassTemplateId_v = false;

    // Specialization for when an unqualified call to GetO3deClassTemplateId is available in the current scope
    // via unqualified name lookup and ADL
    template <class T>
    inline constexpr bool HasUnqualifiedGetO3deClassTemplateId_v<T, AZStd::enable_if_t <
        AZStd::is_same_v<decltype(GetO3deClassTemplateId(AZ::Adl{}, AZStd::type_identity<T>{})), AZ::TemplateId > >> = true;

    // True if the class has an overload as a member function or an overload available in its namespace.
    template <class T>
    inline constexpr bool HasGetO3deClassTemplateId_v = HasUnqualifiedGetO3deClassTemplateId_v<T>;
}

namespace AZ
{
    /**
    * Since O3DE fully support cross shared libraries (DLL) operation, it does not rely on typeid, static templates, etc.
    * to generate the same result in different compilations. A unique ID for each type is required.
    * By default the code will try to access to a static GetO3deTypeName/GetO3deTypeId functions inside a class the type info metadata.
    * For types when intrusive is not an option(fundamental types such as int, float or enums), the class will need to specialize the AzTypeInfo.
    */
    template<class T>
    struct AzTypeInfo
    {
        //! Used for aggregation of TypeIds when part of a template argument
        //! Returns the canonical type id of the actual type
        //! when a value type such as int or AZStd::string
        //!
        //! When type T is a pointer
        //! Calls the PointeeTypeId function to retrieve
        //! the TypeId of for type T instead of T*
        //! This is different from the other AzTypeInfo T types
        //! Required for maintaining backwards compatibility
        //! with code that expects AzTypeInfo<int*>::Uuid() == AzTypeInfo<int>::Uuid()
        static AZ::TypeId Uuid()
        {
            if constexpr (!AZStd::is_pointer_v<AZStd::remove_cvref_t<T>>)
            {
                return GetCanonicalTypeId();
            }
            else
            {
                return GetPointeeTypeId();
            }
        }

        static AZ::TypeId GetCanonicalTypeId()
        {
            // Get the pointee to type with its qualifiers intact
            // ex.
            // const int* const& -> const int*
            //         ^                ^
            //        raw     ->    remove_cvref<T>
            //
            // const int*          -> const int
            //      ^                    ^
            // remove_cvref<T> -> remove_pointer_t<remove_cvref_t<T>>
            //
            // const int                                                     -> int
            //      ^                                                            ^
            // remove_pointer_t<remove_cvref_t<T>>> ->   remove_cvref_t<remove_pointer_t<remove_cvref_t<T>>>
            using TypeNoQualifiers = AZStd::remove_cvref_t<T>;

            using ValueType = AZStd::conditional_t<AZStd::is_pointer_v<TypeNoQualifiers>,
                AZStd::remove_pointer_t<TypeNoQualifiers>,
                T>;
            using ValueTypeNoQualifiers = AZStd::remove_cvref_t<ValueType>;

            if constexpr (HasGetO3deTypeId_v<ValueTypeNoQualifiers>)
            {
                static AZ::TypeId s_canonicalTypeId;
                // Calculate the uuid only once
                if (s_canonicalTypeId.IsNull())
                {
                    s_canonicalTypeId = GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<ValueTypeNoQualifiers>{});

                    // If the T parameter is a pointer mixin the pointer id
                    if constexpr (AZStd::is_pointer_v<TypeNoQualifiers>)
                    {
                        //! The canonical TypeId for T* is the canonical TypeId for T + PointerId constant
                        s_canonicalTypeId += AZ::Internal::PointerId_v;
                    }
                }

                return s_canonicalTypeId;
            }
            else
            {
                static_assert(HasGetO3deTypeId_v<ValueTypeNoQualifiers>,
                    "No unqualified or static member GetO3deTypeId method is available. "
                    "AZ_TYPE_INFO or AZ_RTTI can be used in a class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                    "Make sure to include the header containing this information (usually your class header).");
                return AZ::TypeId{};
            }
        }

        static AZ::TypeId GetPointeeTypeId()
        {
            // For pointer types update the type id and type traits here
            // Get the pointee to type with its qualifiers intact
            // ex.
            // const int* const& -> const int*
            //         ^                ^
            //        raw     ->    remove_cvref<T>
            //
            // const int*          -> const int
            //      ^                    ^
            // remove_cvref<T> -> remove_pointer_t<remove_cvref_t<T>>
            //
            // const int                                                     -> int
            //      ^                                                            ^
            // remove_pointer_t<remove_cvref_t<T>>> ->   remove_cvref_t<remove_pointer_t<remove_cvref_t<T>>>
            using TypeNoQualifiers = AZStd::remove_cvref_t<T>;

            using ValueType = AZStd::conditional_t<AZStd::is_pointer_v<TypeNoQualifiers>,
                AZStd::remove_pointer_t<TypeNoQualifiers>,
                T>;
            using ValueTypeNoQualifiers = AZStd::remove_cvref_t<ValueType>;

            // First calculate the type qualifiers of the pointee type
            if constexpr (!AZStd::is_pointer_v<TypeNoQualifiers>)
            {
                // If it is not a pointer then return a default constructed TypeId
                return AZ::TypeId{};
            }
            else if constexpr (HasGetO3deTypeId_v<ValueTypeNoQualifiers>)
            {
                static const AZ::TypeId s_pointeeTypeId = GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<ValueTypeNoQualifiers>{});

                // Calculate the uuid only once
                return s_pointeeTypeId;
            }
            else
            {
                static_assert(HasGetO3deTypeId_v<ValueTypeNoQualifiers>,
                    "No unqualified or static member GetO3deTypeId method is available. "
                    "AZ_TYPE_INFO or AZ_RTTI can be used in a class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                    "Make sure to include the header containing this information (usually your class header).");
                return AZ::TypeId{};
            }
        }

        static AZ::TemplateId GetTemplateId()
        {
            using TypeNoQualifiers = AZStd::remove_cvref_t<T>;
            using ValueType = AZStd::conditional_t<AZStd::is_pointer_v<TypeNoQualifiers>,
                AZStd::remove_pointer_t<TypeNoQualifiers>,
                T>;
            using ValueTypeNoQualifiers = AZStd::remove_cvref_t<ValueType>;

            if constexpr (HasGetO3deClassTemplateId_v<ValueTypeNoQualifiers>)
            {
                return GetO3deClassTemplateId(AZ::Adl{}, AZStd::type_identity<ValueTypeNoQualifiers>{});
            }
            else
            {
                return AZ::TemplateId{};
            }
        }

        static const char* Name()
        {
            // Get the pointee to type with its qualifiers intact
            // ex.
            // const int* const& -> const int*
            //         ^                ^
            //        raw     ->    remove_cvref<T>
            //
            // const int*          -> const int
            //      ^                    ^
            // remove_cvref<T> -> remove_pointer_t<remove_cvref_t<T>>
            //
            // const int                                                     -> int
            //      ^                                                            ^
            // remove_pointer_t<remove_cvref_t<T>>> ->   remove_cvref_t<remove_pointer_t<remove_cvref_t<T>>>
            using TypeNoQualifiers = AZStd::remove_cvref_t<T>;
            // Needed for the case when checking is_const on a `const int&`
            // the reference must be removed before checking the constantness
            using TypeNoReference = AZStd::remove_reference_t<T>;

            using ValueType = AZStd::conditional_t<AZStd::is_pointer_v<TypeNoQualifiers>,
                AZStd::remove_pointer_t<TypeNoQualifiers>,
                T>;
            using ValueTypeNoQualifiers = AZStd::remove_cvref_t<ValueType>;
            // Needed for the case when checking is_const on a `const int* const&`
            using ValueTypeNoReference = AZStd::remove_reference_t<ValueType>;

            if constexpr (HasGetO3deTypeName_v<ValueTypeNoQualifiers>)
            {
                static AZ::TypeNameString s_typeNameString;
                // Calculate the type name only once on startup
                if (s_typeNameString.empty())
                {
                    AZ::TypeNameString typeName = GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<ValueTypeNoQualifiers>{});

                    // First calculate the type qualifiers of the pointee type
                    if constexpr (AZStd::is_pointer_v<TypeNoQualifiers>)
                    {
                        if constexpr (AZStd::is_const_v<ValueTypeNoReference>)
                        {
                            // Appending the string if " const"  because it can done unambiguously instead of prepending "const "
                            // Plus it has the benefit of not needing to make a new instance of a TypeInfoString for the addition
                            typeName += " const";
                        }

                        // Append '&' if the ValueType with qualifiers is an lvalue
                        // or append '&&' if the ValueType with qualifiers is an rvalue
                        if constexpr (AZStd::is_lvalue_reference_v<ValueType>)
                        {
                            typeName += "&";
                        }
                        else if constexpr (AZStd::is_rvalue_reference_v<ValueType>)
                        {
                            typeName += "&&";
                        }

                        // Finally append the pointer '*' to the name
                        typeName += "*";
                    }

                    if constexpr (AZStd::is_const_v<TypeNoReference>)
                    {
                        typeName += " const";
                    }

                    if constexpr (AZStd::is_lvalue_reference_v<T>)
                    {
                        typeName += "&";
                    }
                    else if constexpr (AZStd::is_rvalue_reference_v<T>)
                    {
                        typeName += "&&";
                    }

                    s_typeNameString = typeName;
                }

                return s_typeNameString.c_str();
            }
            else
            {
                static_assert(HasGetO3deTypeName_v<ValueTypeNoQualifiers>,
                    "No unqualified or static member GetO3deTypeName method is available. "
                    "AZ_TYPE_INFO or AZ_RTTI can be used in a class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                    "Make sure to include the header containing this information (usually your class header).");
                return "";
            }
        }

        static constexpr TypeTraits GetTypeTraits()
        {
            AZ::TypeTraits typeTraits{};
            typeTraits |= AZStd::is_signed_v<AZStd::RemoveEnumT<T>> ? AZ::TypeTraits::is_signed : AZ::TypeTraits{};
            typeTraits |= AZStd::is_unsigned_v<AZStd::RemoveEnumT<T>> ? AZ::TypeTraits::is_unsigned : AZ::TypeTraits{};
            typeTraits |= AZStd::is_enum_v<T> ? AZ::TypeTraits::is_enum : AZ::TypeTraits{};
            typeTraits |= AZStd::is_pointer_v<T> ? AZ::TypeTraits::is_pointer : AZ::TypeTraits{};

            return typeTraits;
        }

        static constexpr size_t Size()
        {
            using TypeNoQualifiers = AZStd::remove_cvref_t<T>;
            return AZ::Internal::TypeInfoSizeof<TypeNoQualifiers>;
        }
    };

    template<class T, class U>
    constexpr inline bool operator==(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.GetCanonicalTypeId() == rhs.GetCanonicalTypeId();
    }

    template<class T, class U>
    constexpr inline bool operator!=(AzTypeInfo<T> const& lhs, AzTypeInfo<U> const& rhs)
    {
        return lhs.GetCanonicalTypeId() != rhs.GetCanonicalTypeId();
    }
}

// Macro which uses the friend keyword to declare and the GetO3deTypeName and GetO3deTypeId
// functions as non-members.
// This allows avoiding needed to support checking for static member versions of these
// functions and in order to query the type name and type id
// There is one caveat though and that is friends functions are found using ordinary name lookup
// and can only be found via ADL
// Therefore given a class "Namespace::MyClass" which opts into TypeInfo
// a call such as `AZ::GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<Namespace::MyClass>{});` will not find
// the GetO3deTypeName overload
// The call to GetO3deTypeName must be made unqualified without any namespace(such AZ::)
// `GetO3deTypeName(AZ::Adl{}, AZStd::type_identity<Namespace::MyClass>{});`
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_0(_ClassName, _DisplayName, _ClassUuid) \
    friend constexpr AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<_ClassName>) \
    { \
        constexpr AZ::TypeNameString displayName(_DisplayName); \
        return displayName; \
    } \
    friend constexpr AZ::TypeId GetO3deTypeId(AZ::Adl, AZStd::type_identity<_ClassName>) \
    { \
        constexpr AZ::TypeId typeId{ _ClassUuid }; \
        return typeId; \
    } \
    static const char* TYPEINFO_Name() \
    { \
        return _DisplayName; \
    } \
    static AZ::TypeId TYPEINFO_Uuid() \
    { \
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<_ClassName>{}); \
    }

// NOTE: This the same macro logic as the `AZ_MACRO_SPECIALIZE`, but it must used instead of
// `AZ_MACRO_SPECIALIZE` macro as the C preprocessor macro expansion will not expand a macro with the same
// name again during second scan of the macro during expansion
// For example given the following set of macros
// #define AZ_EXPAND_1(X) X
// #define AZ_TYPE_INFO_INTERNAL_1(params) AZ_MACRO_SPECIALIZE(AZ_EXPAND_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))
//
// If the AZ_MACRO_SPEICALIZE is used as follows with a single argument of FOO:
// `AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_INTERNAL_, 1, (FOO))`,
// then the inner AZ_EXPAND_1 will NOT expand because the preprocessor suppresses expansion of the `AZ_MACRO_SPECIALIZE`
// with even if it is called with a different set of arguments.
// So the result of the macro is `AZ_MACRO_SPECIALIZE(AZ_EXPAND_, 1, (FOO))`, not `FOO`
// https://godbolt.org/z/GM6f7drh1
//
// See the following thread for information about the C Preprocessor works in this manner:
// https://stackoverflow.com/questions/66593868/understanding-the-behavior-of-cs-preprocessor-when-a-macro-indirectly-expands-i
// Using the name AZ_TYPE_INFO_MACRO_CALL
#   define AZ_TYPE_INFO_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#   define AZ_TYPE_INFO_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_TYPE_INFO_MACRO_CALL_II(MACRO_NAME, NPARAMS, PARAMS)
#   define AZ_TYPE_INFO_MACRO_CALL(MACRO_NAME, NPARAMS, PARAMS)       AZ_TYPE_INFO_MACRO_CALL_I(MACRO_NAME, NPARAMS, PARAMS)

/**
* Use this macro inside a class to allow it to be identified across modules and serialized (in different contexts).
* The expected input is the class and the assigned uuid as a string or an instance of a uuid.
* Example:
*   class MyClass
*   {
*   public:
*       AZ_TYPE_INFO(MyClass, "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}");
*       ...
*   };
*
* If a different name other than the stringized name of the first parameter is desired,
* then the AZ_TYPE_WITH_NAME macro should be used
*   class MyClass
*   {
*   public:
*       AZ_TYPE_INFO_WITH_NAME(MyClass, "O3DE::MyClass", "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}");
*       ...
*   };
*/
#define AZ_TYPE_INFO_WITH_NAME(_Identifier, _DisplayName, _ClassUuid, ...) \
    AZ_TYPE_INFO_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_WITH_NAME_, \
    AZ_VA_NUM_ARGS(__VA_ARGS__), \
    (_Identifier, _DisplayName, _ClassUuid AZ_VA_OPT(AZ_COMMA_SEPARATOR, __VA_ARGS__) __VA_ARGS__))
#define AZ_TYPE_INFO(_Identifier, _ClassUuid, ...) AZ_TYPE_INFO_WITH_NAME(_Identifier, #_Identifier, _ClassUuid, __VA_ARGS__)

// Helper macro that declares the TYPEINFO_Name and TYPEINFO_Uuid static members as part of a class
// This pairs with the AZ_TYPE_INFO_WITH_NAME_IMPL/AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE where an implemenation can be provided
// in a translation unit(.cpp) or a an inline(.inl) file in order to help reduce compile times
// https://godbolt.org/z/EGPvKr7xM
#define AZ_TYPE_INFO_WITH_NAME_DECL_HELPER(_ClassName, _TemplatePlaceholders) \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    friend AZ::TypeNameString GetO3deTypeName(AZ::Adl, AZStd::type_identity<_ClassName>); \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplatePlaceholders \
    friend AZ::TypeId GetO3deTypeId(AZ::Adl,AZStd::type_identity<_ClassName>); \
    static const char* TYPEINFO_Name(); \
    static AZ::TypeId TYPEINFO_Uuid();

#define AZ_TYPE_INFO_WITH_NAME_DECL(_ClassNameOrTemplateName) \
    AZ_TYPE_INFO_WITH_NAME_DECL_HELPER(AZ_USE_FIRST_ARG(AZ_UNWRAP(_ClassNameOrTemplateName)), \
    AZ_WRAP(AZ_SKIP_FIRST_ARG(AZ_UNWRAP(_ClassNameOrTemplateName))) )


// Repeat of AZ_TYPE_INFO_MACRO_CALL with a different name to allow
// calling a macro if inside of an expansion of a current AZ_TYPE_INFO_MACRO_CALL call
#define AZ_TYPE_INFO_MACRO_CALL_NEW_II(MACRO_NAME, NPARAMS, PARAMS)    MACRO_NAME##NPARAMS PARAMS
#define AZ_TYPE_INFO_MACRO_CALL_NEW_I(MACRO_NAME, NPARAMS, PARAMS)     AZ_TYPE_INFO_MACRO_CALL_NEW_II(MACRO_NAME, NPARAMS, PARAMS)
#define AZ_TYPE_INFO_MACRO_CALL_NEW(MACRO_NAME, NPARAMS, PARAMS)       AZ_TYPE_INFO_MACRO_CALL_NEW_I(MACRO_NAME, NPARAMS, PARAMS)

#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_0(...)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(...) template< AZ_TYPE_INFO_INTERNAL_TEMPLATE_TYPE_EXPANSION(__VA_ARGS__) >
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_2(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_3(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_4(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_5(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_6(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_7(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_8(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_9(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_10(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_11(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_12(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_13(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_14(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_15(...) AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_1(__VA_ARGS__)
#define AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID(...) AZ_TYPE_INFO_MACRO_CALL_NEW(AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_0(...)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(...) < AZ_TYPE_INFO_INTERNAL_TEMPLATE_ARGUMENT_EXPANSION(__VA_ARGS__) >
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_2(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_3(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_4(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_5(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_6(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_7(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_8(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_9(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_10(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_11(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_12(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_13(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_14(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_15(...) AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_1(__VA_ARGS__)
#define AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST(...) AZ_TYPE_INFO_MACRO_CALL_NEW(AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// Provides implementation for non-member GetO3deTypeName and GetO3deTypeId friend functions
// as the static member TYPEINFO_Name and TYPEINFO_Uuid functions
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_0(_ClassName, _DisplayName, _ClassUuid, _Inline, _TemplateParamsInParen) \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeNameString GetO3deTypeName( \
        AZ::Adl, AZStd::type_identity<_ClassName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen>) \
    { \
        constexpr AZStd::string_view displayName(_DisplayName); \
        constexpr AZ::TypeNameString typeName = !displayName.empty() ? AZ::TypeNameString(displayName) \
            : AZ::TypeNameString(#_ClassName); \
        return typeName; \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeId GetO3deTypeId( \
        AZ::Adl, AZStd::type_identity<_ClassName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen>) \
    { \
        constexpr AZ::TypeId typeId{ _ClassUuid }; \
        return typeId; \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline const char* _ClassName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen ::TYPEINFO_Name() \
    { \
        return _DisplayName; \
    } \
    AZ_TYPE_INFO_SIMPLE_TEMPLATE_ID _TemplateParamsInParen \
    _Inline AZ::TypeId _ClassName AZ_TYPE_INFO_TEMPLATE_ARGUMENT_LIST _TemplateParamsInParen ::TYPEINFO_Uuid() \
    { \
        return GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<_ClassName>{}); \
    }



// Helper macro which allows adding an optional function specifier such as `inline` to type info
// function definitions
// It wraps the variadic args in parenthesis as a single argument to add function definitions
// for class template types
// The AZ_UNWRAP removes any existing parenthesis before adding the new parenthesis
#define AZ_TYPE_INFO_WITH_NAME_IMPL_HELPER(_Identifier, _DisplayName, _Uuid, _Inline, ...) \
    AZ_TYPE_INFO_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_WITH_NAME_IMPL_, \
    AZ_VA_NUM_ARGS(__VA_ARGS__), \
    (\
        _Identifier, \
        _DisplayName, \
        _Uuid, \
        _Inline \
        AZ_VA_OPT(AZ_COMMA_SEPARATOR, AZ_WRAP(AZ_UNWRAP(__VA_ARGS__))) AZ_WRAP(AZ_UNWRAP(__VA_ARGS__)) \
    ))

// Adds definitions of the TypeInfo functions no additional function specifiers
#define AZ_TYPE_INFO_WITH_NAME_IMPL(_Identifier, _DisplayName, _Uuid, ...) \
    AZ_TYPE_INFO_WITH_NAME_IMPL_HELPER(_Identifier, _DisplayName, _Uuid,, __VA_ARGS__)

// Adds definitions of the TypeInfo functions with the inline specified in front of them
#define AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(_Identifier, _DisplayName, _Uuid, ...) \
    AZ_TYPE_INFO_WITH_NAME_IMPL_HELPER(_Identifier, _DisplayName, _Uuid, inline, __VA_ARGS__)


// * NOTE: For a Class template, the first argument must be a
// `template-name` identifier as according to the C++ standard: http://eel.is/c++draft/temp.names#nt:template-name
// It should NOT be a `simple-template-id` whose grammar is shown here http://eel.is/c++draft/temp.names#nt:simple-template-id
// For example tto provide AZ_TYPE_INFO support for a type such as
/* template<class R, class C, class... Args>
   class AttributeMemberFunction<R(C::*)(Args...)>

   The following use of the AZ_TYPE_INFO_WITH_NAME macro is the correct way to add support:
   AZ_TYPE_INFO_WITH_NAME(AttributeMemberFunction, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_VARARGS);
   Notice that the first argument does not have any angle bracket nor template parameters.

   Next is an incorrect usage of AZ_TYPE_INFO_WITH_NAME macro
   AZ_TYPE_INFO_WITH_NAME(AZ::AttributeMemberFunction<R(C::*)(Args...)>, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_VARARGS)
*/
// The problem with using the `simple-template-id` of "AZ::AttributeMemberFunction<R(C::*)(Args...)>" identifier is that name gets stringized exactly as specified
// When a name of an actual specialization of `AZ::AttributeMemberFunction` is requested it would then append the resolved names to the stringized "template name".
// For example if a class member function with a signature of `void (Entity::*)(Component*)`, the resulting name would be
// "AZ::AttributeMemberFunction<R(C::*)(Args...)><void (Entity::*)(Component*)>"
