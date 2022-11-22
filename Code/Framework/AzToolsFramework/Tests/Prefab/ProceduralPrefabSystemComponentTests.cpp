/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/PathReflect.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <Prefab/PrefabTestFixture.h>
#include <Prefab/ProceduralPrefabSystemComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    struct ProceduralPrefabSystemComponentTests
        : AllocatorsTestFixture
          , AZ::ComponentApplicationBus::Handler
    {
        void SetUp() override
        {
            TestRunner::Instance().m_suppressOutput = false;
            TestRunner::Instance().m_suppressPrintf = false;
            TestRunner::Instance().m_suppressWarnings = false;
            TestRunner::Instance().m_suppressErrors = false;
            TestRunner::Instance().m_suppressAsserts = false;

            AllocatorsTestFixture::SetUp();

            AZ::ComponentApplicationBus::Handler::BusConnect();

            m_localFileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();

            m_prevIoBase = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr); // Need to clear the previous instance first
            AZ::IO::FileIOBase::SetInstance(m_localFileIo.get());

            AZ::JsonSystemComponent::Reflect(&m_jsonContext);

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_temporaryDirectory.GetDirectory());

            m_prefabSystem = PrefabSystemComponent::CreateDescriptor();
            m_procSystem = ProceduralPrefabSystemComponent::CreateDescriptor();

            m_prefabSystem->Reflect(&m_context);
            m_prefabSystem->Reflect(&m_jsonContext);
            m_procSystem->Reflect(&m_context);
            m_procSystem->Reflect(&m_jsonContext);

            AZ::Entity::Reflect(&m_context);
            AZ::Entity::Reflect(&m_jsonContext);
            AZ::IO::PathReflect(&m_context);

            m_systemEntity = AZStd::make_unique<AZ::Entity>();
            m_systemEntity->CreateComponent<PrefabSystemComponent>();
            m_systemEntity->CreateComponent<ProceduralPrefabSystemComponent>();

            m_systemEntity->Init();
            m_systemEntity->Activate();

            AZ::Data::AssetManager::Create({});
        }

        void TearDown() override
        {
            AZ::Data::AssetManager::Destroy();

            m_systemEntity->Deactivate();
            m_systemEntity = nullptr;

            m_jsonContext.EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(&m_jsonContext);
            m_prefabSystem->Reflect(&m_jsonContext);
            m_procSystem->Reflect(&m_jsonContext);
            AZ::Entity::Reflect(&m_jsonContext);
            m_jsonContext.DisableRemoveReflection();

            AZ::IO::FileIOBase::SetInstance(nullptr); // Clear the previous instance first
            AZ::IO::FileIOBase::SetInstance(m_prevIoBase);

            m_prevIoBase = nullptr;

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

            AZ::ComponentApplicationBus::Handler::BusDisconnect();
            AllocatorsTestFixture::TearDown();

            TestRunner::Instance().ResetSuppressionSettingsToDefault();

            delete m_procSystem;
            m_procSystem = nullptr;
            delete m_prefabSystem;
            m_prefabSystem = nullptr;
        }

        // ComponentApplicationBus
        AZ::ComponentApplication* GetApplication() override
        {
            return nullptr;
        }

        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }

        bool AddEntity(AZ::Entity*) override
        {
            return true;
        }

        bool RemoveEntity(AZ::Entity*) override
        {
            return true;
        }

        bool DeleteEntity(const AZ::EntityId&) override
        {
            return true;
        }

        AZ::Entity* FindEntity(const AZ::EntityId&) override
        {
            return nullptr;
        }

        AZ::SerializeContext* GetSerializeContext() override
        {
            return &m_context;
        }

        AZ::BehaviorContext* GetBehaviorContext() override
        {
            return nullptr;
        }

        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override
        {
            return &m_jsonContext;
        }

        const char* GetEngineRoot() const override
        {
            return nullptr;
        }

        const char* GetExecutableFolder() const override
        {
            return nullptr;
        }

        void EnumerateEntities(const EntityCallback& /*callback*/) override { }
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override { }
        ////

        AZ::ComponentDescriptor* m_prefabSystem{};
        AZ::ComponentDescriptor* m_procSystem{};
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        AZ::SerializeContext m_context;
        AZ::JsonRegistrationContext m_jsonContext;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_localFileIo;
        AZ::Test::ScopedAutoTempDirectory m_temporaryDirectory;
        AZStd::unique_ptr<AZ::Entity> m_systemEntity;

        AZ::IO::FileIOBase* m_prevIoBase{};
    };

    struct MockCatalog : AZ::Data::AssetCatalogRequestBus::Handler
    {
        static const inline AZ::Data::AssetId TestId{ AZ::Uuid::CreateRandom(), 1234 };

        MockCatalog(AZStd::string testFile)
            : m_testFile(AZStd::move(testFile))
        {
            BusConnect();
        }

        ~MockCatalog() override
        {
            BusDisconnect();
        }

        AZStd::string GetAssetPathById(const AZ::Data::AssetId& assetId) override
        {
            if (assetId == TestId)
            {
                return m_testFile;
            }

            return "InvalidAssetId";
        }

        AZ::Data::AssetId GetAssetIdByPath(const char* path, const AZ::Data::AssetType&, bool) override
        {
            AZ::IO::PathView pathView{ AZStd::string_view(path) };

            if (AZ::IO::PathView(m_testFile) == pathView)
            {
                return TestId;
            }

            AZ_Error("MockCatalog", false, "Requested path %s does not match expected asset path of %s", path, m_testFile.c_str());
            ADD_FAILURE();

            return {};
        }

        AZStd::string m_testFile;
    };

    struct PrefabPublicNotificationsListener : PrefabPublicNotificationBus::Handler
    {
        PrefabPublicNotificationsListener()
        {
            BusConnect();
        }

        ~PrefabPublicNotificationsListener() override
        {
            BusDisconnect();
        }

        void OnPrefabInstancePropagationBegin() override
        {
            m_updated = true;
        }

        bool m_updated = false;
    };

    TEST_F(ProceduralPrefabSystemComponentTests, RegisteredPrefabUpdates)
    {
        const AZStd::string prefabFile = (AZ::IO::Path(m_temporaryDirectory.GetDirectory()) / "test.prefab").Native();
        MockCatalog catalog(prefabFile.c_str());

        auto proceduralPrefabSystemComponentInterface = AZ::Interface<ProceduralPrefabSystemComponentInterface>::Get();
        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        auto prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();

        ASSERT_NE(proceduralPrefabSystemComponentInterface, nullptr);
        ASSERT_NE(prefabSystemComponentInterface, nullptr);
        ASSERT_NE(prefabLoaderInterface, nullptr);

        auto entity = aznew AZ::Entity();

        AZStd::unique_ptr<Instance> instance = prefabSystemComponentInterface->CreatePrefab({ entity }, {}, prefabFile.c_str());

        ASSERT_NE(instance, nullptr);

        prefabLoaderInterface->SaveTemplateToFile(instance->GetTemplateId(), prefabFile.c_str());

        proceduralPrefabSystemComponentInterface->RegisterProceduralPrefab(prefabFile, instance->GetTemplateId());

        AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetChanged, MockCatalog::TestId);

        PrefabPublicNotificationsListener listener;
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);

        EXPECT_TRUE(listener.m_updated);
    }

    TEST_F(ProceduralPrefabSystemComponentTests, UnregisteredPrefabDoesNotUpdate)
    {
        PrefabPublicNotificationsListener listener;

        const AZStd::string prefabFile = (AZ::IO::Path(m_temporaryDirectory.GetDirectory()) / "test.prefab").Native();
        MockCatalog catalog(prefabFile.c_str());

        auto proceduralPrefabSystemComponentInterface = AZ::Interface<ProceduralPrefabSystemComponentInterface>::Get();
        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        auto prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();

        ASSERT_NE(proceduralPrefabSystemComponentInterface, nullptr);
        ASSERT_NE(prefabSystemComponentInterface, nullptr);
        ASSERT_NE(prefabLoaderInterface, nullptr);

        auto entity = aznew AZ::Entity();

        AZStd::unique_ptr<Instance> instance = prefabSystemComponentInterface->CreatePrefab({ entity }, {}, prefabFile.c_str());

        ASSERT_NE(instance, nullptr);

        prefabLoaderInterface->SaveTemplateToFile(instance->GetTemplateId(), prefabFile.c_str());

        AzFramework::AssetCatalogEventBus::Broadcast(
            &AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetChanged, MockCatalog::TestId);

        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);

        EXPECT_FALSE(listener.m_updated);
    }
}
