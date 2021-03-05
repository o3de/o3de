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

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include "EntityTestbed.h"

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class EntityScriptTest
        : public EntityTestbed
    {
    public:

        ScriptContext* m_scriptContext;

        ~EntityScriptTest()
        {
        }

        void OnDestroy() override
        {
            delete m_scriptContext;
            m_scriptContext = nullptr;
        }

        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }

        void OnReflect(AZ::SerializeContext& context, AZ::Entity& systemEntity) override
        {
            (void)context;
            (void)systemEntity;
        }

        void OnSetup() override
        {
            m_scriptContext = aznew AZ::ScriptContext();

            auto* catalogBus = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (catalogBus)
            {
                // Register asset types the asset DB should query our catalog for.
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());

                // Build the catalog (scan).
                catalogBus->AddExtension(".lua");
            }
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            // Add your components.
            entity.CreateComponent<AzToolsFramework::Components::ScriptEditorComponent>();
            entity.Activate();
        }
    };

    TEST_F(EntityScriptTest, DISABLED_Test)
    {
        run();
    }

    class ScriptComponentTest
        : public ::testing::Test
    {
        static int mySubValue;
        static int myReloadValue;

    public:
        void run()
        {
            {
                ComponentApplication app;
                ComponentApplication::Descriptor appDesc;
                appDesc.m_memoryBlocksByteSize = 100 * 1024 * 1024;
                //appDesc.m_recordsMode = AllocationRecords::RECORD_FULL;
                //appDesc.m_stackRecordLevels = 20;
                Entity* systemEntity = app.Create(appDesc);

                systemEntity->CreateComponent<MemoryComponent>();
                systemEntity->CreateComponent("{CAE3A025-FAC9-4537-B39E-0A800A2326DF}"); // JobManager component
                systemEntity->CreateComponent<StreamerComponent>();
                systemEntity->CreateComponent<AssetManagerComponent>();
                systemEntity->CreateComponent("{A316662A-6C3E-43E6-BC61-4B375D0D83B4}"); // Usersettings component
                systemEntity->CreateComponent<ScriptSystemComponent>();

                systemEntity->Init();
                systemEntity->Activate();

                // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
                // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
                // in the unit tests.
                AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

                ScriptComponent::CreateDescriptor(); // descriptor is deleted by app

                ScriptContext* scriptContext = nullptr;
                EBUS_EVENT_RESULT(scriptContext, ScriptSystemRequestBus, GetContext, DefaultScriptContextId);

                BehaviorContext* behaviorContext = nullptr;
                EBUS_EVENT_RESULT(behaviorContext, AZ::ComponentApplicationBus, GetBehaviorContext);

                // make sure script instances don't can read only share data, but don't modify the source table
                {
                    Data::Asset<ScriptAsset> scriptAsset = Data::AssetManager::Instance().CreateAsset<ScriptAsset>(Uuid::CreateRandom());
                    AZStd::string script = "test = {\
                                            --[[test with no properties table as this should work too!]]\
                                            state = {\
                                                mysubstate = {\
                                                   mysubvalue = 2,\
                                                },\
                                                myvalue = 0,\
                                              },\
                                            }\
                                            function test:OnActivate()\
                                              self.state.mysubstate.mysubvalue = 5\
                                            end\
                                            return test;";
                    scriptAsset.Get()->m_scriptBuffer.insert(scriptAsset.Get()->m_scriptBuffer.begin(), script.begin(), script.end());
                    EBUS_EVENT(Data::AssetManagerBus, OnAssetReady, scriptAsset);
                    app.Tick();
                    app.TickSystem();

                    Entity* entity1 = aznew Entity();
                    entity1->CreateComponent<ScriptComponent>()->SetScript(scriptAsset);

                    entity1->Init();
                    entity1->Activate();

                    Entity* entity2 = aznew Entity();
                    entity2->CreateComponent<ScriptComponent>()->SetScript(scriptAsset);

                    entity2->Init();
                    entity2->Activate();

                    behaviorContext->Property("globalMySubValue", BehaviorValueProperty(&mySubValue));
                    scriptContext->Execute("globalMySubValue = test.state.mysubstate.mysubvalue", "Read my subvalue");
                    AZ_TEST_ASSERT(mySubValue == 2); // we should not have changed test. table but the instance table of each component.

                    delete entity1;
                    delete entity2;

                    scriptAsset.Reset();
                }

                // Test script reload
                {
                    behaviorContext->Property("myReloadValue", BehaviorValueProperty(&myReloadValue));
                    Data::Asset<ScriptAsset> scriptAsset1 = Data::AssetManager::Instance().CreateAsset<ScriptAsset>(Uuid::CreateRandom());
                    AZStd::string script1 ="local testReload = {}\
                                            function testReload:OnActivate()\
                                              myReloadValue = 1\
                                            end\
                                            function testReload:OnDeactivate()\
                                              myReloadValue = 0\
                                            end\
                                            return testReload;";

                    scriptAsset1.Get()->m_scriptBuffer.insert(scriptAsset1.Get()->m_scriptBuffer.begin(), script1.begin(), script1.end());
                    EBUS_EVENT(Data::AssetManagerBus, OnAssetReady, scriptAsset1);

                    app.Tick();
                    app.TickSystem(); // flush assets etc.

                    Entity* entity = aznew Entity();
                    entity->CreateComponent<ScriptComponent>()->SetScript(scriptAsset1);

                    entity->Init();
                    entity->Activate();

                    // test value, it should set during activation of the first script
                    AZ_TEST_ASSERT(myReloadValue == 1);

                    AZStd::string script2 ="local testReload = {}\
                                            function testReload:OnActivate()\
                                               myReloadValue = 5\
                                            end\
                                            return testReload";

                    // modify the asset
                    Data::Asset<ScriptAsset> scriptAsset2(aznew ScriptAsset(scriptAsset1.GetId()), AZ::Data::AssetLoadBehavior::Default);
                    scriptAsset2.Get()->m_scriptBuffer.insert(scriptAsset2.Get()->m_scriptBuffer.begin(), script2.begin(), script2.end());

                    // When reloading script assets from files, ScriptSystemComponent would clear old script caches automatically in the
                    // function `ScriptSystemComponent::LoadAssetData()`. But here we are changing script directly in memory, therefore we 
                    // need to clear old cache manually.
                    AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequestBus::Events::ClearAssetReferences, scriptAsset1.GetId());

                    // trigger reload
                    Data::AssetManager::Instance().ReloadAssetFromData(scriptAsset2);
                    
                    // ReloadAssetFromData is (now) a queued event
                    // Need to tick subsystems here to receive reload event.
                    app.Tick();
                    app.TickSystem();

                    // test value with the reloaded value
                    EXPECT_EQ(5, myReloadValue);

                    delete entity;

                    scriptAsset1.Reset();
                    scriptAsset2.Reset();
                }

                app.Destroy();
                //////////////////////////////////////////////////////////////////////////
            }
        }
    };

    TEST_F(ScriptComponentTest, ScriptComponentTestExecution)
    {
        run();
    }

    int ScriptComponentTest::mySubValue = 0;
    int ScriptComponentTest::myReloadValue = 0;
} // namespace UnitTest
