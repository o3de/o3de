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

#define ANY_JSON "{\n\t\"$type\": \"%s\",\n\t\"value\": %s\n}"

 // test the any serializer against several standard serializers, and then
 // compare results of deserialization of types against each other
 // then just do the erroneous input and empty any stuff

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

    // write this against the TestCases_Classes thing but store everything in an any

    // these tests test the *transparency* of the effect of storage through an any
    // that is, serializing a value contained in an any should behave exactly as not in an any
    template<typename T>
    class AnySerializerTestDescription final
        : public JsonSerializerConformityTestDescriptor<AZStd::any>
    {
    public:
        using Any = AZStd::any;
        static constexpr bool SupportsPartialDefaults = T::SupportsPartialDefaults;

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

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            // return m_jsonForPartialDefaultInstanceKeptDefaults.begin(); // may need to split up return values
            return m_jsonForPartialDefaultInstanceStrippedDefaults.begin();
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return m_jsonForFullyConfiguredIntance.begin();
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_defaultIsEqualToEmpty = false;
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
            const char* withoutDefaultsJSON = instanceWithoutDefaults.m_json;
            const char* withSomeDefaults = instanceWithSomeDefaults.m_jsonWithStrippedDefaults;
            const char* withKeptDefaults = instanceWithSomeDefaults.m_jsonWithKeptDefaults;

            azsnprintf(m_jsonForFullyConfiguredIntance.begin(), 2048, ANY_JSON, typeName, withoutDefaultsJSON);
            azsnprintf(m_jsonForPartialDefaultInstanceStrippedDefaults.begin(), 2048, ANY_JSON, typeName, withSomeDefaults);
            azsnprintf(m_jsonForPartialDefaultInstanceKeptDefaults.begin(), 2048, ANY_JSON, typeName, withKeptDefaults);
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
        AZStd::fixed_string<2048> m_jsonForFullyConfiguredIntance;
    };

    using AnyConformityTestTypes = ::testing::Types
        // all work
        < AnySerializerTestDescription<SimpleClass>
        , AnySerializerTestDescription<SimpleInheritence>
        , AnySerializerTestDescription<MultipleInheritence>
        , AnySerializerTestDescription<SimpleNested>
        , AnySerializerTestDescription<SimpleEnumWrapper>
        , AnySerializerTestDescription<NonReflectedEnumWrapper>
        , AnySerializerTestDescription<SimpleAssignedPointer>
        , AnySerializerTestDescription<ComplexAssignedPointer>
       
        /*
        // fails all cases
        AnySerializerTestDescription<SimpleNullPointer>

        // cause problems with memory allocation tracking, if not actual serialization issues
        // , AnySerializerTestDescription<ComplexNullInheritedPointer>
        // , AnySerializerTestDescription<ComplexAssignedDifferentInheritedPointer>
        // , AnySerializerTestDescription<ComplexAssignedSameInheritedPointer>
        // , AnySerializerTestDescription<PrimitivePointerInContainer>

        // a null check should allow this to pass, defaults should allow null type
        // , AnySerializerTestDescription<SimplePointerInContainer> 
        // , AnySerializerTestDescription<InheritedPointerInContainer>
        */
        >;
    INSTANTIATE_TYPED_TEST_CASE_P(Any, JsonSerializerConformityTests, AnyConformityTestTypes);
}
