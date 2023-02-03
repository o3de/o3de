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
        //! Stores if the TypeInfoObject represents an object pointer type
        //! NOTE: This should not be set for a member pointer(i.e T::*)
        is_pointer = 0b1000,
        //! Stores if the TypeInfoObject repersents a template
        is_template = 0b10000
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::TypeTraits);

    //! An identifier which used to reference a Class Template
    using TemplateId = AZ::Uuid;

    //! Structure used to contain compile time information
    //! associated with a Class Template
    struct TemplateInfoObject
    {
        constexpr TemplateId GetTemplateId() const
        {
            return m_templateId;
        }

        TemplateId m_templateId;
    };

    //! Stores fields needed type info object
    //! NOTE:This class doesn't have to be used for the GetAzTypeInfo function below
    //! The only requirement is that a type that supports the following function exist
    //! const char* GetName();
    //! TypeId GetCanonicalTypeId();
    //! TypeTraits GetTraits()
    //! size_t GetSize()
    struct TypeInfoObject
    {
        // Returns a fixed_string const reference instead of a const char*
        // This makes it safer to use the GetName function to be used with an xvalue TypeInfoObject
        // i.e `auto typeName = TypeInfoObject{}.GetName();` will return an `AZStd::fixed_string`
        // instead of a `const char*` to a dangling TypeInfoObject
        constexpr const TypeNameString& GetName() const
        {
            return m_name;
        }

        constexpr AZ::TypeId GetCanonicalTypeId() const
        {
            return m_canonicalTypeId;
        }
        constexpr AZ::TypeId GetPointeeTypeId() const
        {
            return m_pointeeTypeId;
        }
        constexpr AZ::TypeId GetPointerTypeId() const
        {
            return m_pointerTypeId;
        }
        constexpr AZ::TypeId GetTemplateId() const
        {
            return m_templateId;
        }

        constexpr TypeTraits GetTraits() const
        {
            return m_typeTraits;
        }

        constexpr size_t GetSize() const
        {
            return m_typeSize;
        }

        //! Name identifier is for storing the only the class name
        //! or class template name
        //! The names of template parameters are not stored.
        //! and are composed via any template argument type info when GetName() is called
        TypeNameString m_name;

        //! Stores the exact TypeId to use for a specific type.
        //! This Id takes into account the qualifiers and templates used to make the "complete" type
        //! The following conditions for two types hold, using AZStd::vector as an example
        //! `AZStd::vector<int>` TypeId == `AZStd::vector<int>` TypeId
        //! `AZStd::vector<int>` TypeId != `AZStd::vector<int*>` TypeId
        //! `AZStd::vector<int>` TypeId != `AZStd::vector<int>*` TypeId
        //! `AZStd::vector<int>` TypeId != `AZStd::vector<float>` TypeId
        AZ::TypeId m_canonicalTypeId;

        //! Stores the TypeId for a specific type without the pointer typeid from it
        //! ex. If the Canonical Type represents `AZStd::vector<int*>*`,
        //! then the Pointee TypeId is `AZStd::vector<int*>`
        //!
        //! If the TypeInfo isn't a pointer, then a value of Null Uuid is the default
        AZ::TypeId m_pointeeTypeId;

        //! Stores the identifier that represents the canonical type with a pointer added to it.
        //! ex. If the Canonical TypeId represents `AZStd::vector<int*>`,
        //! then the Pointer TypeId is `AZStd::vector<int*>*`
        AZ::TypeId m_pointerTypeId;

        //! Store the identifier if the TypeInfo is a Class Template
        //! ex. `AZStd::vector`, or `AZStd::unordered_map`, etc...
        //! Even if two types have different canonical Type Ids, as long as the template
        //! Ids are the same, this value will be the same
        //! `AZStd::vector<int>` TemplateId == `AZStd::vector<float>` TemplateId
        //! `AZStd::vector<int>` TemplateId == `AZStd::vector<int*>` TemplateId
        //! `AZStd::vector<int>` TemplateId != `AZStd::unordered_set<int>` TemplateId
        //!
        //! If the TypeInfo isn't a template, then a value of Null Uuid is the default
        TemplateId m_templateId;

        AZ::TypeTraits m_typeTraits{};
        size_t m_typeSize{};
    };
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
    // calls of GetAzTypeInfo and GetAzTemplateInfo
    // This allow Argument Dependent Lookup(ADL) to find overloads of these functions in the AZ namespace
    // Therefore allowing for existing overloads for types not declared in the AZ namespace to be picked up
    // Example:
    // AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(AZStd::allocator, "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}", "allocator");
    // The AZStd::allocator class is declared in the AZStd namespace
    // Therefore the AZ_TYPE_INFO_SPECIALIZE_WITH_NAME up above would have to be put in the AZStd namespace to be
    // picked up by ADL normally.
    // But by adding an argument within the AZ namespace to the call of `GetAzTypeInfo`, the
    // AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(AZStd::allocator...) macro can also be placed in the AZ namespace
    struct Adl {};
}

