/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <Tests/Serialization/Json/TestCases_Base.h>

namespace JsonSerializationTests
{
    struct SimpleClass
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_CLASS_ALLOCATOR(SimpleClass, AZ::SystemAllocator, 0);
        AZ_RTTI(SimpleClass, "{5A40E851-A748-40F3-97BA-5BB3A98CDD61}");

        SimpleClass() = default;
        explicit SimpleClass(int var1);
        SimpleClass(int var1, float var2);
        virtual ~SimpleClass() = default;

        void add_ref();
        void release();

        static const bool SupportsPartialDefaults = true;

        bool Equals(const SimpleClass& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleClass> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleClass> GetInstanceWithoutDefaults();

        bool operator<(const SimpleClass& rhs) const;
        bool operator>(const SimpleClass& rhs) const;
        bool operator<=(const SimpleClass& rhs) const;
        bool operator>=(const SimpleClass& rhs) const;
        bool operator==(const SimpleClass& rhs) const;
        bool operator!=(const SimpleClass& rhs) const;

        mutable int m_refCount = 0;
        int m_var1{ 42 };
        float m_var2{ 42.0f };
    };

    struct SimpleInheritence : public BaseClass
    {
        AZ_CLASS_ALLOCATOR(SimpleInheritence, AZ::SystemAllocator, 0);
        AZ_RTTI(SimpleInheritence, "{CD2A6797-63B4-4B4D-B3D2-9044781C1E42}", BaseClass);

        ~SimpleInheritence() override = default;

        static const bool SupportsPartialDefaults = true;

        bool Equals(const SimpleInheritence& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleInheritence> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleInheritence> GetInstanceWithoutDefaults();

        int m_var1{ 42 };
        float m_var2{ 42.0f };
    };

    struct MultipleInheritence : public BaseClass, public BaseClass2
    {
        AZ_CLASS_ALLOCATOR(MultipleInheritence, AZ::SystemAllocator, 0);
        AZ_RTTI(MultipleInheritence, "{44E30C38-CE6B-4DC6-9CBF-85B10C148B11}", BaseClass, BaseClass2);

        ~MultipleInheritence() override = default;

        static const bool SupportsPartialDefaults = true;

        bool Equals(const MultipleInheritence& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<MultipleInheritence> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<MultipleInheritence> GetInstanceWithoutDefaults();
        
        int m_var1{ 42 };
        float m_var2{ 42.0f };
    };

    struct SimpleNested
    {
        AZ_RTTI(SimpleNested, "{6BB50CDB-BD78-4CB6-94F3-A586E785D95C}");

        static const bool SupportsPartialDefaults = true;

        virtual ~SimpleNested() = default;

        bool Equals(const SimpleNested& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleNested> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleNested> GetInstanceWithoutDefaults();

        SimpleClass m_nested;
        int m_varAdditional{ 42 };
    };

    struct SimpleEnumWrapper
    {
        enum class SimpleEnumClass
        {
            Option1 = 1,
            Option2,
        };
        enum SimpleRawEnum
        {
            RawOption1 = 1,
            RawOption2,
        };
        AZ_CLASS_ALLOCATOR(SimpleEnumWrapper, AZ::SystemAllocator, 0);
        AZ_RTTI(SimpleEnumWrapper, "{CC95FF00-8D04-437A-9849-80D5AD7AD5DC}");
        
        static constexpr bool SupportsPartialDefaults = true;

        SimpleEnumWrapper() = default;
        virtual ~SimpleEnumWrapper() = default;

        bool Equals(const SimpleEnumWrapper& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleEnumWrapper> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleEnumWrapper> GetInstanceWithoutDefaults();

        SimpleEnumClass m_enumClass{};
        SimpleRawEnum m_rawEnum{};
    };

    template<typename T>
    struct TemplatedClass
    {
    };

    template<>
    struct TemplatedClass<int>
    {
        static const bool SupportsPartialDefaults = false;

        bool Equals(const TemplatedClass<int>& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<TemplatedClass<int>> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<TemplatedClass<int>> GetInstanceWithoutDefaults();

        int m_var1{ 142 };
        int m_var2{ 242 };
    };
} // namespace JsonSerializationTests

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::SimpleEnumWrapper::SimpleEnumClass, "{AF6F1964-5B20-4689-BF23-F36B9C9AAE6A}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::SimpleEnumWrapper::SimpleRawEnum, "{EB24207F-B48F-4D8B-940D-3CD06A371739}");
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::TemplatedClass, "{CA4ADF74-66E7-4D16-B4AC-F71278C60EC7}", AZ_TYPE_INFO_TYPENAME);
}
