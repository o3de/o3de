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

#include <AzTest/AzTest.h>

#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Rules/CommentRule.h>

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    namespace SceneAPI
    {
        class MockRotationRule final
            : public DataTypes::IManifestObject
        {
        public:
            AZ_RTTI(MockRotationRule, "{90AECE4A-58D4-411C-9CDE-59B54C59354F}", DataTypes::IManifestObject);
            AZ_CLASS_ALLOCATOR(MockRotationRule, AZ::SystemAllocator, 0);

            static void Reflect(ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MockRotationRule, DataTypes::IManifestObject>()
                        ->Version(1)
                        ->Field("rotation", &MockRotationRule::m_rotation);
                }
            }

            AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        };

        namespace Containers
        {
            class SceneManifestContainer
            {
            public:
                static AZ::Outcome<rapidjson::Document, AZStd::string> SaveToJsonDocumentHelper(
                    SceneAPI::Containers::SceneManifest& manifest,
                    SerializeContext* context,
                    JsonRegistrationContext* registrationContext)
                {
                    return manifest.SaveToJsonDocument(context, registrationContext);
                }
            };
        }
    }

    namespace SceneData
    {
        class SceneManifest_JSON
            : public UnitTest::AllocatorsFixture
        {
        public:
            AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
            AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
            AZStd::unique_ptr<JsonSystemComponent> m_jsonSystemComponent;

            void SetUp() override
            {
                UnitTest::AllocatorsFixture::SetUp();
                AZ::NameDictionary::Create();

                m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
                AZ::SceneAPI::RegisterDataTypeReflection(m_serializeContext.get());
                AZ::SceneAPI::Containers::SceneManifest::Reflect(m_serializeContext.get());
                AZ::SceneAPI::DataTypes::IManifestObject::Reflect(m_serializeContext.get());
                AZ::SceneAPI::MockRotationRule::Reflect(m_serializeContext.get());

                m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

                m_jsonSystemComponent = AZStd::make_unique<JsonSystemComponent>();
                m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            }

            void TearDown() override
            {
                m_jsonRegistrationContext->EnableRemoveReflection();
                m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
                m_jsonRegistrationContext->DisableRemoveReflection();

                m_serializeContext.reset();
                m_jsonRegistrationContext.reset();
                m_jsonSystemComponent.reset();

                AZ::NameDictionary::Destroy();
                UnitTest::AllocatorsFixture::TearDown();
            }
        };

        TEST_F(SceneManifest_JSON, LoadFromString_BlankManifest_HasDefaultParts)
        {
            SceneAPI::Containers::SceneManifest sceneManifest;
            sceneManifest.LoadFromString("{}", m_serializeContext.get(), m_jsonRegistrationContext.get(), false);
        }

        TEST_F(SceneManifest_JSON, LoadFromString_LoadRotationRuleWithQuaternion_ReturnsTrue)
        {
            using namespace SceneAPI::Containers;
            SceneManifest sceneManifest;

            auto anglesInDegrees = AZ::Vector3(45.0f, 90.0f, 45.0f);
            auto originRule = AZStd::make_shared<AZ::SceneAPI::MockRotationRule>();
            originRule->m_rotation = Quaternion::CreateFromEulerAnglesDegrees(anglesInDegrees);
            sceneManifest.AddEntry(originRule);

            auto writeToJsonResult = SceneManifestContainer::SaveToJsonDocumentHelper(sceneManifest, m_serializeContext.get(), m_jsonRegistrationContext.get());
            ASSERT_TRUE(writeToJsonResult.IsSuccess());

            AZStd::string jsonText;
            auto writeToStringResult = AzFramework::FileFunc::WriteJsonToString(writeToJsonResult.GetValue(), jsonText);
            ASSERT_TRUE(writeToStringResult.IsSuccess());
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("$type": "MockRotationRule")"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("rotation": [)"));

            SceneManifest loaded;
            auto loadFromStringResult = loaded.LoadFromString(jsonText, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(loadFromStringResult.IsSuccess());
            EXPECT_FALSE(loaded.IsEmpty());

            ASSERT_EQ(loaded.GetEntryCount(), sceneManifest.GetEntryCount());
        }

        TEST_F(SceneManifest_JSON, LoadFromString_LoadRotationRuleWithAnglesInDegrees_ReturnsTrue)
        {
            using namespace SceneAPI::Containers;

            constexpr const char* jsonWithAngles = { R"JSON(
            {
                "values": [
                    {
                        "$type": "MockRotationRule",
                        "rotation": { "yaw" : 45.0, "pitch" : 90.0, "roll" : 0.0 }
                    }
                ]
            })JSON"};

            SceneManifest loaded;
            auto loadFromStringResult = loaded.LoadFromString(jsonWithAngles, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(loadFromStringResult.IsSuccess());
            EXPECT_FALSE(loaded.IsEmpty());

            auto writeToJsonResult =
                SceneManifestContainer::SaveToJsonDocumentHelper(loaded, m_serializeContext.get(), m_jsonRegistrationContext.get());
            ASSERT_TRUE(writeToJsonResult.IsSuccess());

            AZStd::string jsonText;
            auto writeToStringResult = AzFramework::FileFunc::WriteJsonToString(writeToJsonResult.GetValue(), jsonText);
            ASSERT_TRUE(writeToStringResult.IsSuccess());
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("$type": "MockRotationRule")"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("rotation": [)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.27)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.65)"));
        }
    }
}
