/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>
#include <AzToolsFramework/ToolsComponents/ScriptEditorComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include "EntityTestbed.h"

extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    // Global Properties used for Testing
    int mySubValue = 0;
    int myReloadValue = 0;

    namespace
    {
        constexpr AZ::Crc32 SupportedAssetTypesAttribute = AZ_CRC_CE("SupportedAssetTypes");

        class TestAssetTypeInfo
            : public AZ::AssetTypeInfoBus::Handler
        {
        public:
            TestAssetTypeInfo(AZ::Data::AssetType assetType, const char* displayName)
                : m_assetType(assetType)
                , m_displayName(displayName)
            {
                BusConnect(m_assetType);
            }

            ~TestAssetTypeInfo() override
            {
                BusDisconnect();
            }

            AZ::Data::AssetType GetAssetType() const override
            {
                return m_assetType;
            }

            const char* GetAssetTypeDisplayName() const override
            {
                return m_displayName;
            }

        private:
            AZ::Data::AssetType m_assetType;
            const char* m_displayName;
        };

        class AssetIdScriptPropertyStub
            : public AZ::ScriptProperty
        {
        public:
            const void* GetDataAddress() const override
            {
                return nullptr;
            }

            AZ::TypeId GetDataTypeUuid() const override
            {
                return azrtti_typeid<AZ::Data::AssetId>();
            }

            AZ::ScriptProperty* Clone([[maybe_unused]] const char* name = nullptr) const override
            {
                return nullptr;
            }

            bool Write([[maybe_unused]] AZ::ScriptContext& context) override
            {
                return false;
            }

        protected:
            void CloneDataFrom([[maybe_unused]] const AZ::ScriptProperty* scriptProperty) override
            {
            }
        };

        class TestScriptEditorComponent
            : public AzToolsFramework::Components::ScriptEditorComponent
        {
        public:
            using ScriptEditorComponent::LoadAttribute;
        };

        AZStd::vector<AZ::Data::AssetType> ReadSupportedAssetTypes(const AZ::Edit::ElementData& editData)
        {
            AZStd::vector<AZ::Data::AssetType> assetTypes;
            AZ::Attribute* attribute = editData.FindAttribute(SupportedAssetTypesAttribute);
            if (attribute)
            {
                AZ::AttributeReader reader(nullptr, attribute);
                reader.Read<AZStd::vector<AZ::Data::AssetType>>(assetTypes);
            }
            return assetTypes;
        }
    } // namespace

    class ScriptComponentTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        AZ_TYPE_INFO(ScriptComponentTest, "{85CDBD49-70FF-416A-8154-B5525EDD30D4}");

        void SetUp() override
        {        
            ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 100 * 1024 * 1024;
            //appDesc.m_recordsMode = AllocationRecords::RECORD_FULL;
            //appDesc.m_stackRecordLevels = 20;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            Entity* systemEntity = m_app.Create(appDesc, startupParameters);

            systemEntity->CreateComponent(AZ::TypeId{ "{CAE3A025-FAC9-4537-B39E-0A800A2326DF}" }); // JobManager component
            systemEntity->CreateComponent<TaskGraphSystemComponent>();
            systemEntity->CreateComponent<StreamerComponent>();
            systemEntity->CreateComponent<AssetManagerComponent>();
            systemEntity->CreateComponent(AZ::TypeId{ "{A316662A-6C3E-43E6-BC61-4B375D0D83B4}" }); // Usersettings component
            systemEntity->CreateComponent<ScriptSystemComponent>();

            systemEntity->Init();
            systemEntity->Activate();

            ScriptSystemRequestBus::BroadcastResult(m_scriptContext, &ScriptSystemRequestBus::Events::GetContext, DefaultScriptContextId);
            AZ::ComponentApplicationBus::BroadcastResult(m_behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            AzToolsFramework::Components::ScriptEditorComponent::CreateDescriptor(); // descriptor is deleted by app
            AzToolsFramework::Components::ScriptEditorComponent::Reflect(m_serializeContext);

            ScriptComponent::CreateDescriptor(); // descriptor is deleted by app
            ScriptComponent::Reflect(m_serializeContext);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        AZStd::optional<Data::Asset<ScriptAsset>> CreateAndLoadScriptAsset(const AZStd::string& script, ScriptContext& scriptContext, Uuid id = Uuid::CreateRandom())
        {
            AzFramework::ScriptCompileRequest compileRequest;
            compileRequest.m_errorWindow = "LuaTests";
            AZ::IO::MemoryStream inputStream(script.data(), script.size());
            compileRequest.m_input = &inputStream;

            if (CompileScript(compileRequest, scriptContext))
            {
                Data::Asset<ScriptAsset> scriptAsset = Data::AssetManager::Instance().CreateAsset<ScriptAsset>(Data::AssetId(id));
                scriptAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
                scriptAsset.Get()->m_data = compileRequest.m_luaScriptDataOut;
                Data::AssetManagerBus::Broadcast(&Data::AssetManagerBus::Events::OnAssetReady, scriptAsset);
                m_app.Tick();
                m_app.TickSystem(); // flush assets etc.

                return scriptAsset;
            }

            return AZStd::nullopt;
        }

        static ScriptComponent* BuildGameEntity(const Data::Asset<ScriptAsset>& scriptAsset, Entity& gameEntity)
        {
            // We first setup the ScriptEditorComponent.
            // After a script asset is loaded the ScriptEditorComponent builds the properties table.
            // BuildGameEntity() hands off the properties table to the game runtime ScriptComponent.
            Entity editorEntity;
            auto* scriptEditorComponent = editorEntity.CreateComponent<AzToolsFramework::Components::ScriptEditorComponent>();
            scriptEditorComponent->SetScript(scriptAsset);
            editorEntity.Init();
            editorEntity.Activate();
            scriptEditorComponent->LoadScript();
            scriptEditorComponent->BuildGameEntity(&gameEntity);

            auto* scriptComponent = gameEntity.FindComponent<ScriptComponent>();
            return scriptComponent;
        }

        bool LoadAssetTypeAttributes(const AZStd::string& attributes, AZ::Edit::ElementData& editData)
        {
            const AZStd::string script = AZStd::string::format("AssetTypeAttributeTest = %s", attributes.c_str());
            if (!m_scriptContext->Execute(script.c_str(), "AssetTypeAttributeTest"))
            {
                return false;
            }

            AZ::ScriptDataContext attributeTable;
            if (!m_scriptContext->InspectTable("AssetTypeAttributeTest", attributeTable))
            {
                return false;
            }

            TestScriptEditorComponent component;
            AssetIdScriptPropertyStub property;
            bool result = true;
            const char* attributeName = nullptr;
            int attributeFieldIndex = 0;
            int attributeIndex = 0;
            while (attributeTable.InspectNextElement(attributeIndex, attributeName, attributeFieldIndex))
            {
                result = component.LoadAttribute(attributeTable, attributeIndex, attributeName, editData, &property) && result;
            }
            return result;
        }

        ComponentApplication m_app;
        ScriptContext* m_scriptContext = nullptr;
        BehaviorContext* m_behaviorContext = nullptr;
        SerializeContext* m_serializeContext = nullptr;
    };


    TEST_F(ScriptComponentTest, ScriptInstancesCanReadButDontModifySourceTable)
    {
        // make sure script instances don't can read only share data, but don't modify the source table
        const AZStd::string script = "test = {\
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

        auto scriptAssetOpt = CreateAndLoadScriptAsset(script, *m_scriptContext);
        AZ_TEST_ASSERT(scriptAssetOpt);
        auto& scriptAsset = *scriptAssetOpt;

        auto* entity1 = aznew Entity();
        entity1->CreateComponent<ScriptComponent>()->SetScript(scriptAsset);

        entity1->Init();
        entity1->Activate();

        auto* entity2 = aznew Entity();
        entity2->CreateComponent<ScriptComponent>()->SetScript(scriptAsset);

        entity2->Init();
        entity2->Activate();

        m_behaviorContext->Property("globalMySubValue", BehaviorValueProperty(&mySubValue));
        m_scriptContext->Execute("globalMySubValue = test.state.mysubstate.mysubvalue", "Read my subvalue");
        AZ_TEST_ASSERT(mySubValue == 2); // we should not have changed test. table but the instance table of each component.

        delete entity1;
        delete entity2;
    }


    TEST_F(ScriptComponentTest, ScriptReloads)
    {
        // Test script reload
        m_behaviorContext->Property("myReloadValue", BehaviorValueProperty(&myReloadValue));
        const AZStd::string script1 ="local testReload = {}\
                                function testReload:OnActivate()\
                                  myReloadValue = 1\
                                end\
                                function testReload:OnDeactivate()\
                                  myReloadValue = 0\
                                end\
                                return testReload;";
        auto scriptAssetOpt = CreateAndLoadScriptAsset(script1, *m_scriptContext);
        AZ_TEST_ASSERT(scriptAssetOpt);
        auto& scriptAsset1 = *scriptAssetOpt;

        auto* entity = aznew Entity();
        entity->CreateComponent<ScriptComponent>()->SetScript(scriptAsset1);

        entity->Init();
        entity->Activate();

        // test value, it should set during activation of the first script
        AZ_TEST_ASSERT(myReloadValue == 1);

        const AZStd::string script2 ="local testReload = {}\
                                myReloadValue = 5\
                                return testReload";

        // modify the asset, re-use the previous ID
        Data::Asset<ScriptAsset> scriptAsset2(aznew ScriptAsset(scriptAsset1.GetId()), AZ::Data::AssetLoadBehavior::Default);
        {
            AzFramework::ScriptCompileRequest compileRequest;
            compileRequest.m_errorWindow = "LuaTests";
            AZ::IO::MemoryStream inputStream(script2.data(), script2.size());
            compileRequest.m_input = &inputStream;

            if (CompileScript(compileRequest, *m_scriptContext))
            {
                scriptAsset2.Get()->m_data = compileRequest.m_luaScriptDataOut;
            }
        }

        // When reloading script assets from files, ScriptSystemComponent would clear old script caches automatically in the
        // function `ScriptSystemComponent::LoadAssetData()`. But here we are changing script directly in memory, therefore we 
        // need to clear old cache manually.
        AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequestBus::Events::ClearAssetReferences, scriptAsset1.GetId());

        // trigger reload
        Data::AssetManager::Instance().ReloadAssetFromData(scriptAsset2);
        
        // ReloadAssetFromData is (now) a queued event
        // Need to tick subsystems here to receive reload event.
        m_app.Tick();
        m_app.TickSystem();

        // test value with the reloaded value
        EXPECT_EQ(5, myReloadValue);

        delete entity;
    }

    TEST_F(ScriptComponentTest, LuaPropertiesAreDiscovered)
    {
        const AZStd::string script = "local test = {\
                                      Properties = {\
                                        myNum = { default = 2 },\
                                      },\
                                    }\
                                    function test:OnActivate()\
                                      self.Properties.myNum = 5\
                                    end\
                                    return test";

        auto scriptAssetOpt = CreateAndLoadScriptAsset(script, *m_scriptContext);
        AZ_TEST_ASSERT(scriptAssetOpt);
        auto& scriptAsset = *scriptAssetOpt;
        Entity editorEntity, gameEntity;
        auto* scriptComponent = BuildGameEntity(scriptAsset, gameEntity);
        EXPECT_NE(scriptComponent->GetScriptProperty("myNum"), nullptr);
    }

    TEST_F(ScriptComponentTest, LuaAssetIdPropertyAcceptsOneAssetTypeDisplayName)
    {
        const AZ::Data::AssetType modelAssetType{ "{8F38B502-014C-4E46-A867-20CB7BC24954}" };
        TestAssetTypeInfo modelAssetTypeInfo(modelAssetType, "ModelAsset");
        AZ::Edit::ElementData editData;

        EXPECT_TRUE(LoadAssetTypeAttributes("{ assetType = 'ModelAsset' }", editData));
        EXPECT_THAT(ReadSupportedAssetTypes(editData), ::testing::ElementsAre(modelAssetType));
        ASSERT_NE(editData.FindAttribute(SupportedAssetTypesAttribute), nullptr);
        EXPECT_FALSE(editData.FindAttribute(SupportedAssetTypesAttribute)->m_describesChildren);

        editData.ClearAttributes();
    }

    TEST_F(ScriptComponentTest, LuaAssetIdPropertyAcceptsBetweenOneAndEightAssetTypes)
    {
        const AZStd::array<AZ::Data::AssetType, 8> assetTypes = {
            AZ::Data::AssetType{ "{5091E379-381F-4603-8941-9AC13550D6FB}" },
            AZ::Data::AssetType{ "{A6DF59CF-6D35-4FB3-AE0C-631144C88A07}" },
            AZ::Data::AssetType{ "{A0B281BB-2835-46AD-A3AE-5E1D07468320}" },
            AZ::Data::AssetType{ "{C5AC91D0-D70F-4305-93F7-3CC00FA243F7}" },
            AZ::Data::AssetType{ "{C9E956F0-AD2B-4C14-9493-F9E89538A60F}" },
            AZ::Data::AssetType{ "{52CC79AD-1D7C-4B83-A59B-7089ABBD9E82}" },
            AZ::Data::AssetType{ "{E1D10AD6-4CF5-418D-9A16-82FE01A6A609}" },
            AZ::Data::AssetType{ "{4437603D-FBD6-4B59-BA2B-11554A4C3425}" }
        };
        TestAssetTypeInfo firstTypeInfo(assetTypes[0], "TestAsset1");
        TestAssetTypeInfo secondTypeInfo(assetTypes[1], "TestAsset2");
        TestAssetTypeInfo thirdTypeInfo(assetTypes[2], "TestAsset3");
        TestAssetTypeInfo fourthTypeInfo(assetTypes[3], "TestAsset4");
        TestAssetTypeInfo fifthTypeInfo(assetTypes[4], "TestAsset5");
        TestAssetTypeInfo sixthTypeInfo(assetTypes[5], "TestAsset6");
        TestAssetTypeInfo seventhTypeInfo(assetTypes[6], "TestAsset7");
        TestAssetTypeInfo eighthTypeInfo(assetTypes[7], "TestAsset8");

        for (size_t typeCount = 1; typeCount <= assetTypes.size(); ++typeCount)
        {
            SCOPED_TRACE(typeCount);
            AZStd::string attribute = "{ assetType = {";
            for (size_t index = 0; index < typeCount; ++index)
            {
                attribute += AZStd::string::format("'TestAsset%zu',", index + 1);
            }
            attribute += "} }";

            AZ::Edit::ElementData editData;
            EXPECT_TRUE(LoadAssetTypeAttributes(attribute, editData));
            const AZStd::vector<AZ::Data::AssetType> actualAssetTypes = ReadSupportedAssetTypes(editData);
            ASSERT_EQ(actualAssetTypes.size(), typeCount);
            EXPECT_TRUE(AZStd::equal(actualAssetTypes.begin(), actualAssetTypes.end(), assetTypes.begin()));
            editData.ClearAttributes();
        }
    }

    TEST_F(ScriptComponentTest, LuaAssetIdPropertyRejectsInvalidAssetTypeFilters)
    {
        const AZ::Data::AssetType firstAssetType{ "{72C86438-A410-41D5-A927-854799F62DC0}" };
        const AZ::Data::AssetType secondAssetType{ "{48415517-F909-4F5F-B9AA-57073A6B3090}" };
        TestAssetTypeInfo firstTypeInfo(firstAssetType, "TestAsset1");
        TestAssetTypeInfo secondTypeInfo(secondAssetType, "TestAsset2");
        TestAssetTypeInfo ambiguousTypeInfo(secondAssetType, "TestAsset1");

        const AZStd::array<const char*, 6> invalidAttributes = {
            "{ assetType = 'MissingAsset' }",
            "{ assetType = 'TestAsset1' }",
            "{ assetType = {} }",
            "{ assetType = {'TestAsset2', 'TestAsset2'} }",
            "{ assetType = {'TestAsset1', 'TestAsset2', 'TestAsset3', 'TestAsset4', 'TestAsset5', 'TestAsset6', 'TestAsset7', 'TestAsset8', 'TestAsset9'} }",
            "{ assetType = 42 }"
        };

        for (const char* attribute : invalidAttributes)
        {
            SCOPED_TRACE(attribute);
            AZ::Edit::ElementData editData;
            EXPECT_FALSE(LoadAssetTypeAttributes(attribute, editData));
            EXPECT_THAT(ReadSupportedAssetTypes(editData), ::testing::ElementsAre(AZ::Data::s_invalidAssetType));
            editData.ClearAttributes();
        }
    }

    TEST_F(ScriptComponentTest, AssetIdSupportedAssetTypesRoundTripsThroughDpe)
    {
        AZ::DocumentPropertyEditor::PropertyEditorSystem propertyEditorSystem;
        AzToolsFramework::AssetIdPropertyHandlerDefault assetIdPropertyHandler;
        assetIdPropertyHandler.RegisterWithPropertySystem(&propertyEditorSystem);

        const AZ::Name attributeName = propertyEditorSystem.LookupNameFromId(SupportedAssetTypesAttribute);
        EXPECT_EQ(attributeName, AZ::Name("SupportedAssetTypes"));

        const AZ::DocumentPropertyEditor::AttributeDefinitionInterface* supportedAssetTypesDefinition = nullptr;
        propertyEditorSystem.EnumerateRegisteredAttributes(
            attributeName,
            [&supportedAssetTypesDefinition](const AZ::DocumentPropertyEditor::AttributeDefinitionInterface& definition)
            {
                if (definition.GetTypeId() == azrtti_typeid<AZStd::vector<AZ::Data::AssetType>>())
                {
                    supportedAssetTypesDefinition = &definition;
                }
            });
        ASSERT_NE(supportedAssetTypesDefinition, nullptr);

        const AZStd::vector<AZ::Data::AssetType> expectedAssetTypes = {
            AZ::Data::AssetType{ "{18F441A4-DB79-42F1-A2D7-70E450A05D64}" },
            AZ::Data::AssetType{ "{70984272-AB02-4A86-B55D-8AE3A16F6744}" }
        };
        AZ::AttributeData<AZStd::vector<AZ::Data::AssetType>> legacyAttribute(expectedAssetTypes);

        const AZ::Dom::Value domValue = supportedAssetTypesDefinition->LegacyAttributeToDomValue(
            AZ::PointerObject{ nullptr, AZ::TypeId::CreateNull() }, &legacyAttribute);
        ASSERT_FALSE(domValue.IsNull());

        AZStd::shared_ptr<AZ::Attribute> restoredAttribute =
            supportedAssetTypesDefinition->DomValueToLegacyAttribute(domValue, true);
        ASSERT_NE(restoredAttribute, nullptr);

        AZStd::vector<AZ::Data::AssetType> actualAssetTypes;
        AZ::AttributeReader restoredAttributeReader(nullptr, restoredAttribute.get());
        ASSERT_TRUE(restoredAttributeReader.Read<AZStd::vector<AZ::Data::AssetType>>(actualAssetTypes));
        EXPECT_EQ(actualAssetTypes, expectedAssetTypes);
    }
} // namespace UnitTest
