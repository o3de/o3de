/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/JsonTestUtils.h>
#include <Common/TestUtils.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>

namespace JsonSerializationTests
{
    class MaterialFunctorSourceDataSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::RPI::MaterialFunctorSourceDataHolder>
    {
    public:
        class TestFunctorSourceData
            : public AZ::RPI::MaterialFunctorSourceData
        {
        public:
            AZ_CLASS_ALLOCATOR(TestFunctorSourceData, AZ::SystemAllocator)
            AZ_RTTI(TestFunctorSourceData, "{D9B569EA-F45B-4852-9F42-0C104C51166A}", AZ::RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<TestFunctorSourceData>()
                        ->Version(1)
                        ->Field("value", &TestFunctorSourceData::m_value)
                        ;
                }
            }

            int32_t m_value = 0;
        };

        AZ::RPI::MaterialFunctorSourceDataRegistration m_functorRegistration;

        void SetUp() override
        {
            m_functorRegistration.Init();
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("Test", azrtti_typeid<TestFunctorSourceData>());
        }

        virtual void TearDown() override
        {
            m_functorRegistration.Shutdown();
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            TestFunctorSourceData::Reflect(context.get());
            AZ::RPI::MaterialFunctorSourceDataHolder::Reflect(context.get());
        }

        void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            context->Serializer<AZ::RPI::JsonMaterialFunctorSourceDataSerializer>()->HandlesType<AZ::RPI::MaterialFunctorSourceDataHolder>();
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::RPI::JsonMaterialFunctorSourceDataSerializer>();
        }

        AZStd::shared_ptr<AZ::RPI::MaterialFunctorSourceDataHolder> CreateDefaultInstance() override
        {
            // As normal we would expect default constructed MaterialFunctorSourceDataHolder.
            // But from its design, the actual default is TestFunctorSourceData.
            TestFunctorSourceData* actualSourceData = aznew TestFunctorSourceData();
            return AZStd::make_shared<AZ::RPI::MaterialFunctorSourceDataHolder>(actualSourceData);
        }

        AZStd::shared_ptr<AZ::RPI::MaterialFunctorSourceDataHolder> CreateFullySetInstance() override
        {
            TestFunctorSourceData* actualSourceData = aznew TestFunctorSourceData();
            actualSourceData->m_value = 42;
            return AZStd::make_shared<AZ::RPI::MaterialFunctorSourceDataHolder>(actualSourceData);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return
                R"(
                    {
                        "type": "Test",
                        "args":
                        {
                            "value": 42
                        }
                    }
                )";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_mandatoryFields.push_back("type");
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(
            const AZ::RPI::MaterialFunctorSourceDataHolder& lhs,
            const AZ::RPI::MaterialFunctorSourceDataHolder& rhs) override
        {
            TestFunctorSourceData* leftSource = azrtti_cast<TestFunctorSourceData*>(lhs.GetActualSourceData().get());
            TestFunctorSourceData* rightSource = azrtti_cast<TestFunctorSourceData*>(rhs.GetActualSourceData().get());

            return (!leftSource && !rightSource || (leftSource && rightSource && (leftSource->m_value == rightSource->m_value)));
        }
    };

    using MaterialFunctorSourceDataSerializerTestTypes = ::testing::Types<MaterialFunctorSourceDataSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(MaterialFunctorSourceDataTests, JsonSerializerConformityTests, MaterialFunctorSourceDataSerializerTestTypes));
} // namespace JsonSerializationTests
