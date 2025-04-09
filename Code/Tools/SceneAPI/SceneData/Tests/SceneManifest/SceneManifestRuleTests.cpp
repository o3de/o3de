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

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/UnitTest/Mocks/MockSettingsRegistry.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

extern "C" AZ_DLL_EXPORT void CleanUpSceneCoreGenericClassInfo();
extern "C" AZ_DLL_EXPORT void CleanUpSceneDataGenericClassInfo();

namespace AZ
{
    class MockEditorPythonEventsInterface
        : public AzToolsFramework::EditorPythonEventsInterface
    {
    public:
        MOCK_METHOD1(StartPython, bool(bool));
        MOCK_METHOD1(StopPython, bool(bool));
        MOCK_METHOD0(IsPythonActive, bool());
        MOCK_METHOD0(WaitForInitialization, void());
        MOCK_METHOD1(ExecuteWithLock, void(AZStd::function<void()>));
        MOCK_METHOD1(TryExecuteWithLock, bool(AZStd::function<void()>));
    };

    class MockEditorPythonRunnerRequestBus
        : public AzToolsFramework::EditorPythonRunnerRequestBus::Handler
    {
    public:
        MockEditorPythonRunnerRequestBus()
        {
            AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusConnect();
        }

        ~MockEditorPythonRunnerRequestBus()
        {
            AzToolsFramework::EditorPythonRunnerRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(ExecuteByString, void(AZStd::string_view, bool));
        MOCK_METHOD1(ExecuteByFilename, bool(AZStd::string_view));
        MOCK_METHOD2(ExecuteByFilenameWithArgs, bool(AZStd::string_view, const AZStd::vector<AZStd::string_view>&));
        MOCK_METHOD3(ExecuteByFilenameAsTest, bool(AZStd::string_view, AZStd::string_view, const AZStd::vector<AZStd::string_view>&));
    };

    using NiceEditorPythonEventsInterfaceMock = ::testing::NiceMock<MockEditorPythonEventsInterface>;
    using NiceMockEditorPythonRunnerRequestBus = ::testing::NiceMock<MockEditorPythonRunnerRequestBus>;
    using NiceMockFileIOBase = ::testing::NiceMock<AZ::IO::MockFileIOBase>;

    namespace SceneAPI
    {
        class MockRotationRule final
            : public DataTypes::IManifestObject
        {
        public:
            AZ_RTTI(MockRotationRule, "{90AECE4A-58D4-411C-9CDE-59B54C59354F}", DataTypes::IManifestObject);
            AZ_CLASS_ALLOCATOR(MockRotationRule, AZ::SystemAllocator);

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
            : public UnitTest::LeakDetectionFixture
        {
        public:
            AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
            AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
            AZStd::unique_ptr<JsonSystemComponent> m_jsonSystemComponent;

            void SetUp() override
            {
                UnitTest::LeakDetectionFixture::SetUp();
                AZ::NameDictionary::Create();

                m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
                AZ::SceneAPI::RegisterDataTypeReflection(m_serializeContext.get());
                AZ::SceneAPI::Containers::SceneManifest::Reflect(m_serializeContext.get());
                AZ::SceneAPI::DataTypes::IManifestObject::Reflect(m_serializeContext.get());
                m_serializeContext->Class<AZ::SceneAPI::DataTypes::IRule, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
                AZ::SceneAPI::MockRotationRule::Reflect(m_serializeContext.get());

                m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

                m_jsonSystemComponent = AZStd::make_unique<JsonSystemComponent>();
                m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());

                m_data.reset(new DataMembers);
                m_data->m_settings = AZStd::make_unique<AZ::NiceSettingsRegistrySimpleMock>();

                using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

                ON_CALL(*m_data->m_settings, Get(::testing::Matcher<FixedValueString&>(::testing::_), testing::_))
                    .WillByDefault([](FixedValueString& value, AZStd::string_view) -> bool
                    {
                        value = "mock_path";
                        return true;
                    });

                AZ::SettingsRegistry::Register(m_data->m_settings.get());

                SetupFileBaseIO();
            }

            void TearDown() override
            {
                TeardownFileBaseIO();

                if (m_data->m_editorPythonEventsInterfacePrepared)
                {
                    Interface<AzToolsFramework::EditorPythonEventsInterface>::Unregister(&m_data->m_editorPythonEventsInterface);
                }

                m_jsonRegistrationContext->EnableRemoveReflection();
                m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
                m_jsonRegistrationContext->DisableRemoveReflection();

                m_serializeContext.reset();
                m_jsonRegistrationContext.reset();
                m_jsonSystemComponent.reset();

                if (m_data->m_settings)
                {
                    AZ::SettingsRegistry::Unregister(m_data->m_settings.get());
                    m_data->m_settings.reset();
                }
                m_data.reset();

                CleanUpSceneCoreGenericClassInfo();
                CleanUpSceneDataGenericClassInfo();

                AZ::NameDictionary::Destroy();
                UnitTest::LeakDetectionFixture::TearDown();
            }

            void PrepareMockPythonInterface()
            {
                ON_CALL(m_data->m_editorPythonEventsInterface, StartPython(::testing::_)).WillByDefault(::testing::Return(true));
                ON_CALL(m_data->m_editorPythonEventsInterface, StopPython(::testing::_)).WillByDefault(::testing::Return(true));
                ON_CALL(m_data->m_editorPythonEventsInterface, IsPythonActive()).WillByDefault(::testing::Return(true));

                ON_CALL(m_data->m_editorPythonEventsInterface, ExecuteWithLock(::testing::_))
                    .WillByDefault([](auto callback)
                    {
                        callback();
                    });

                ON_CALL(m_data->m_editorPythonEventsInterface, TryExecuteWithLock(::testing::_))
                    .WillByDefault([](auto callback) -> bool
                    {
                        callback();
                        return true;
                    });

                Interface<AzToolsFramework::EditorPythonEventsInterface>::Register(&m_data->m_editorPythonEventsInterface);
                m_data->m_editorPythonEventsInterfacePrepared = true;
            }

            void SetupFileBaseIO()
            {
                m_data->m_fileIOMock = AZStd::make_unique<NiceMockFileIOBase>();
                m_data->m_prevFileIO = IO::FileIOBase::GetInstance();
                IO::FileIOBase::SetInstance(nullptr);
                IO::FileIOBase::SetInstance(m_data->m_fileIOMock.get());
            }

            void TeardownFileBaseIO()
            {
                if (m_data->m_fileIOMock)
                {
                    IO::FileIOBase::SetInstance(nullptr);
                    IO::FileIOBase::SetInstance(m_data->m_prevFileIO);
                    m_data->m_fileIOMock.reset();
                }
            }

            struct DataMembers
            {
                NiceMockEditorPythonRunnerRequestBus m_niceMockEditorPythonRunnerRequestBus;
                AZStd::unique_ptr<NiceSettingsRegistrySimpleMock> m_settings;
                NiceEditorPythonEventsInterfaceMock m_editorPythonEventsInterface;
                bool m_editorPythonEventsInterfacePrepared = false;
                AZStd::unique_ptr<NiceMockFileIOBase> m_fileIOMock;
                IO::FileIOBase* m_prevFileIO = nullptr;
            };

            AZStd::unique_ptr<DataMembers> m_data;
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
            auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
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
            auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
            ASSERT_TRUE(writeToStringResult.IsSuccess());
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("$type": "MockRotationRule")"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("rotation": [)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.27)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.65)"));
        }

