/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneData/ReflectionRegistrar.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Behaviors/ScriptProcessorRuleBehavior.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Mocks/MockBehaviorUtils.h>

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/UnitTest/Mocks/MockSettingsRegistry.h>
#include <AzCore/UnitTest/TestTypes.h>

extern "C" AZ_DLL_EXPORT void CleanUpSceneCoreGenericClassInfo();
extern "C" AZ_DLL_EXPORT void CleanUpSceneDataGenericClassInfo();

namespace Testing
{
    struct SceneScriptTest
        : public UnitTest::AllocatorsFixture
    {
        static void TestExpectTrue(bool value)
        {
            EXPECT_TRUE(value);
        }

        static void TestEqualNumbers(AZ::s64 lhs, AZ::s64 rhs)
        {
            EXPECT_EQ(lhs, rhs);
        }

        static void TestEqualStrings(AZStd::string_view lhs, AZStd::string_view rhs)
        {
            EXPECT_STRCASEEQ(lhs.data(), rhs.data());
        }

        void ExpectExecute(AZStd::string_view script)
        {
            EXPECT_TRUE(m_scriptContext->Execute(script.data()));
        }

        void ReflectTypes(AZ::ReflectContext* context)
        {
            AZ::SceneAPI::Containers::Scene::Reflect(context);
            AZ::SceneAPI::Containers::SceneManifest::Reflect(context);
            AZ::SceneAPI::DataTypes::IManifestObject::Reflect(context);
            AZ::SceneAPI::Behaviors::ScriptProcessorRuleBehavior::Reflect(context);
            AZ::SceneAPI::Events::ExportProductList::Reflect(context);
        }

        void SetUp() override
        {
            UnitTest::AllocatorsFixture::SetUp();
            AZ::NameDictionary::Create();
            m_data.reset(new DataMembers);

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_serializeContext->Class<AZ::SceneAPI::DataTypes::IRule, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
            AZ::SceneAPI::RegisterDataTypeReflection(m_serializeContext.get());
            ReflectTypes(m_serializeContext.get());

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            ReflectTypes(m_behaviorContext.get());
            AZ::MathReflect(m_behaviorContext.get());
            m_behaviorContext->Method("TestExpectTrue", &TestExpectTrue);
            m_behaviorContext->Method("TestEqualNumbers", &TestEqualNumbers);
            m_behaviorContext->Method("TestEqualStrings", &TestEqualStrings);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_classes.find("Scene")->second->m_attributes);
            UnitTest::ScopeForUnitTest(m_behaviorContext->m_ebuses.find("ScriptBuildingNotificationBus")->second->m_attributes);

            m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
            m_scriptContext->BindTo(m_behaviorContext.get());

            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

            ON_CALL(m_data->m_settings, Get(::testing::Matcher<FixedValueString&>(::testing::_), testing::_))
                .WillByDefault([](FixedValueString& value, AZStd::string_view) -> bool
                    {
                        value = "mock_path";
                        return true;
                    });

            AZ::SettingsRegistry::Register(&m_data->m_settings);
        }

        void TearDown() override
        {
            m_scriptContext.reset();
            m_serializeContext.reset();
            m_behaviorContext.reset();

            AZ::SettingsRegistry::Unregister(&m_data->m_settings);
            m_data.reset();

            CleanUpSceneDataGenericClassInfo();
            CleanUpSceneCoreGenericClassInfo();

            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsFixture::TearDown();
        }

        struct DataMembers
        {
            int m_count = 0;
            AZ::NiceSettingsRegistrySimpleMock m_settings;
        };

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<DataMembers> m_data;
    };

    TEST_F(SceneScriptTest, Scene_ScriptBuildingNotificationBus_Exists)
    {
        ExpectExecute("TestExpectTrue(ScriptBuildingNotificationBus ~= nil)");
        ExpectExecute("self = {}");
        ExpectExecute("self.handler = ScriptBuildingNotificationBus.Connect(self)");
        ExpectExecute("TestExpectTrue(self.handler ~= nil)");
    }

    TEST_F(SceneScriptTest, Scene_ScriptBuildingNotificationBus_OnUpdateManifestCalled)
    {
        const char* handlerScript = R"LUA(
            local ScriptSample = {
                OnUpdateManifest = function (self, scene)
                    TestEqualStrings(scene.name, 'test')
                    return ''
                end
            }
            scene = Scene('test')
            ScriptSample.handler = ScriptBuildingNotificationBus.Connect(ScriptSample)
            manifest = ScriptBuildingNotificationBus.Broadcast.OnUpdateManifest(scene)
            ScriptSample.handler:Disconnect()
            )LUA";

        ExpectExecute(handlerScript);
    }

    TEST_F(SceneScriptTest, Scene_ScriptBuildingNotificationBus_OnUpdateManifestClearsHandler)
    {
        const char* handlerScript = R"LUA(
            local ScriptSample = {
                OnUpdateManifest = function (self, scene)
                    TestEqualStrings(scene.name, 'test')
                    self.handler:Disconnect()
                    self.handler = nil
                    return ''
                end
            }
            scene = Scene('test')
            ScriptSample.handler = ScriptBuildingNotificationBus.Connect(ScriptSample)
            manifest = ScriptBuildingNotificationBus.Broadcast.OnUpdateManifest(scene)
            )LUA";

        ExpectExecute(handlerScript);
        AZ::SceneAPI::Events::AssetPostImportRequestBus::ExecuteQueuedEvents();
    }
}
