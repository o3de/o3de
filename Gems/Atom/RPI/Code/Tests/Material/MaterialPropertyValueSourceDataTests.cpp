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

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceDataSerializer.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>

namespace JsonSerializationTests
{
    class MaterialPropertyValueSourceDataSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::RPI::MaterialPropertyValueSourceData>
    {
    public:
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            AZ::RPI::MaterialPropertyValueSourceData::Reflect(context.get());
        }

        void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            context->Serializer<AZ::RPI::JsonMaterialPropertyValueSourceDataSerializer>()->HandlesType<AZ::RPI::MaterialPropertyValueSourceData>();
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::RPI::JsonMaterialPropertyValueSourceDataSerializer>();
        }

        AZStd::shared_ptr<AZ::RPI::MaterialPropertyValueSourceData> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::RPI::MaterialPropertyValueSourceData>();
        }

        AZStd::shared_ptr<AZ::RPI::MaterialPropertyValueSourceData> CreateFullySetInstance() override
        {
            auto result = AZStd::make_shared<AZ::RPI::MaterialPropertyValueSourceData>();
            result->SetValue(42);
            return result;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(42)";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kFalseType);
            features.EnableJsonType(rapidjson::kTrueType);
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kStringType);
            features.EnableJsonType(rapidjson::kNumberType);
            features.m_fixedSizeArray = true;
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(
            const AZ::RPI::MaterialPropertyValueSourceData& lhs,
            const AZ::RPI::MaterialPropertyValueSourceData& rhs) override
        {
            return AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(lhs, rhs);
        }
    };

    using MaterialPropertyValueSourceDataSerializerTestTypes = ::testing::Types<MaterialPropertyValueSourceDataSerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(MaterialPropertyValueSourceDataTests, JsonSerializerConformityTests, MaterialPropertyValueSourceDataSerializerTestTypes);
} // namespace JsonSerializationTests

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialPropertyValueSourceDataTests
        : public RPITestFixture
    {
    protected:

        //! A dummy material creator filled with necessary data.
        //! It is kept alive between SetUp and TearDown, so that MaterialPropertyValueSourceData can have access to the layout and resolve its value.
        MaterialTypeAssetCreator m_materialTypeCreator;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Samples used for testing MaterialFunctor

        class ValueFunctor final
            : public MaterialFunctor
        {
        public:
            AZ_RTTI(MaterialPropertyValueSourceDataTests::ValueFunctor, "{07CE498C-6E97-45C9-8B2D-18BC03724055}", AZ::RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<ValueFunctor, AZ::RPI::MaterialFunctor>()
                        ->Version(1)
                        ->Field("propertyIndex", &ValueFunctor::m_propertyIndex)
                        ->Field("propertyValue", &ValueFunctor::m_propertyValue)
                        ;
                }
            }

            MaterialPropertyIndex m_propertyIndex;
            MaterialPropertyValue m_propertyValue;
        };

        class ValueFunctorSourceData final
            : public MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(ValueFunctorSourceData, "{777CE7A5-3023-4C63-BA43-5763DF51D82D}", AZ::RPI::MaterialFunctorSourceData);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<ValueFunctorSourceData>()
                        ->Version(1)
                        ->Field("propertyName", &ValueFunctorSourceData::m_propertyName)
                        ->Field("propertyValue", &ValueFunctorSourceData::m_propertyValue)
                        ;
                }
            }

            AZStd::string m_propertyName;
            MaterialPropertyValueSourceData m_propertyValue;

            using MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override
            {
                Ptr<ValueFunctor> functor = aznew ValueFunctor;

                functor->m_propertyIndex = context.FindMaterialPropertyIndex(Name(m_propertyName));
                EXPECT_TRUE(functor->m_propertyIndex.IsValid());

                m_propertyValue.Resolve(*context.GetMaterialPropertiesLayout(), AZ::Name(m_propertyName));

                functor->m_propertyValue = m_propertyValue.GetValue();

                return Success(Ptr<MaterialFunctor>(functor));
            }

        };

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        void Reflect(ReflectContext* context) override
        {
            RPITestFixture::Reflect(context);

            MaterialPropertyValueSourceData::Reflect(context);

            MaterialFunctorSourceDataHolder::Reflect(context);

            ValueFunctorSourceData::Reflect(context);
            ValueFunctor::Reflect(context);
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_materialTypeCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Bool"},    MaterialPropertyDataType::Bool);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Int"},     MaterialPropertyDataType::Int);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.UInt"},    MaterialPropertyDataType::UInt);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Float"},   MaterialPropertyDataType::Float);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Vector2"}, MaterialPropertyDataType::Vector2);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Vector3"}, MaterialPropertyDataType::Vector3);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Vector4"}, MaterialPropertyDataType::Vector4);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Color"},   MaterialPropertyDataType::Color);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{"general.Image"},   MaterialPropertyDataType::Image);
            m_materialTypeCreator.EndMaterialProperty();
            m_materialTypeCreator.BeginMaterialProperty(AZ::Name{ "general.Enum" }, MaterialPropertyDataType::Enum);
            m_materialTypeCreator.SetMaterialPropertyEnumNames(AZStd::vector<AZStd::string>({ "DummyEnum" }));
            m_materialTypeCreator.EndMaterialProperty();
        }

        void TearDown() override
        {
            {
                Data::Asset<MaterialTypeAsset> testAsset;
                m_materialTypeCreator.End(testAsset);
            }

            RPITestFixture::TearDown();
        }
    };

    TEST_F(MaterialPropertyValueSourceDataTests, MaterialFunctorTest)
    {
        AZStd::unordered_map<MaterialPropertyDataType, const char *> typeValue =
        {
            {MaterialPropertyDataType::Bool,    "true"},
            {MaterialPropertyDataType::Int,     "-42"},
            {MaterialPropertyDataType::UInt,    "42"},
            {MaterialPropertyDataType::Float,   "42.0"},
            {MaterialPropertyDataType::Vector2, "[42.0, 42.0]"},
            {MaterialPropertyDataType::Vector3, "[42.0, 42.0, 42.0]"},
            {MaterialPropertyDataType::Vector4, "[42.0, 42.0, 42.0, 42.0]"},
            {MaterialPropertyDataType::Color,   "[0.0, 0.0, 0.0, 1.0]"},
            {MaterialPropertyDataType::Image,   "\"DummyImagePath.png\""},
            {MaterialPropertyDataType::Enum,    "\"DummyEnum\""},
        };
        AZStd::array<Ptr<MaterialFunctor>, static_cast<uint32_t>(MaterialPropertyDataType::Count)> valueFunctors;
        valueFunctors.fill(nullptr);
        char inputJson[2048];
        AZStd::string outputJson;

        uint32_t propertyTypeCount = static_cast<uint32_t>(MaterialPropertyDataType::Count);
        static_assert(static_cast<uint32_t>(MaterialPropertyDataType::Invalid) == 0u, "If MaterialPropertyDataType::Invalid has changed its order, the following logic should change accordingly.");
        // Test through each type, serialization and deserialization.
        // Collect the functors and test the serialized value later.
        for (uint32_t i = static_cast<uint32_t>(MaterialPropertyDataType::Invalid) + 1u; i < propertyTypeCount; ++i)
        {
            MaterialPropertyDataType type = static_cast<MaterialPropertyDataType>(i);
            azsprintf(inputJson,
                R"(
                    {
                        "propertyName": "general.%s",
                        "propertyValue": %s
                    }
                )",
                ToString(type),
                typeValue[type]
            );

            // Stored as base class.
            Ptr<ValueFunctorSourceData> functorData = aznew ValueFunctorSourceData;

            JsonTestResult loadResult = LoadTestDataFromJson(*functorData, inputJson);

            // Where type resolving happens.
            MaterialFunctorSourceData::FunctorResult functorResult = functorData->CreateFunctor(
                MaterialFunctorSourceData::RuntimeContext(
                    "Dummy.materialtype",
                    m_materialTypeCreator.GetMaterialPropertiesLayout(),
                    m_materialTypeCreator.GetMaterialShaderResourceGroupLayout(),
                    m_materialTypeCreator.GetShaderCollection()
                )
            );

            valueFunctors[i] = functorResult.GetValue();

            // Store back to json after type is resolved.
            outputJson.clear();
            JsonTestResult storeResult = StoreTestDataToJson(*functorData, outputJson);
            ExpectSimilarJson(inputJson, outputJson);
        }

        MaterialPropertyValue& value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Bool)].get())->m_propertyValue;
        EXPECT_TRUE(value == true);

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Int)].get())->m_propertyValue;
        EXPECT_TRUE(value == -42);

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::UInt)].get())->m_propertyValue;
        EXPECT_TRUE(value == 42u);

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Float)].get())->m_propertyValue;
        EXPECT_TRUE(value == 42.0f);

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Vector2)].get())->m_propertyValue;
        EXPECT_TRUE(value == Vector2(42.0f, 42.0f));

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Vector3)].get())->m_propertyValue;
        EXPECT_TRUE(value == Vector3(42.0f, 42.0f, 42.0f));

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Vector4)].get())->m_propertyValue;
        EXPECT_TRUE(value == Vector4(42.0f, 42.0f, 42.0f, 42.0f));

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Color)].get())->m_propertyValue;
        EXPECT_TRUE(value == Color(0.0f, 0.0f, 0.0f, 1.0f));

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Image)].get())->m_propertyValue;
        EXPECT_TRUE(value == AZStd::string("DummyImagePath.png"));

        value = static_cast<ValueFunctor*>(valueFunctors[static_cast<uint32_t>(MaterialPropertyDataType::Enum)].get())->m_propertyValue;
        EXPECT_TRUE(value == AZStd::string("DummyEnum"));
    }

    TEST_F(MaterialPropertyValueSourceDataTests, DataSimilarityTest)
    {
        MaterialPropertyValueSourceData emptyValue;
        MaterialPropertyValueSourceData onlyResolvedValue[2];
        MaterialPropertyValueSourceData onlyPossibleValue[2];
        MaterialPropertyValueSourceData fullValue[2];

        // Load data
        static const char* source[] = { R"(42)", R"(43)" };
        constexpr int32_t value[] = { 42, 43 };
        for (int32_t i = 0; i < 2; ++i)
        {
            LoadTestDataFromJson(onlyPossibleValue[i], source[i]);
            LoadTestDataFromJson(fullValue[i], source[i]);
            onlyResolvedValue[i].SetValue(value[i]);
            fullValue[i].SetValue(value[i]);
        }

        // Group them into 2 based on the source data.
        // Within each set, AreSimilar should return true.
        // Between two sets, assuming different source data, AreSimilar should return false.
        MaterialPropertyValueSourceData* setA[] = {
            &onlyResolvedValue[0],
            &onlyPossibleValue[0],
            &fullValue[0]
        };

        MaterialPropertyValueSourceData* setB[] = {
            &onlyResolvedValue[1],
            &onlyPossibleValue[1],
            &fullValue[1]
        };

        EXPECT_TRUE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(emptyValue, emptyValue));

        for (int32_t i = 0; i < 3; ++i)
        {
            for (int32_t j = 0; j < 3; ++j)
            {
                // Test within each set and between them, including comparing to the element itself.
                EXPECT_TRUE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(*setA[i], *setA[j]));
                EXPECT_TRUE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(*setB[i], *setB[j]));
                EXPECT_FALSE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(*setA[i], *setB[j]));
            }

            // Test against empty.
            EXPECT_FALSE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(*setA[i], emptyValue));
            EXPECT_FALSE(AZ::RPI::MaterialPropertyValueSourceData::AreSimilar(*setB[i], emptyValue));
        }
    }
}