        TEST_F(SceneManifest_JSON, LoadFromString_CoordinateSystemRule_ReturnsTrue)
        {
            AZ::SceneAPI::SceneData::CoordinateSystemRule foo;
            EXPECT_FALSE(foo.GetUseAdvancedData());

            using namespace SceneAPI::Containers;

            constexpr const char* jsonCoordinateSystemRule = { R"JSON(
            {
                "values": [
                    {
                        "$type": "CoordinateSystemRule",
                        "useAdvancedData": true,
                        "originNodeName": "test_origin_name",
                        "translation": [1.0, 2.0, 3.0],
                        "rotation": { "yaw" : 45.0, "pitch" : 18.5, "roll" : 215.0 },
                        "scale": 10.0
                    }
                ]
            })JSON" };

            SceneManifest loaded;
            auto loadFromStringResult = loaded.LoadFromString(jsonCoordinateSystemRule, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(loadFromStringResult.IsSuccess());
            EXPECT_FALSE(loaded.IsEmpty());

            auto writeToJsonResult =
                SceneManifestContainer::SaveToJsonDocumentHelper(loaded, m_serializeContext.get(), m_jsonRegistrationContext.get());
            ASSERT_TRUE(writeToJsonResult.IsSuccess());

            AZStd::string jsonText;
            auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
            ASSERT_TRUE(writeToStringResult.IsSuccess());
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("$type": "CoordinateSystemRule")"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("useAdvancedData": true,)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("originNodeName": "test_origin_name",)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("rotation": [)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.028)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(-0.40)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(0.85)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(-0.33)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("translation": [)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(1.0)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(2.0)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"(3.0)"));
            EXPECT_THAT(jsonText.c_str(), ::testing::HasSubstr(R"("scale": 10.0)"));
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_LoadWithEmptyScriptFilename_ReturnsEarly)
        {
            using namespace SceneAPI::Containers;
            using namespace SceneAPI::Events;

            constexpr const char* jsonManifest = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": ""
                    }
                ]
            })JSON" };

            auto scene = AZ::SceneAPI::Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            auto result = scene.GetManifest().LoadFromString(jsonManifest, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_FALSE(scene.GetManifest().IsEmpty());

            auto scriptProcessorRuleBehavior =  AZ::SceneAPI::Behaviors::ScriptProcessorRuleBehavior();
            scriptProcessorRuleBehavior.Activate();
            auto update = scriptProcessorRuleBehavior.UpdateManifest(scene, AssetImportRequest::Update, AssetImportRequest::Generic);
            scriptProcessorRuleBehavior.Deactivate();
            EXPECT_EQ(update, ProcessingResult::Ignored);
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_DefaultFallbackLogic_Works)
        {
            using namespace AZ::SceneAPI;

            constexpr const char* defaultJson = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": "foo.py"
                    }
                ]
            })JSON" };

            auto scene = Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            auto result = scene.GetManifest().LoadFromString(defaultJson, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_FALSE(scene.GetManifest().IsEmpty());
            ASSERT_EQ(scene.GetManifest().GetEntryCount(), 1);

            auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(scene.GetManifest().GetValueStorage());
            EXPECT_EQ(view.begin()->GetScriptProcessorFallbackLogic(), DataTypes::ScriptProcessorFallbackLogic::FailBuild);
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_ExplicitFallbackLogic_Works)
        {
            using namespace AZ::SceneAPI;

            constexpr const char* fallbackLogicJson = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": "foo.py",
                        "fallbackLogic": "FailBuild"
                    }
                ]
            })JSON" };

            auto scene = Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            auto result = scene.GetManifest().LoadFromString(fallbackLogicJson, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_FALSE(scene.GetManifest().IsEmpty());
            ASSERT_EQ(scene.GetManifest().GetEntryCount(), 1);

            auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(scene.GetManifest().GetValueStorage());
            EXPECT_EQ(view.begin()->GetScriptProcessorFallbackLogic(), DataTypes::ScriptProcessorFallbackLogic::FailBuild);
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_ContinueBuildFallbackLogic_Works)
        {
            using namespace AZ::SceneAPI;

            constexpr const char* fallbackLogicJson = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": "foo.py",
                        "fallbackLogic": "ContinueBuild"
                    }
                ]
            })JSON" };

            auto scene = Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            auto result = scene.GetManifest().LoadFromString(fallbackLogicJson, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_FALSE(scene.GetManifest().IsEmpty());
            ASSERT_EQ(scene.GetManifest().GetEntryCount(), 1);

            auto view = Containers::MakeDerivedFilterView<DataTypes::IScriptProcessorRule>(scene.GetManifest().GetValueStorage());
            EXPECT_EQ(view.begin()->GetScriptProcessorFallbackLogic(), DataTypes::ScriptProcessorFallbackLogic::ContinueBuild);
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_ScriptLogic_CallsIntoPythonInterface)
        {
            using namespace SceneAPI::Containers;
            using namespace SceneAPI::Events;

            constexpr const char* jsonManifest = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": "mock_update_manifest.py"
                    }
                ]
            })JSON" };

            PrepareMockPythonInterface();
            EXPECT_CALL(m_data->m_editorPythonEventsInterface, IsPythonActive()).Times(1);
            EXPECT_CALL(m_data->m_editorPythonEventsInterface, ExecuteWithLock(testing::_)).Times(1);

            ON_CALL(*m_data->m_fileIOMock.get(), Exists(::testing::_))
                .WillByDefault([](const char*) -> bool
                {
                    return true;
                }
            );
            EXPECT_CALL(*m_data->m_fileIOMock.get(), Exists(testing::_)).Times(1);

            bool executeByFilenameCalled = false;
            ON_CALL(m_data->m_niceMockEditorPythonRunnerRequestBus, ExecuteByFilename(::testing::_))
                .WillByDefault([&executeByFilenameCalled](AZStd::string_view)
                {
                    executeByFilenameCalled = true;
                    return true;
                }
            );
            EXPECT_CALL(m_data->m_niceMockEditorPythonRunnerRequestBus, ExecuteByFilename(testing::_)).Times(1);

            auto scene = AZ::SceneAPI::Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            auto result = scene.GetManifest().LoadFromString(jsonManifest, m_serializeContext.get(), m_jsonRegistrationContext.get());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_FALSE(scene.GetManifest().IsEmpty());

            auto scriptProcessorRuleBehavior = AZ::SceneAPI::Behaviors::ScriptProcessorRuleBehavior();
            scriptProcessorRuleBehavior.Activate();
            scriptProcessorRuleBehavior.UpdateManifest(scene, AssetImportRequest::Update, AssetImportRequest::Generic);
            scriptProcessorRuleBehavior.Deactivate();
            EXPECT_TRUE(executeByFilenameCalled);
        }

        TEST_F(SceneManifest_JSON, ScriptProcessorRule_EditorPythonEventsInterface_RunsWithEditorPythonEventsInterfaceCleared)
        {
            using namespace SceneAPI::Containers;
            using namespace SceneAPI::Events;

            constexpr const char* jsonManifest = { R"JSON(
            {
                "values": [
                    {
                        "$type": "ScriptProcessorRule",
                        "scriptFilename": ""
                    }
                ]
            })JSON" };

            PrepareMockPythonInterface();
            EXPECT_CALL(m_data->m_editorPythonEventsInterface, IsPythonActive()).Times(1);
            EXPECT_CALL(m_data->m_editorPythonEventsInterface, StartPython(testing::_)).Times(0);
            EXPECT_CALL(m_data->m_editorPythonEventsInterface, StopPython(testing::_)).Times(1);

            auto scene = AZ::SceneAPI::Containers::Scene("mock");
            scene.SetManifestFilename("mock.fake.assetinfo");
            scene.GetManifest().LoadFromString(jsonManifest, m_serializeContext.get(), m_jsonRegistrationContext.get());

            auto scriptProcessorRuleBehavior = AZ::SceneAPI::Behaviors::ScriptProcessorRuleBehavior();
            scriptProcessorRuleBehavior.Activate();
            auto update = scriptProcessorRuleBehavior.UpdateManifest(scene, AssetImportRequest::Update, AssetImportRequest::Generic);
            EXPECT_EQ(update, ProcessingResult::Ignored);
            AZ::SettingsRegistry::Unregister(m_data->m_settings.get());
            m_data->m_settings.reset();
            scriptProcessorRuleBehavior.Deactivate();
        }
    }
}
