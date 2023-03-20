/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/SmartPointerSerializer.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename SmartPointer>
    class SmartPointerBaseTestDescription :
        public JsonSerializerConformityTestDescriptor<SmartPointer>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonSmartPointerSerializer>();
        }

        AZStd::shared_ptr<SmartPointer> CreateDefaultConstructedInstance() override
        {
            return AZStd::make_shared<SmartPointer>();
        }

        using JsonSerializerConformityTestDescriptor<SmartPointer>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<SmartPointer>();
        }
    };

    template<template<typename...> class T>
    class SmartPointerSimpleClassTestDescription :
        public SmartPointerBaseTestDescription<T<SimpleClass>>
    {
    public:
        using SmartPointer = T<SimpleClass>;
        using Base = SmartPointerBaseTestDescription<SmartPointer>;

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew SimpleClass());
            return result;
        }

        AZStd::shared_ptr<SmartPointer> CreateFullySetInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew SimpleClass(88, 88.0f));
            return result;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "var1": 88,
                    "var2": 88.0
                })";
        }

        AZStd::shared_ptr<SmartPointer> CreatePartialDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew SimpleClass(88));
            return result;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "var1": 88
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kNullType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = true;
        }

        bool AreEqual(const SmartPointer& lhs, const SmartPointer& rhs) override
        {
            if (!lhs || !rhs)
            {
                return !lhs && !rhs;
            }
            return *lhs == *rhs;
        }

        using Base::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            Base::Reflect(context);
        }
    };

    template<template<typename...> class T>
    class SmartPointerSimpleDerivedClassTestDescription :
        public SmartPointerBaseTestDescription<T<BaseClass>>
    {
    public:
        using SmartPointer = T<BaseClass>;
        using Base = SmartPointerBaseTestDescription<SmartPointer>;

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew BaseClass());
            return result;
        }

        AZStd::shared_ptr<SmartPointer> CreateFullySetInstance() override
        {
            auto* instance = aznew SimpleInheritence();
            instance->m_var1 = 88;
            instance->m_var2 = 88.0f;
            instance->m_baseVar = -188.0f;
            
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(instance);
            return result;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "$type": "SimpleInheritence",
                    "base_var": -188.0,
                    "var1": 88,
                    "var2": 88.0
                })";
        }

        AZStd::shared_ptr<SmartPointer> CreatePartialDefaultInstance() override
        {
            auto* instance = aznew SimpleInheritence();
            instance->m_var1 = 88;
            instance->m_baseVar = -188.0f;

            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(instance);
            return result;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "$type": "SimpleInheritence",
                    "base_var": -188.0,
                    "var1": 88
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kNullType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = true;
        }

        using SmartPointerBaseTestDescription<T<BaseClass>>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleInheritence::Reflect(context, true);
            Base::Reflect(context);
        }

        bool AreEqual(const SmartPointer& lhs, const SmartPointer& rhs) override
        {
            if (!lhs || !rhs)
            {
                return !lhs && !rhs;
            }

            if (azrtti_typeid(*lhs) != azrtti_typeid(*rhs))
            {
                return false;
            }

            if (azrtti_typeid(*lhs) == azrtti_typeid<SimpleInheritence>())
            {
                auto leftInstance = azrtti_cast<SimpleInheritence*>(lhs.get());
                auto rightInstance = azrtti_cast<SimpleInheritence*>(rhs.get());
                return leftInstance->Equals(*rightInstance, true);
            }
            else
            {
                return lhs->Equals(*rhs, true);
            }
        }
    };

    template<template<typename...> class T>
    class SmartPointerSimpleDerivedClassWithBaseInstanceTestDescription :
        public SmartPointerSimpleDerivedClassTestDescription<T>
    {
    public:
        using SmartPointer = typename SmartPointerSimpleDerivedClassTestDescription<T>::SmartPointer;

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew BaseClass());
            return result;
        }
    };

    template<template<typename...> class T>
    class SmartPointerSimpleDerivedClassWithDerivedInstanceTestDescription :
        public SmartPointerSimpleDerivedClassTestDescription<T>
    {
    public:
        using SmartPointer = typename SmartPointerSimpleDerivedClassTestDescription<T>::SmartPointer;

        // This test is specific for derived classes being used as a default value.
        AZStd::shared_ptr<SmartPointer> CreateDefaultConstructedInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew SimpleInheritence());
            return result;
        }

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            return CreateDefaultConstructedInstance();
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "base_var": -188.0,
                    "var1": 88
                })";
        }

        AZStd::string_view GetJsonFor_Store_SerializeFullySetInstance() override
        {
            // This is a unique situation because the $type is determined separate from other values, so all
            // member values can be changed, but since the default type matches the stored type the $type
            // will only be written if default values are explicitly kept.
            return R"(
                    {
                        "base_var": -188.0,
                        "var1": 88,
                        "var2": 88.0
                    })";
        }
    };

    template<template<typename...> class T>
    class SmartPointerComplexDerivedClassTestDescription :
        public SmartPointerBaseTestDescription<T<BaseClass2>>
    {
    public:
        using SmartPointer = T<BaseClass2>;
        using Base = SmartPointerBaseTestDescription<SmartPointer>;

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew BaseClass2());
            return result;
        }

        AZStd::shared_ptr<SmartPointer> CreateFullySetInstance() override
        {
            auto* instance = aznew MultipleInheritence();
            instance->m_var1 = 88;
            instance->m_var2 = 88.0f;
            instance->m_baseVar = -188.0f;
            instance->m_base2Var1 = -2188.0;
            instance->m_base2Var2 = -2288.0;
            instance->m_base2Var3 = -2388.0;

            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(instance);
            return result;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "$type": "MultipleInheritence",
                    "base_var": -188.0,
                    "base2_var1": -2188.0,
                    "base2_var2": -2288.0,
                    "base2_var3": -2388.0,
                    "var1": 88,
                    "var2": 88.0
                })";
        }

        AZStd::shared_ptr<SmartPointer> CreatePartialDefaultInstance() override
        {
            auto* instance = aznew MultipleInheritence();
            instance->m_var1 = 88;
            instance->m_baseVar = -188.0f;
            instance->m_base2Var2 = -2288.0;

            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(instance);
            return result;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "$type": "MultipleInheritence",
                    "base_var": -188.0,
                    "base2_var2": -2288.0,
                    "var1": 88
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kNullType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = true;
        }

        using SmartPointerBaseTestDescription<T<BaseClass2>>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            MultipleInheritence::Reflect(context, true);
            Base::Reflect(context);
        }

        bool AreEqual(const SmartPointer& lhs, const SmartPointer& rhs) override
        {
            if (!lhs || !rhs)
            {
                return !lhs && !rhs;
            }

            if (azrtti_typeid(*lhs) != azrtti_typeid(*rhs))
            {
                return false;
            }

            if (azrtti_typeid(*lhs) == azrtti_typeid<MultipleInheritence>())
            {
                auto leftInstance = azrtti_cast<MultipleInheritence*>(lhs.get());
                auto rightInstance = azrtti_cast<MultipleInheritence*>(rhs.get());
                return leftInstance->Equals(*rightInstance, true);
            }
            else
            {
                return lhs->Equals(*rhs, true);
            }
        }
    };

    template<template<typename...> class T>
    class SmartPointerComplexDerivedClassWithBaseInstanceTestDescription :
        public SmartPointerComplexDerivedClassTestDescription<T>
    {
    public:
        using SmartPointer = typename SmartPointerComplexDerivedClassTestDescription<T>::SmartPointer;

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew BaseClass2());
            return result;
        }
    };

    template<template<typename...> class T>
    class SmartPointerComplexDerivedClassWithDerivedInstanceTestDescription :
        public SmartPointerComplexDerivedClassTestDescription<T>
    {
    public:
        using SmartPointer = typename SmartPointerComplexDerivedClassTestDescription<T>::SmartPointer;

        // This test is specific for derived classes being used as a default value.
        AZStd::shared_ptr<SmartPointer> CreateDefaultConstructedInstance() override
        {
            auto result = AZStd::make_shared<SmartPointer>();
            *result = SmartPointer(aznew MultipleInheritence());
            return result;
        }

        AZStd::shared_ptr<SmartPointer> CreateDefaultInstance() override
        {
            return CreateDefaultConstructedInstance();
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "base_var": -188.0,
                    "base2_var2": -2288.0,
                    "var1": 88
                })";
        }

        AZStd::string_view GetJsonFor_Store_SerializeFullySetInstance() override
        {
            // This is a unique situation because the $type is determined separate from other values, so all
            // member values can be changed, but since the default type matches the stored type the $type
            // will only be written if default values are explicitly kept.
            return R"(
                {
                    "base_var": -188.0,
                    "base2_var1": -2188.0,
                    "base2_var2": -2288.0,
                    "base2_var3": -2388.0,
                    "var1": 88,
                    "var2": 88.0
                })";
        }
    };

    using SmartPointerSerializerConformityTestTypes = ::testing::Types <
        // Simple class.
        SmartPointerSimpleClassTestDescription<AZStd::unique_ptr>,
        SmartPointerSimpleClassTestDescription<AZStd::shared_ptr>,
        SmartPointerSimpleClassTestDescription<AZStd::intrusive_ptr>,
        // Simple derived class, include single inheritance.
        SmartPointerSimpleDerivedClassTestDescription<AZStd::unique_ptr>,
        SmartPointerSimpleDerivedClassTestDescription<AZStd::shared_ptr>,
        SmartPointerSimpleDerivedClassTestDescription<AZStd::intrusive_ptr>,
        SmartPointerSimpleDerivedClassWithBaseInstanceTestDescription<AZStd::unique_ptr>,
        SmartPointerSimpleDerivedClassWithBaseInstanceTestDescription<AZStd::shared_ptr>,
        SmartPointerSimpleDerivedClassWithBaseInstanceTestDescription<AZStd::intrusive_ptr>,
        SmartPointerSimpleDerivedClassWithDerivedInstanceTestDescription<AZStd::unique_ptr>,
        SmartPointerSimpleDerivedClassWithDerivedInstanceTestDescription<AZStd::shared_ptr>,
        SmartPointerSimpleDerivedClassWithDerivedInstanceTestDescription<AZStd::intrusive_ptr>,
        // Complex derived class, include multiple inheritance.
        SmartPointerComplexDerivedClassTestDescription<AZStd::unique_ptr>,
        SmartPointerComplexDerivedClassTestDescription<AZStd::shared_ptr>,
        SmartPointerComplexDerivedClassTestDescription<AZStd::intrusive_ptr>,
        SmartPointerComplexDerivedClassWithBaseInstanceTestDescription<AZStd::unique_ptr>,
        SmartPointerComplexDerivedClassWithBaseInstanceTestDescription<AZStd::shared_ptr>,
        SmartPointerComplexDerivedClassWithBaseInstanceTestDescription<AZStd::intrusive_ptr>,
        SmartPointerComplexDerivedClassWithDerivedInstanceTestDescription<AZStd::unique_ptr>,
        SmartPointerComplexDerivedClassWithDerivedInstanceTestDescription<AZStd::shared_ptr>,
        SmartPointerComplexDerivedClassWithDerivedInstanceTestDescription<AZStd::intrusive_ptr>
    >;

    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(SmartPointerSerializer, JsonSerializerConformityTests, SmartPointerSerializerConformityTestTypes));

    struct SimpleInheritenceAlt : BaseClass
    {
        AZ_CLASS_ALLOCATOR(SimpleInheritenceAlt, AZ::SystemAllocator);
        AZ_RTTI(SimpleInheritenceAlt, "{5513DF52-E3C2-4849-BBFF-13E00F3E3EDA}", BaseClass);

        ~SimpleInheritenceAlt() override = default;

        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
        {
            context->Class<SimpleInheritenceAlt, BaseClass>()
                ->Field("var1", &SimpleInheritenceAlt::m_var1)
                ->Field("var2", &SimpleInheritenceAlt::m_var2);
        }

        int m_var1{ 42 };
        float m_var2{ 42.0f };
    };

    struct AbstractClass : BaseClass
    {
        AZ_CLASS_ALLOCATOR(AbstractClass, AZ::SystemAllocator);
        AZ_RTTI(AbstractClass, "{D065A72E-E49B-4E23-9E4E-A4E08D344FC2}", BaseClass);

        ~AbstractClass() override = default;

        virtual void VirtualFunction() = 0;

        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
        {
            context->Class<AbstractClass, BaseClass>();
        }
    };

    struct UnregisteredClass : BaseClass
    {
        AZ_CLASS_ALLOCATOR(UnregisteredClass, AZ::SystemAllocator);
        AZ_RTTI(UnregisteredClass, "{9163CBB9-0B7F-450E-B93E-A3EC32E5229A}", BaseClass);

        ~UnregisteredClass() override = default;
    };

    class JsonSmartPointerSerializerTests
        : public BaseJsonSerializerFixture
    {
    protected:
        SmartPointerSimpleDerivedClassTestDescription<AZStd::shared_ptr> m_description;
        AZ::JsonSmartPointerSerializer m_serializer;

    public:
        using SmartPointer = typename SmartPointerSimpleDerivedClassTestDescription<AZStd::shared_ptr>::SmartPointer;
        using InstanceSmartPointer = AZStd::shared_ptr<SimpleInheritence>;

        using BaseJsonSerializerFixture::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            m_description.Reflect(context);
            SimpleInheritenceAlt::Reflect(context);
            AbstractClass::Reflect(context);
            context->RegisterGenericType<SmartPointer>();
            context->RegisterGenericType<AZStd::shared_ptr<SimpleInheritence>>();
            context->RegisterGenericType<AZStd::shared_ptr<SimpleInheritenceAlt>>();
            context->RegisterGenericType<AZStd::vector<int>>();
        }
    };

    TEST_F(JsonSmartPointerSerializerTests, Load_ProvidedNonContainerType_ReturnsUnsupported)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZ::Uuid incorrectType = azrtti_typeid<int>();
        JSR::ResultCode result = m_serializer.Load(&instance, incorrectType, *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_ProvideNonSmartPointerType_ReturnsUnsupported)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZ::Uuid incorrectType = azrtti_typeid<AZStd::vector<int>>();
        JSR::ResultCode result = m_serializer.Load(&instance, incorrectType, *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_InstanceBeingReplacedWithNullptr_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        m_jsonDocument->Parse("null");

        JSR::ResultCode result = m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Processing::Completed, result.GetProcessing());
        EXPECT_EQ(nullptr, *instance);
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_DefaultInstanceToNullptr_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateDefaultInstance();
        m_jsonDocument->SetObject();

        JSR::ResultCode result =
            m_serializer.Load(&instance, azrtti_typeid<SmartPointer>(), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Processing::Completed, result.GetProcessing());
        EXPECT_NE(nullptr, instance);
        EXPECT_TRUE(m_description.AreEqual(instance, *compare));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_DefaultObjectDoesNotUpdateInstance_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateFullySetInstance();
        m_jsonDocument->SetObject();

        JSR::ResultCode result =
            m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Processing::Completed, result.GetProcessing());
        EXPECT_TRUE(m_description.AreEqual(*instance, *compare));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_InstanceBeingReplacedWithDifferentType_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        auto instance = AZStd::make_shared<SimpleInheritenceAlt>();
        instance->m_baseVar = 142.0f;
        instance->m_var1 = 242;
        instance->m_var2 = 343.0f;
        auto compare = m_description.CreateFullySetInstance();

        m_jsonDocument->Parse(m_description.GetJsonForFullySetInstance().data());

        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Processing::Completed, result.GetProcessing());
        EXPECT_TRUE((*compare)->Equals(*instance, true));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetNonExistingTypeName_ReturnsUnknownAndLeavesInstanceAsNullptr)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        m_jsonDocument->Parse(R"(
            {
                "$type": "NonExisting"
            })");

        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unknown, result.GetOutcome());
        EXPECT_EQ(nullptr, instance);
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetNonExistingTypeName_ReturnsUnknownAndLeavesInstanceUntouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateFullySetInstance();
        m_jsonDocument->Parse(R"(
            {
                "$type": "NonExisting"
            })");

        JSR::ResultCode result = m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE((*compare)->Equals(**instance, true));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetNonExistingTypeId_ReturnsUnknownAndLeavesInstanceAsNullptr)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        m_jsonDocument->Parse(R"(
            {
                "$type": "{9A4415E7-D744-42D6-82BF-B3408B54D7BD}"
            })");

        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unknown, result.GetOutcome());
        EXPECT_EQ(nullptr, instance);
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetNonExistingTypeId_ReturnsUnknownAndLeavesInstanceUntouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateFullySetInstance();
        m_jsonDocument->Parse(R"(
            {
                "$type": "{9A4415E7-D744-42D6-82BF-B3408B54D7BD}"
            })");

        JSR::ResultCode result = m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE((*compare)->Equals(**instance, true));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetUnrelatedType_ReturnsUnknownAndLeavesInstanceAsNullptr)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        m_jsonDocument->Parse(R"(
            {
                "$type": "int"
            })");

        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::TypeMismatch, result.GetOutcome());
        EXPECT_EQ(nullptr, instance);
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetUnrelatedType_ReturnsUnknownAndLeavesInstanceUntouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateFullySetInstance();
        m_jsonDocument->Parse(R"(
            {
                "$type": "int"
            })");

        JSR::ResultCode result = m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::TypeMismatch, result.GetOutcome());
        EXPECT_TRUE((*compare)->Equals(**instance, true));
    }

    TEST_F(JsonSmartPointerSerializerTests, Load_TargetAbstractClass_ReturnsUnknownAndLeavesInstanceUntouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        AZStd::shared_ptr<SmartPointer> compare = m_description.CreateFullySetInstance();
        m_jsonDocument->Parse(R"(
            {
                "$type": "AbstractClass"
            })");

        JSR::ResultCode result = m_serializer.Load(instance.get(), azrtti_typeid<SmartPointer>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Catastrophic, result.GetOutcome());
        EXPECT_TRUE((*compare)->Equals(**instance, true));
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ProvidedNonContainerType_ReturnsUnsupported)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZ::Uuid incorrectType = azrtti_typeid<int>();
        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, nullptr, incorrectType, *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ProvideNonSmartPointerType_ReturnsUnsupported)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZ::Uuid incorrectType = azrtti_typeid<AZStd::vector<int>>();
        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, nullptr, incorrectType, *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_DefaultPointerIsNullPtr_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateFullySetInstance();
        SmartPointer defaultInstance;
        JSR::ResultCode result = m_serializer.Store(
            *m_jsonDocument, instance.get(), &defaultInstance,
            azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Success, result.GetOutcome());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ValuePointerIsNullPtr_ReturnsSuccessAndStoresNull)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        AZStd::shared_ptr<SmartPointer> defaultInstance = m_description.CreateFullySetInstance();
        JSR::ResultCode result = m_serializer.Store(
            *m_jsonDocument, &instance, defaultInstance.get(), azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Success, result.GetOutcome());
        EXPECT_TRUE(m_jsonDocument->IsNull());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ValueAndDefaultPointersAreNullPtr_ReturnsSuccessAndStoresNull)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance;
        SmartPointer defaultInstance;
        JSR::ResultCode result =
            m_serializer.Store(*m_jsonDocument, &instance, &defaultInstance, azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_TRUE(m_jsonDocument->IsNull());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ValueAndDefaultPointersAreBothDefault_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateDefaultInstance();
        AZStd::shared_ptr<SmartPointer> defaultInstance = m_description.CreateDefaultInstance();
        JSR::ResultCode result =
            m_serializer.Store(*m_jsonDocument, instance.get(), defaultInstance.get(), azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::DefaultsUsed, result.GetOutcome());
        Expect_ExplicitDefault(*m_jsonDocument);
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ValueHasDefaultValuesAndDefaultHasNullPointer_ReturnsSuccess)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZStd::shared_ptr<SmartPointer> instance = m_description.CreateDefaultInstance();
        SmartPointer defaultInstance;
        JSR::ResultCode result = m_serializer.Store(
            *m_jsonDocument, instance.get(), &defaultInstance, azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::DefaultsUsed, result.GetOutcome());
        Expect_ExplicitDefault(*m_jsonDocument);
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_DefaultPointerIsOtherClass_CompletesButDoesNotReturnDefaults)
    {
        namespace JSR = AZ::JsonSerializationResult;

        auto instance = AZStd::make_shared<SimpleInheritence>();
        instance->m_baseVar = 142.0f;
        instance->m_var1 = 242;
        instance->m_var2 = 343.0f;

        auto defaultInstance = AZStd::make_shared<SimpleInheritenceAlt>();
        defaultInstance->m_baseVar = 142.0f;
        defaultInstance->m_var1 = 242;
        defaultInstance->m_var2 = 343.0f;

        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, &defaultInstance,
            azrtti_typeid(instance), *m_jsonSerializationContext);

        EXPECT_NE(JSR::Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_NE(JSR::Outcomes::PartialDefaults, result.GetOutcome());
        EXPECT_EQ(JSR::Processing::Completed, result.GetProcessing());
    }

    TEST_F(JsonSmartPointerSerializerTests, Store_ClassThatIsNotReflected_ReturnsUnknown)
    {
        namespace JSR = AZ::JsonSerializationResult;

        SmartPointer instance = AZStd::make_shared<UnregisteredClass>();
        m_jsonDocument->SetObject();
        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, nullptr,
            azrtti_typeid<SmartPointer>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Unknown, result.GetOutcome());
        ASSERT_TRUE(m_jsonDocument->IsObject());
        EXPECT_EQ(0, m_jsonDocument->MemberCount());
    }
} // namespace JsonSerializationTests