namespace AZ
{
    // AzTypeInfo specialization helper for non intrusive TypeInfo
#define AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(_ClassName, _DisplayName, _ClassUuid) \
    constexpr AZ::TypeInfoObject GetAzTypeInfo(AZ::Adl, AZStd::type_identity<_ClassName>) \
    { \
        using ClassType = _ClassName; \
        constexpr AZStd::string_view name_{_DisplayName }; \
        AZ::TypeTraits typeTraits{}; \
        typeTraits |= AZStd::is_signed_v<AZStd::RemoveEnumT<ClassType>> ? AZ::TypeTraits::is_signed : AZ::TypeTraits{}; \
        typeTraits |= AZStd::is_unsigned_v<AZStd::RemoveEnumT<ClassType>> ? AZ::TypeTraits::is_unsigned : AZ::TypeTraits{}; \
        typeTraits |= AZStd::is_enum_v<ClassType> ? AZ::TypeTraits::is_enum : AZ::TypeTraits{}; \
        \
        AZ::TypeInfoObject typeInfoObject; \
        typeInfoObject.m_name = !name_.empty() ? name_.data() : #_ClassName; \
        typeInfoObject.m_canonicalTypeId = AZ::TypeId{ _ClassUuid }; \
        typeInfoObject.m_pointerTypeId = typeInfoObject.m_canonicalTypeId + AZ::Internal::PointerId_v; \
        typeInfoObject.m_typeTraits = typeTraits; \
        typeInfoObject.m_typeSize = AZ::Internal::TypeInfoSizeof<ClassType>; \
        return typeInfoObject; \
    }

