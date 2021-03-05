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

#include <AzCore/Serialization/SerializeContext.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    // SimpleClass
    SimpleClass::SimpleClass(int var1)
        : m_var1(var1)
    {}

    SimpleClass::SimpleClass(int var1, float var2)
        : m_var1(var1)
        , m_var2(var2)
    {}

    void SimpleClass::add_ref()
    {
        ++m_refCount;
    }

    void SimpleClass::release()
    {
        if (--m_refCount == 0)
        {
            delete this;
        }
    }

    bool SimpleClass::Equals(const SimpleClass& rhs, bool fullReflection) const
    {
        return !fullReflection || (m_var1 == rhs.m_var1 && m_var2 == rhs.m_var2);
    }

    void SimpleClass::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<SimpleClass>()
                ->Field("var1", &SimpleClass::m_var1)
                ->Field("var2", &SimpleClass::m_var2);
        }
    }

    InstanceWithSomeDefaults<SimpleClass> SimpleClass::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleClass>();
        instance->m_var1 = 88;

        const char* strippedDefaults = R"(
            {
                "var1": 88
            })";
        const char* keptDefaults = R"(
            {
                "var1": 88,
                "var2": 42.0
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleClass> SimpleClass::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleClass>();
        instance->m_var1 = 88;
        instance->m_var2 = 88.0;

        const char* json = R"(
            {
                "var1": 88,
                "var2": 88.0
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }

    bool SimpleClass::operator<(const SimpleClass& rhs) const
    {
        if (m_var1 < rhs.m_var1)
        {
            return true;
        }
        return m_var1 == rhs.m_var1 ? m_var2 < rhs.m_var2 : false;
    }

    bool SimpleClass::operator>(const SimpleClass& rhs) const
    {
        return !((*this < rhs) || (*this == rhs));
    }

    bool SimpleClass::operator<=(const SimpleClass& rhs) const
    {
        return !(*this > rhs);
    }

    bool SimpleClass::operator>=(const SimpleClass& rhs) const
    {
        return !(*this < rhs);
    }

    bool SimpleClass::operator==(const SimpleClass& rhs) const
    {
        return m_var1 == rhs.m_var1 && m_var2 == rhs.m_var2;
    }

    bool SimpleClass::operator!=(const SimpleClass& rhs) const
    {
        return !(*this == rhs);
    }



    // SimpleInheritence

    bool SimpleInheritence::Equals(const SimpleInheritence& rhs, bool fullReflection) const
    {
        if (m_var1 != rhs.m_var1 || m_var2 != rhs.m_var2)
        {
            return false;
        }
        return !fullReflection || BaseClass::Equals(rhs, fullReflection);
    }

    void SimpleInheritence::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            BaseClass::Reflect(context);
        }
        context->Class<SimpleInheritence, BaseClass>()
            ->Field("var1", &SimpleInheritence::m_var1)
            ->Field("var2", &SimpleInheritence::m_var2);
    }

    InstanceWithSomeDefaults<SimpleInheritence> SimpleInheritence::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleInheritence>();
        instance->m_var1 = 88;

        const char* strippedDefaults = R"(
            {
                "var1": 88
            })";
        const char* keptDefaults = R"(
            {
                "base_var": -42.0,
                "var1": 88,
                "var2": 42.0
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleInheritence> SimpleInheritence::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleInheritence>();
        instance->m_var1 = 88;
        instance->m_var2 = 88.0;
        instance->m_baseVar = -88.0f;

        const char* json = R"(
        {
            "base_var": -88.0,
            "var1": 88,
            "var2": 88.0
        })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // MultipleInheritence

    bool MultipleInheritence::Equals(const MultipleInheritence& rhs, bool fullReflection) const
    {
        if (m_var1 != rhs.m_var1 || m_var2 != rhs.m_var2 || !BaseClass::Equals(rhs, fullReflection))
        {
            return false;
        }
        return !fullReflection || BaseClass2::Equals(rhs, fullReflection);
    }

    void MultipleInheritence::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        BaseClass::Reflect(context);
        if (fullReflection)
        {
            BaseClass2::Reflect(context);
        }
        context->Class<MultipleInheritence, BaseClass, BaseClass2>()
            ->Field("var1", &MultipleInheritence::m_var1)
            ->Field("var2", &MultipleInheritence::m_var2);
    }

    InstanceWithSomeDefaults<MultipleInheritence> MultipleInheritence::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<MultipleInheritence>();
        instance->m_var1 = 88;
        instance->m_base2Var1 = -88.0;

        const char* strippedDefaults = R"(
            {
                "base2_var1": -88.0,
                "var1": 88
            })";
        const char* keptDefaults = R"(
            {
                "base_var": -42.0,
                "base2_var1": -88.0,
                "base2_var2": -233.0,
                "base2_var3": -333.0,
                "var1": 88,
                "var2": 42.0
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<MultipleInheritence> MultipleInheritence::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<MultipleInheritence>();
        instance->m_var1 = 88;
        instance->m_var2 = 88.0;
        instance->m_baseVar = -88.0f;
        instance->m_base2Var1 = -190.0;
        instance->m_base2Var2 = -290.0;
        instance->m_base2Var3 = -390.0;

        const char* json = R"(
            {
                "base_var": -88.0,
                "base2_var1": -190.0,
                "base2_var2": -290.0,
                "base2_var3": -390.0,
                "var1": 88,
                "var2": 88.0
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // SimpleNested

    bool SimpleNested::Equals(const SimpleNested& rhs, bool fullReflection) const
    {
        if (m_varAdditional != rhs.m_varAdditional)
        {
            return false;
        }
        return m_nested.Equals(rhs.m_nested, fullReflection);
    }

    void SimpleNested::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            SimpleClass::Reflect(context, true);
        }
        context->Class<SimpleNested>()
            ->Field("nested", &SimpleNested::m_nested)
            ->Field("var_additional", &SimpleNested::m_varAdditional);
    }

    InstanceWithSomeDefaults<SimpleNested> SimpleNested::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleNested>();
        instance->m_nested.m_var1 = 88;

        const char* strippedDefaults = R"(
            {
                "nested": {
                    "var1": 88
                }
            })";
        const char* keptDefaults = R"(
            {
                "nested": {
                    "var1": 88,
                    "var2": 42.0
                },
                "var_additional": 42
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleNested> SimpleNested::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleNested>();
        instance->m_nested.m_var1 = 88;
        instance->m_nested.m_var2 = 88.0;
        instance->m_varAdditional = -88;

        const char* json = R"(
            {
                "nested": {
                    "var1": 88,
                    "var2": 88.0
                },
                "var_additional": -88
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }

    // SimpleEnumWrapper
    bool SimpleEnumWrapper::Equals(const SimpleEnumWrapper& rhs, bool fullReflection) const
    {
        return !fullReflection || (m_enumClass == rhs.m_enumClass && m_rawEnum== rhs.m_rawEnum);
    }

    void SimpleEnumWrapper::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Enum<SimpleEnumWrapper::SimpleEnumClass>()
                ->Value("Option1", SimpleEnumWrapper::SimpleEnumClass::Option1)
                ->Value("Option2", SimpleEnumWrapper::SimpleEnumClass::Option2)
                ;
            context->Enum<SimpleEnumWrapper::SimpleRawEnum>()
                ->Value("Option1", SimpleEnumWrapper::SimpleRawEnum::RawOption1)
                ->Value("Option2", SimpleEnumWrapper::SimpleRawEnum::RawOption2)
                ;
            context->Class<SimpleEnumWrapper>()
                ->Field("enumClass", &SimpleEnumWrapper::m_enumClass)
                ->Field("rawEnum", &SimpleEnumWrapper::m_rawEnum);
        }
    }

    InstanceWithSomeDefaults<SimpleEnumWrapper> SimpleEnumWrapper::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleEnumWrapper>();
        instance->m_enumClass = SimpleEnumWrapper::SimpleEnumClass::Option2;

        const char* strippedDefaults = R"(
            {
                "enumClass": "Option2"
            })";
        const char* keptDefaults = R"(
            {
                "enumClass": "Option2",
                "rawEnum": 0
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleEnumWrapper> SimpleEnumWrapper::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleEnumWrapper>();
        instance->m_enumClass = SimpleEnumWrapper::SimpleEnumClass::Option2;
        instance->m_rawEnum = SimpleEnumWrapper::SimpleRawEnum::RawOption1;

        const char* json = R"(
            {
                "enumClass": "Option2",
                "rawEnum": "Option1"
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }

    // TemplatedClass<int>

    bool TemplatedClass<int>::Equals(const TemplatedClass<int>& rhs, bool fullReflection) const
    {
        return !fullReflection || (m_var1 == rhs.m_var1 && m_var2 == rhs.m_var2);
    }

    void TemplatedClass<int>::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<TemplatedClass<int>>()
                ->Field("var1", &TemplatedClass<int>::m_var1)
                ->Field("var2", &TemplatedClass<int>::m_var2);
        }
    }

    InstanceWithSomeDefaults<TemplatedClass<int>> TemplatedClass<int>::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<TemplatedClass<int>>();
        instance->m_var1 = 188;

        const char* strippedDefaults = R"(
            {
                "var1": 188
            })";
        const char* keptDefaults = R"(
            {
                "var1": 188,
                "var2": 242
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<TemplatedClass<int>> TemplatedClass<int>::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<TemplatedClass<int>>();
        instance->m_var1 = 188;
        instance->m_var2 = 288;

        const char* json = R"(
            {
                "var1": 188,
                "var2": 288
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }
} // namespace JsonSerializationTests
