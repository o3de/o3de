/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <Tests/Serialization/Json/TestCases_Pointers.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>

#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/MathVectorSerializer.h>

#define ANY_JSON_REFERENCE_STRING_FORMAT "{\n\t\"$type\": \"%s\",\n\t\"Value\": %s\n}"

namespace JsonSerializationTests
{
    class AnyFixture
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_anySerializer = AZStd::make_unique<AZ::JsonAnySerializer>();
        }

        void TearDown() override
        {
            m_anySerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        template<typename T>
        void CompareStoredAgainstLoaded(const T& t)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::any instanceStore;
            instanceStore = t;
            ASSERT_EQ(azrtti_typeid<T>(), instanceStore.type());
            ASSERT_EQ(t, *AZStd::any_cast<T>(&instanceStore));

            ResultCode resultStore = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instanceStore, *m_serializationSettings);
            ASSERT_NE(Processing::Halted, resultStore.GetProcessing());

            AZStd::any instanceLoad;
            ResultCode resultLoad = AZ::JsonSerialization::Load(instanceLoad, *m_jsonDocument, *m_deserializationSettings);
            ASSERT_EQ(Outcomes::Success, resultLoad.GetOutcome());

            ASSERT_TRUE(!instanceLoad.empty());
            ASSERT_EQ(instanceStore.type(), instanceLoad.type());
            ASSERT_TRUE(AZStd::any_cast<T>(&instanceLoad) != nullptr);
            EXPECT_EQ(*AZStd::any_cast<T>(&instanceStore), *AZStd::any_cast<T>(&instanceLoad));
        }

    protected:
        AZStd::unique_ptr<AZ::JsonAnySerializer> m_anySerializer;
    };

    TEST_F(AnyFixture, Any2JSON_StoreAndLoad_Bool)
    {
        CompareStoredAgainstLoaded(true);
        CompareStoredAgainstLoaded(false);
    }

    TEST_F(AnyFixture, Any2JSON_StoreAndLoad_Vector3)
    {
        CompareStoredAgainstLoaded(AZ::Vector3(1.23f, 5.711f, 13.17f));
        CompareStoredAgainstLoaded(AZ::Vector3(9.0f, -9.0f, -81.0f));
    }

    // These tests test the *transparency* of the effect of storage through an any.
    // That is, serializing a value contained in an any should behave exactly as serializing a value NOT in an any.
    // *DO NOT DISABLE THESE TESTS*
    template<typename T>
    class AnySerializerTestDescription final
        : public JsonSerializerConformityTestDescriptor<AZStd::any>
    {
    public:
        using Any = AZStd::any;
        static constexpr bool SupportsPartialDefaults = T::SupportsPartialDefaults;
        static constexpr bool PartialDefaultReportingIsStrict = T::PartialDefaultReportingIsStrict;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonAnySerializer>();
        }

        AZStd::shared_ptr<AZStd::any> CreateDefaultInstance() override
        {
            T instanceT;
            AZStd::any anyT;
            anyT = instanceT;
            return AZStd::make_shared<AZStd::any>(anyT);
        }

        AZStd::shared_ptr<AZStd::any> CreateFullySetInstance() override
        {
            T instanceT = *T::GetInstanceWithoutDefaults().m_instance.get();
            AZStd::any anyT;
            anyT = instanceT;
            return AZStd::make_shared<AZStd::any>(anyT);
        }

        AZStd::shared_ptr<AZStd::any> CreatePartialDefaultInstance()
        {
            T instanceT = *T::GetInstanceWithSomeDefaults().m_instance.get();
            AZStd::any anyT;
            anyT = instanceT;
            return AZStd::make_shared<AZStd::any>(anyT);
        }

        std::optional<AZStd::string_view> GetJSON([[maybe_unused]] const JSONRequestSpec& requestSpec) override
        {
            if (requestSpec.jsonStorage == DefaultJSONStorage::Drop)
            {
                // dropping defaults
                if (requestSpec.objectSupplied == DefaultObjectSuppliedInSerialization::True)
                {
                    if (requestSpec.objectStatus == DefaultSerializedObjectStatus::None)
                    {
                        // fully configured
                        return m_jsonForFullyConfiguredInstanceWithoutDefaults;
                    }
                }
                else
                {
                    if (requestSpec.objectStatus == DefaultSerializedObjectStatus::None)
                    {
                        return m_jsonForFullyConfiguredInstanceWithoutDefaults;
                    }
                }
            }

            return AZStd::nullopt;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return m_jsonForPartialDefaultInstanceStrippedDefaults;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return m_jsonForFullyConfiguredInstance;
        }
        
        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_defaultIsEqualToEmpty = false;
            features.m_partialDefaultReportingIsStrict = PartialDefaultReportingIsStrict;
            features.m_supportsPartialInitialization = SupportsPartialDefaults;
        }

        bool AreEqual(const AZStd::any& lhs, const AZStd::any& rhs) override
        {
            if (lhs.type() == azrtti_typeid<T>() && lhs.type() == rhs.type())
            {
                auto lhsValue = AZStd::any_cast<const T>(&lhs);
                auto rhsValue = AZStd::any_cast<const T>(&rhs);
                return lhsValue && rhsValue && lhsValue->Equals(*rhsValue, m_fullyReflected);
            }

            return lhs.empty() == rhs.empty()
                && lhs.type() == rhs.type();
        }

        void SetUp() override
        {
            auto instanceWithoutDefaults = T::GetInstanceWithoutDefaults();
            auto instanceWithSomeDefaults = T::GetInstanceWithSomeDefaults();

            const char* typeName = T::RTTI_TypeName();
            const char* fullInstanceWithoutDefaultsInJSON = instanceWithoutDefaults.m_jsonWithStrippedDefaults;
            const char* fullInstanceWithDefaultsInJSON = instanceWithoutDefaults.m_jsonWithKeptDefaults;
            const char* withSomeDefaults = instanceWithSomeDefaults.m_jsonWithStrippedDefaults;
            const char* withKeptDefaults = instanceWithSomeDefaults.m_jsonWithKeptDefaults;

            m_jsonForFullyConfiguredInstance = AZStd::fixed_string<2048>::format(ANY_JSON_REFERENCE_STRING_FORMAT, typeName, fullInstanceWithDefaultsInJSON);
            m_jsonForFullyConfiguredInstanceWithoutDefaults = AZStd::fixed_string<2048>::format(ANY_JSON_REFERENCE_STRING_FORMAT, typeName, fullInstanceWithoutDefaultsInJSON);
            m_jsonForPartialDefaultInstanceStrippedDefaults = AZStd::fixed_string<2048>::format(ANY_JSON_REFERENCE_STRING_FORMAT, typeName, withSomeDefaults);
            m_jsonForPartialDefaultInstanceKeptDefaults = AZStd::fixed_string<2048>::format(ANY_JSON_REFERENCE_STRING_FORMAT, typeName, withKeptDefaults);
        }

        using JsonSerializerConformityTestDescriptor<AZStd::any>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            T::Reflect(context, true);
            m_fullyReflected = true;
        }

        void TearDown() override {}

    private:
        bool m_fullyReflected = false;
        AZStd::fixed_string<2048> m_jsonForPartialDefaultInstanceKeptDefaults;
        AZStd::fixed_string<2048> m_jsonForPartialDefaultInstanceStrippedDefaults;
        AZStd::fixed_string<2048> m_jsonForFullyConfiguredInstance;
        AZStd::fixed_string<2048> m_jsonForFullyConfiguredInstanceWithoutDefaults;
    };

     using AnyConformityTestTypes = ::testing::Types
         // All pass, and all must pass; do not remove any types from these testing cases.
         < AnySerializerTestDescription<SimpleClass>
         , AnySerializerTestDescription<SimpleInheritence>
         , AnySerializerTestDescription<MultipleInheritence>
         , AnySerializerTestDescription<SimpleNested>
         , AnySerializerTestDescription<SimpleEnumWrapper>
         , AnySerializerTestDescription<NonReflectedEnumWrapper>
         , AnySerializerTestDescription<SimpleAssignedPointer>
         , AnySerializerTestDescription<ComplexAssignedPointer>
         , AnySerializerTestDescription<SimpleNullPointer>
         , AnySerializerTestDescription<ComplexNullInheritedPointer>
         , AnySerializerTestDescription<ComplexAssignedDifferentInheritedPointer>
         , AnySerializerTestDescription<ComplexAssignedSameInheritedPointer>
         , AnySerializerTestDescription<SimplePointerInContainer>
         , AnySerializerTestDescription<InheritedPointerInContainer>
         , AnySerializerTestDescription<PrimitivePointerInContainer>
         >;
     IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(Any, JsonSerializerConformityTests, AnyConformityTestTypes));
}