    //! Add GetAzTypeInfo overloads for built-in types
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(char, "char", "{3AB0037F-AF8D-48ce-BCA0-A170D18B2C03}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(AZ::s8, "AZ::s8", "{58422C0E-1E47-4854-98E6-34098F6FE12D}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(short, "short", "{B8A56D56-A10D-4dce-9F63-405EE243DD3C}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(int, "int", "{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(long, "long", "{8F24B9AD-7C51-46cf-B2F8-277356957325}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(AZ::s64, "AZ::s64", "{70D8A282-A1EA-462d-9D04-51EDE81FAC2F}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(unsigned char, "unsigned char", "{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(unsigned short, "unsigned short", "{ECA0B403-C4F8-4b86-95FC-81688D046E40}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(unsigned int, "unsigned int", "{43DA906B-7DEF-4ca8-9790-854106D3F983}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(unsigned long, "unsigned long", "{5EC2D6F7-6859-400f-9215-C106F5B10E53}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(AZ::u64, "AZ::u64", "{D6597933-47CD-4fc8-B911-63F3E2B0993A}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(float, "float", "{EA2C3E90-AFBE-44d4-A90D-FAAF79BAF93D}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(double, "double", "{110C4B14-11A8-4e9d-8638-5051013A56AC}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(bool, "bool", "{A0CA880C-AFE4-43cb-926C-59AC48496112}");
    AZ_TYPE_INFO_INTERNAL_SPECIALIZE_WITH_NAME(void, "void", "{C0F1AFAD-5CB3-450E-B0F5-ADB5D46B0E22}");
}

namespace AZ
{
    // Deleted GetAzTypeInfo(...) function to provide the symbol for a function called GetAzTypeInfo
    // whenever the provided type does not have an overload
    void GetAzTypeInfo(...) = delete;

    // Base variable template which is false whenever a type doesn't have a GetAzTypeInfo
    // function or functor not callable in the current scope
    template <class T, class = void>
    constexpr bool HasUnqualifiedGetAzTypeInfo_v = false;

    // Check if there is a function available using ADL called R GetAzTypeInfo(AZ::Adl, type_identity<T>)
    // This is used as a customization point to allow code to opt in to AzTypeInfo using non-member functions
    // This specialization must be after the GetAzTypeInfo function declarations for built-in types
    // as the overload set for non-class types are formed as soon as the specialization template is defined
    // For class types, they can implement the GetAzTypeInfo function a later point in code
    // and argument dependent lookup will pick them up
    template <class T>
    constexpr bool HasUnqualifiedGetAzTypeInfo_v<T, AZStd::enable_if_t<
        !AZStd::is_void_v<decltype(GetAzTypeInfo(AZ::Adl{}, AZStd::type_identity<T>{})) >> > = true;

    // Base variable template which is false whenever a type doesn't have a GetAzTypeInfo
    // member function
    template <class T, class = void>
    inline constexpr bool HasMemberGetAzTypeInfo_v = false;
    // Check if thereis a function available called `R <type>::GetAzTypeInfo(AZStd::type_identity<T>)` and has a return
    // value that isn't void
    // This is used as a customization point to allow code to opt in to AzTypeInfo using member functions
    // NOTE: The AZStd::type_identity parameter is to make sure that typeinfo is only retrieved from the exact class
    // and not from any derived class
    // i.e
    //  ```
    // struct Component{};
    // struct MeshComponent : Component {};
    // ```
    // Requesting Calling `MeshComponent::GetAzTypeInfo` should NOT return the type info for the base class
    template <class T>
    inline constexpr bool HasMemberGetAzTypeInfo_v<T, AZStd::enable_if_t<
        !AZStd::is_void_v<decltype(T::GetAzTypeInfo(AZStd::type_identity<T>{}))>>> = true;

    template <class T>
    inline constexpr bool HasGetAzTypeInfo_v = HasMemberGetAzTypeInfo_v<T> || HasUnqualifiedGetAzTypeInfo_v<T>;

    template<class T>
    static constexpr AZ::TypeInfoObject CreateTypeInfoObject()
    {
        // Get the pointed to type with its qualifiers intact
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

        if constexpr (HasGetAzTypeInfo_v<ValueTypeNoQualifiers>)
        {
            AZ::TypeInfoObject typeInfoObject{};
            if constexpr (HasMemberGetAzTypeInfo_v<ValueTypeNoQualifiers>)
            {
                typeInfoObject = ValueTypeNoQualifiers::GetAzTypeInfo(AZStd::type_identity<ValueTypeNoQualifiers>{});
            }
            else if constexpr (HasUnqualifiedGetAzTypeInfo_v<ValueTypeNoQualifiers>)
            {
                typeInfoObject = GetAzTypeInfo(Adl{}, AZStd::type_identity<ValueTypeNoQualifiers>{});
            }

            // First calculate the type qualifiers of the pointee type
            if constexpr (AZStd::is_pointer_v<TypeNoQualifiers>)
            {
                if constexpr (AZStd::is_const_v<ValueTypeNoReference>)
                {
                    // Appending the string if " const"  because it can done unambiguously instead of prepending "const "
                    // Plus it has the benefit of not needing to make a new instance of a TypeInfoString for the addition
                    typeInfoObject.m_name += " const";
                }

                // Append '&' if the ValueType with qualifiers is an lvalue
                // or append '&&' if the ValueType with qualifiers is an rvalue
                if constexpr (AZStd::is_lvalue_reference_v<ValueType>)
                {
                    typeInfoObject.m_name += "&";
                }
                else if constexpr (AZStd::is_rvalue_reference_v<ValueType>)
                {
                    typeInfoObject.m_name += "&&";
                }

                // Finally append the pointer '*' to the name
                typeInfoObject.m_name += "*";

                // For pointer types update the type id and type traits here

                //! The pointee TypeId for T* is canonical TypeId for T
                //! Needs to be stored in a local variable
                //! until the canonical TypeId has been updated
                AZ::TypeId pointeeTypeId = typeInfoObject.m_canonicalTypeId;
                //! The canonical TypeId for T* is the pointer TypeId for T
                typeInfoObject.m_canonicalTypeId = pointeeTypeId + AZ::Internal::PointerId_v;
                //! Now the pointee TypeId can be updated with the canonical TypeId for T
                typeInfoObject.m_pointeeTypeId = pointeeTypeId;
                //! The pointer TypeId for T* is it's canonical TypeId combined the Pointer Id constant
                typeInfoObject.m_pointerTypeId = typeInfoObject.m_canonicalTypeId + AZ::Internal::PointerId_v;

                // Update the type traits to indicate the type is a pointer
                typeInfoObject.m_typeTraits |= AZ::TypeTraits::is_pointer;
                // Use the sizeof of the supplied type T with no qualifiers
                typeInfoObject.m_typeSize = sizeof(TypeNoQualifiers*);

            }

            if constexpr (AZStd::is_const_v<TypeNoReference>)
            {
                typeInfoObject.m_name += " const";
            }

            if constexpr (AZStd::is_lvalue_reference_v<T>)
            {
                typeInfoObject.m_name += "&";
            }
            else if constexpr (AZStd::is_rvalue_reference_v<T>)
            {
                typeInfoObject.m_name += "&&";
            }

            return typeInfoObject;
        }
        else
        {
            static_assert(HasGetAzTypeInfo_v<ValueTypeNoQualifiers>,
                "No unqualified or static member GetAzTypeInfo method is available. "
                "AZ_TYPE_INFO or AZ_RTTI can be used in a class/struct, or use AZ_TYPE_INFO_SPECIALIZE() externally. "
                "Make sure to include the header containing this information (usually your class header).");
            return AZ::TypeInfoObject{};
        }
    }

    /**
    * Since O3DE fully support cross shared libraries (DLL) operation, it does not rely on typeid, static templates, etc.
    * to generate the same result in different compilations. A unique ID for each type is required.
    * By default the code will try to access to a static GetAzTypeInfo functions inside a class the type info metadata.
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
        static constexpr AZ::TypeId Uuid()
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

        static constexpr AZ::TypeId GetCanonicalTypeId()
        {
            return s_typeInfoObject.GetCanonicalTypeId();
        }

        static constexpr AZ::TypeId GetPointeeTypeId()
        {
            return s_typeInfoObject.GetPointeeTypeId();
        }

        static constexpr AZ::TypeId GetTemplateId()
        {
            return s_typeInfoObject.GetTemplateId();
        }

        static constexpr AZ::TypeId GetPointerTypeId()
        {
            return s_typeInfoObject.GetPointerTypeId();
        }

        static constexpr const char* Name()
        {
            return s_typeInfoObject.GetName().c_str();
        }

        static constexpr TypeTraits GetTypeTraits()
        {
            return s_typeInfoObject.GetTraits();
        }

        static constexpr size_t Size()
        {
            return s_typeInfoObject.GetSize();
        }

    protected:
        static constexpr AZ::TypeInfoObject s_typeInfoObject = CreateTypeInfoObject<T>();
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

#define AZ_TYPE_INFO_INTERNAL_CLASS_GETTERS(_ClassName) \
    static constexpr auto TYPEINFO_Name() \
    { \
        constexpr const char* typeName = AZ::AzTypeInfo<_ClassName>::Name(); \
        return typeName; \
    } \
    static constexpr auto TYPEINFO_Uuid() \
    { \
        constexpr AZ::TypeId typeId = AZ::AzTypeInfo<_ClassName>::Uuid(); \
        return typeId; \
    }

#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_0(_ClassName, _DisplayName) static_assert(false, "A ClassName and ClassUUID must be provided")
#define AZ_TYPE_INFO_INTERNAL_WITH_NAME_1(_ClassName, _DisplayName, _ClassUuid) \
    static constexpr AZ::TypeInfoObject GetAzTypeInfo(AZStd::type_identity<_ClassName>) \
    { \
        using ClassType = _ClassName; \
        \
        AZ::TypeInfoObject typeInfoObject; \
        typeInfoObject.m_name = !AZStd::string_view(_DisplayName).empty() ? #_ClassName : _DisplayName; \
        typeInfoObject.m_canonicalTypeId = AZ::TypeId{ _ClassUuid }; \
        typeInfoObject.m_pointerTypeId = typeInfoObject.m_canonicalTypeId + AZ::Internal::PointerId_v; \
        typeInfoObject.m_typeTraits = AZ::TypeTraits{}; \
        typeInfoObject.m_typeSize = sizeof(ClassType); \
        return typeInfoObject; \
    } \
    AZ_TYPE_INFO_INTERNAL_CLASS_GETTERS(_ClassName);



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
#define AZ_TYPE_INFO_WITH_NAME(_Identifier, _DisplayName, ...) AZ_TYPE_INFO_MACRO_CALL(AZ_TYPE_INFO_INTERNAL_WITH_NAME_, AZ_VA_NUM_ARGS(__VA_ARGS__), (_Identifier, _DisplayName, __VA_ARGS__))
#define AZ_TYPE_INFO(_Identifier, ...) AZ_TYPE_INFO_WITH_NAME(_Identifier, #_Identifier, __VA_ARGS__)

// * NOTE: For a Class template, the "first" argument to the the following the the first argument must be a
// `template-name` identifier as according to the C++ standard: http://eel.is/c++draft/temp.names#nt:template-name
// It should NOT be a `simple-template-id` whose grammer is shown here http://eel.is/c++draft/temp.names#nt:simple-template-id
// For example tto provide AZ_TYPE_INFO support for a type such as
/* template<class R, class C, class... Args>
   class AttributeMemberFunction<R(C::*)(Args...)>

   The following use of the AZ_TYPE_INFO_WITH_NAME macro is the correct way to add support:
   AZ_TYPE_INFO_WITH_NAME(AZ::AttributeMemberFunction, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_VARARGS);
   Notice that the first argument does not have any angle bracket nor template parameters.

   Next is an incorrect usage of AZ_TYPE_INFO_WITH_NAME macro
   AZ_TYPE_INFO_WITH_NAME(AZ::AttributeMemberFunction<R(C::*)(Args...)>, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_TYPENAME, AZ_TYPE_INFO_CLASS_VARARGS)
*/
// The problem with using the `simple-template-id` of "AZ::AttributeMemberFunction<R(C::*)(Args...)>" identifier is that name gets stringized exactly as specified
// When a name of an actual specialization of `AZ::AttributeMemberFunction` is requested it would then append the resolved names to the stringized "template name".
// For example if a class member function with a signature of `void (Entity::*)(Component*)`, the resulting name would be
// "AZ::AttributeMemberFunction<R(C::*)(Args...)><void (Entity::*)(Component*)>"
