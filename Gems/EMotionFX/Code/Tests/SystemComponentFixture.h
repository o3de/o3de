/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>

#include <Integration/System/SystemComponent.h>
#include <Integration/AnimationBus.h>

#include <Tests/Printers.h>
#include <Tests/Matchers.h>

namespace EMotionFX
{
    template<class... Components>
    class EMotionFXTestModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(EMotionFXTestModule, "{32567457-5341-4D8D-91A9-E48D8395DE65}", AZ::Module);
        AZ_CLASS_ALLOCATOR(EMotionFXTestModule, AZ::OSAllocator);

        EMotionFXTestModule()
        {
            m_descriptors.insert(m_descriptors.end(), {Components::CreateDescriptor()...});
        }
    };

    template<class... Components>
    class ComponentFixtureApp
        : public AzFramework::Application
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentFixtureApp, AZ::SystemAllocator)

        ComponentFixtureApp()
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            constexpr auto projectPathKey = FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            if(auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZ::IO::FixedMaxPath enginePath;
                settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                settingsRegistry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
            }
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return {azrtti_typeid<Components>()...};
        }

        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override
        {
            // This is intentionally bypassing the static modules that
            // AzFramework::Application would create. That creates more
            // components than these tests need.
            outModules.emplace_back(aznew EMotionFXTestModule<Components...>());
        }

        void StartCommon(AZ::Entity* systemEntity) override
        {
            m_systemEntity = systemEntity;
            AzFramework::Application::StartCommon(systemEntity);
        }

        AZ::Entity* GetSystemEntity() const
        {
            return m_systemEntity;
        }

    private:
        AZ::Entity* m_systemEntity = nullptr;
    };

    //! A fixture that constructs an EMotionFX::Integration::SystemComponent
    /*!
     * This fixture can be used by any test that needs the EMotionFX runtime to
     * be working. It will construct all necessary allocators for EMotionFX
     * objects to be successfully instantiated.
    */
    template<class... Components>
    class ComponentFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:

        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            PreStart();

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_createEditContext = true;
            startupParameters.m_loadAssetCatalog = false;
            startupParameters.m_loadSettingsRegistry = false;

            // Add EMotionFX as an active gem within the Settings Registry for unit test
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZ::Test::AddActiveGem("EMotionFX", *settingsRegistry);
            }

            m_app.Start(AZ::ComponentApplication::Descriptor{}, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            GetSerializeContext()->CreateEditContext();
        }

        void TearDown() override
        {
            // If we loaded the asset catalog, call this function to release all the assets that has been loaded internally.
            if (HasComponentType<AzFramework::AssetCatalogComponent>())
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }

            GetSerializeContext()->DestroyEditContext();
            // Clear the queue of messages from unit tests on our buses
            EMotionFX::Integration::ActorNotificationBus::ClearQueuedEvents();

            m_app.Stop();

            UnitTest::LeakDetectionFixture::TearDown();
        }

        ~ComponentFixture() override
        {
            if (GetSystemEntity()->GetState() == AZ::Entity::State::Active)
            {
                GetSystemEntity()->Deactivate();
            }
        }

        AZ::SerializeContext* GetSerializeContext()
        {
            return m_app.GetSerializeContext();
        }

        AZ::Entity* GetSystemEntity() const
        {
            return m_app.GetSystemEntity();
        }

        AZStd::string ResolvePath(const char* path) const
        {
            AZStd::string result;
            result.resize(AZ::IO::MaxPathLength);
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(path, result.data(), result.size());
            result.resize_no_construct(AZStd::char_traits<char>::length(result.data()));
            return result;
        }

    protected:

        virtual AZStd::string GetAssetFolder() const
        {
            AZStd::string assetCachePath;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(assetCachePath, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
            }
            return assetCachePath;
        }

        // Runs after allocators are set up but before application startup
        // Used by the InitSceneAPI fixture to load the SceneAPI dlls
        virtual void PreStart() {}

        template<typename T>
        static constexpr bool HasComponentType()
        {
            // Return true if T is somewhere in the Components parameter pack
            // This expands to
            // return (((AZStd::is_same_v<Components[0], T> || AZStd::is_same_v<Components[1], T>) || AZStd::is_same_v<Components[2], T>) || ...)
            return ((... || AZStd::is_same_v<Components, T>));
        }

    protected:
        // The ComponentApplication must not be a pointer, because it cannot be
        // dynamically allocated. Calls to new will try to use the SystemAllocator
        // that has not been created yet. If one is created before
        // ComponentApplication::Create() is called, ComponentApplication will
        // complain that there's already a SystemAllocator, as it tries to make one
        // itself.
        ComponentFixtureApp<Components...> m_app;
    };

    using SystemComponentFixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        Physics::MaterialSystemComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // Use this fixture if you want to load asset catalog. Some assets (reference anim graph for example)
    // can only be loaded when asset catalog is loaded.
    using SystemComponentFixtureWithCatalog = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzFramework::AssetCatalogComponent,
        Physics::MaterialSystemComponent,
        EMotionFX::Integration::SystemComponent
    >;
} // end namespace EMotionFX
