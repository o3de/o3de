/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h> // This should be the first include to make sure Windows.h is defined with NOMINMAX
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/NativeUI/NativeUISystemComponent.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/CustomAssetTypeComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/BehaviorEntity.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/FileTag/FileTagComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/IO/RemoteStorageDrive.h>
#include <AzFramework/PaintBrush/PaintBrushSettings.h>
#include <AzFramework/PaintBrush/PaintBrushSystemComponent.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>
#include <AzFramework/Render/GameIntersectorComponent.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Script/ScriptRemoteDebuggingConstants.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzFramework/StreamingInstall/StreamingInstall.h>
#include <AzFramework/SurfaceData/SurfaceData.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzFramework/Visibility/VisibleGeometryBus.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

#include "Application.h"
#include <AzNetworking/AzNetworkingModule.h>
#include <AzFramework/AzFrameworkModule.h>
#include <cctype>
#include <stdio.h>

[[maybe_unused]] static const char* s_azFrameworkWarningWindow = "AzFramework";

namespace AzFramework
{

    namespace ApplicationInternal
    {
        static constexpr const char s_editorModeFeedbackKey[] = "/Amazon/Preferences/EnableEditorModeFeedback";
        static constexpr const char s_prefabWipSystemKey[] = "/Amazon/Preferences/EnablePrefabSystemWipFeatures";
        static constexpr const char s_legacySlicesAssertKey[] = "/Amazon/Preferences/ShouldAssertForLegacySlicesUsage";
        static constexpr const char* DeprecatedFileIOAliasesRoot = "/O3DE/AzCore/FileIO/DeprecatedAliases";
        static constexpr const char* DeprecatedFileIOAliasesOldAliasKey = "OldAlias";
        static constexpr const char* DeprecatedFileIOAliasesNewAliasKey = "NewAlias";

        static constexpr const char* FilesystemAliasesRoot = "/O3DE/Filesystem/Aliases";
    }

    Application::Application()
        : Application(nullptr, nullptr, {})
    {
    }

    Application::Application(AZ::ComponentApplicationSettings componentAppSettings)
        : Application(nullptr, nullptr, AZStd::move(componentAppSettings))
    {
    }

    Application::Application(int* argc, char*** argv)
        : Application(argc, argv, {})
    {
    }

    Application::Application(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
        : ComponentApplication(
            argc ? *argc : 0,
            argv ? *argv : nullptr,
            AZStd::move(componentAppSettings)
        )
    {
        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (AZ::IO::FileIOBase::GetDirectInstance() == nullptr)
        {
            m_directFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetDirectInstance(m_directFileIO.get());
        }

        // Initializes the IArchive for reading archive(.pak) files
        if (auto archive = AZ::Interface<AZ::IO::IArchive>::Get(); archive == nullptr)
        {
            m_archive = AZStd::make_unique<AZ::IO::Archive>();
            AZ::Interface<AZ::IO::IArchive>::Register(m_archive.get());
        }

        // Set the ArchiveFileIO as the default FileIOBase instance
        if (AZ::IO::FileIOBase::GetInstance() == nullptr)
        {
            m_archiveFileIO = AZStd::make_unique<AZ::IO::ArchiveFileIO>(m_archive.get());
            AZ::IO::FileIOBase::SetInstance(m_archiveFileIO.get());
            SetFileIOAliases();
            // The FileIOAvailable event needs to be registered here as this event is sent out
            // before the settings registry has merged the .setreg files from the <engine-root>
            // (That happens in MergeSettingsToRegistry
            AZ::ComponentApplicationLifecycle::RegisterEvent(*m_settingsRegistry, "FileIOAvailable");
            AZ::ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "FileIOAvailable", R"({})");
        }

        if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI == nullptr)
        {
            m_nativeUI = AZStd::make_unique<AZ::NativeUI::NativeUISystem>();
            AZ::Interface<AZ::NativeUI::NativeUIRequests>::Register(m_nativeUI.get());
        }

        if (auto poolManager = AZ::Interface<AZ::InstancePoolManagerInterface>::Get(); poolManager == nullptr)
        {
            m_poolManager = AZStd::make_unique<AZ::InstancePoolManager>();
            AZ::Interface<AZ::InstancePoolManagerInterface>::Register(m_poolManager.get());
        }

        ApplicationRequests::Bus::Handler::BusConnect();
        AZ::UserSettingsFileLocatorBus::Handler::BusConnect();
    }

    Application::~Application()
    {
        if (m_isStarted)
        {
            Stop();
        }

        AZ::UserSettingsFileLocatorBus::Handler::BusDisconnect();
        ApplicationRequests::Bus::Handler::BusDisconnect();

        if (AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get() == m_nativeUI.get())
        {
            AZ::Interface<AZ::NativeUI::NativeUIRequests>::Unregister(m_nativeUI.get());
        }
        m_nativeUI.reset();

        // Unset the Archive file IO if it is set as the direct instance
        if (AZ::IO::FileIOBase::GetInstance() == m_archiveFileIO.get())
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }
        m_archiveFileIO.reset();

        // Destroy the IArchive instance
        if (AZ::Interface<AZ::IO::IArchive>::Get() == m_archive.get())
        {
            AZ::Interface<AZ::IO::IArchive>::Unregister(m_archive.get());
        }
        m_archive.reset();

        // Unset the Local file IO if it is set as the direct instance
        if (AZ::IO::FileIOBase::GetDirectInstance() == m_directFileIO.get())
        {
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
        }

        // Destroy the Direct instance after the IArchive has been destroyed
        // Archive classes relies on the FileIOBase DirectInstance to close
        // files properly
        m_directFileIO.reset();

        AZ::ComponentApplicationLifecycle::SignalEvent(*m_settingsRegistry, "FileIOUnavailable", R"({})");
    }

    void Application::Start(const Descriptor& descriptor, const StartupParameters& startupParameters)
    {
        AZ::Entity* systemEntity = Create(descriptor, startupParameters);

        // Sets FileIOAliases again in case the App root was overridden by the
        // startupParameters in ComponentApplication::Create
        SetFileIOAliases();

        if (systemEntity)
        {
            StartCommon(systemEntity);
        }
    }

    void Application::StartCommon(AZ::Entity* systemEntity)
    {
        auto implementationFactory = AZ::Interface<Application::ImplementationFactory>::Get();
        m_pimpl = (implementationFactory != nullptr) ? implementationFactory->Create() : nullptr;

        systemEntity->Init();
        systemEntity->Activate();
        AZ_Assert(systemEntity->GetState() == AZ::Entity::State::Active, "System Entity failed to activate.");

        if (m_isStarted = (systemEntity->GetState() == AZ::Entity::State::Active); m_isStarted)
        {
            if (m_startupParameters.m_loadAssetCatalog)
            {
                // Start Monitoring Asset changes over the network and load the AssetCatalog
                auto StartMonitoringAssetsAndLoadCatalog = [this](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
                {
                    if (AZ::IO::FixedMaxPath assetCatalogPath;
                        m_settingsRegistry->Get(assetCatalogPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
                    {
                        assetCatalogPath /= "assetcatalog.xml";
                        assetCatalogRequests->LoadCatalog(assetCatalogPath.c_str());
                    }
                };
                using AssetCatalogBus = AZ::Data::AssetCatalogRequestBus;
                AssetCatalogBus::Broadcast(AZStd::move(StartMonitoringAssetsAndLoadCatalog));
            }
#if defined(ENABLE_REMOTE_TOOLS)
            IRemoteTools* remoteTools = RemoteToolsInterface::Get();
            if (remoteTools)
            {
                remoteTools->RegisterToolingServiceClient(
                    AzFramework::LuaToolsKey, AzFramework::LuaToolsName, AzFramework::LuaToolsPort);
            }
#endif
        }
    }

    void Application::Stop()
    {
        if (m_isStarted)
        {
            if (m_startupParameters.m_loadAssetCatalog)
            {
                // Stop Monitoring Assets changes
                auto StopMonitoringAssets = [](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
                {
                    assetCatalogRequests->StopMonitoringAssets();
                };
                using AssetCatalogBus = AZ::Data::AssetCatalogRequestBus;
                AssetCatalogBus::Broadcast(AZStd::move(StopMonitoringAssets));
            }

            ApplicationLifecycleEvents::Bus::Broadcast(&ApplicationLifecycleEvents::OnApplicationAboutToStop);

            m_pimpl.reset();

            // Free any memory owned by the command line container.
            m_commandLine = CommandLine();

            Destroy();

            m_isStarted = false;
        }
    }

    void Application::RegisterCoreComponents()
    {
        AZ::ComponentApplication::RegisterCoreComponents();

        // This is internal Amazon code, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
        AZStd::vector<AZ::Uuid> componentUuidsForMetricsCollection
        {
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::UserSettingsComponent>(),
            azrtti_typeid<AZ::SliceComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),

            azrtti_typeid<AzFramework::AssetCatalogComponent>(),
            azrtti_typeid<AzFramework::CustomAssetTypeComponent>(),
            azrtti_typeid<AzFramework::FileTag::ExcludeFileComponent>(),
            azrtti_typeid<AzFramework::TransformComponent>(),
            azrtti_typeid<AzFramework::SceneSystemComponent>(),
            azrtti_typeid<AzFramework::AzFrameworkConfigurationSystemComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<AzFramework::InputSystemComponent>(),

#if !defined(AZCORE_EXCLUDE_LUA)
            azrtti_typeid<AZ::ScriptSystemComponent>(),
            azrtti_typeid<AzFramework::ScriptComponent>(),
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
        };

        AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
            &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, componentUuidsForMetricsCollection);
    }

    void Application::Reflect(AZ::ReflectContext* context)
    {
        AZ::ComponentApplication::Reflect(context);

        AZ::DataPatch::Reflect(context);
        AZ::EntityUtils::Reflect(context);
        AZ::NonUniformScaleRequests::Reflect(context);
        AzFramework::BehaviorEntity::Reflect(context);
        AzFramework::EntityContext::Reflect(context);
        AzFramework::SliceEntityOwnershipService::Reflect(context);
        AzFramework::SimpleAssetReferenceBase::Reflect(context);
        AzFramework::ConsoleRequests::Reflect(context);
        AzFramework::ConsoleNotifications::Reflect(context);
        AzFramework::ViewportRequests::Reflect(context);
        AzFramework::BoundsRequests::Reflect(context);
        AzFramework::ScreenGeometryReflect(context);
        AzFramework::RemoteStorageDriveConfig::Reflect(context);
        AzFramework::PaintBrushSettings::Reflect(context);
        AzFramework::VisibleGeometry::Reflect(context);
        AzFramework::VisibleGeometryRequests::Reflect(context);

        Physics::ReflectionUtils::ReflectPhysicsApi(context);
        AzFramework::SurfaceData::SurfaceTagWeight::Reflect(context);
        AzFramework::SurfaceData::SurfacePoint::Reflect(context);
        AzFramework::Terrain::TerrainDataRequests::Reflect(context);
        Physics::HeightfieldProviderRequests::Reflect(context);
        Physics::HeightMaterialPoint::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            AzFramework::AssetRegistry::ReflectSerialize(serializeContext);
            CameraState::Reflect(*serializeContext);
            AzFramework::AssetBundleManifest::ReflectSerialize(serializeContext);
        }
    }

    AZ::ComponentTypeList Application::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = ComponentApplication::GetRequiredSystemComponents();

        components.insert(components.end(), {
            azrtti_typeid<AZ::StreamerComponent>(),
            azrtti_typeid<AZ::AssetManagerComponent>(),
            azrtti_typeid<AZ::UserSettingsComponent>(),
            azrtti_typeid<AZ::ScriptSystemComponent>(),
            azrtti_typeid<AZ::JobManagerComponent>(),
            azrtti_typeid<AZ::SliceSystemComponent>(),
            azrtti_typeid<AZ::TaskGraphSystemComponent>(),

            azrtti_typeid<AzFramework::AssetCatalogComponent>(),
            azrtti_typeid<AzFramework::CustomAssetTypeComponent>(),
            azrtti_typeid<AzFramework::FileTag::ExcludeFileComponent>(),
            azrtti_typeid<AzFramework::SceneSystemComponent>(),
            azrtti_typeid<AzFramework::AzFrameworkConfigurationSystemComponent>(),
            azrtti_typeid<AzFramework::GameEntityContextComponent>(),
            azrtti_typeid<AzFramework::RenderGeometry::GameIntersectorComponent>(),
            azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            azrtti_typeid<AzFramework::InputSystemComponent>(),
            azrtti_typeid<AzFramework::PaintBrushSystemComponent>(),
            azrtti_typeid<AzFramework::StreamingInstall::StreamingInstallSystemComponent>(),
            azrtti_typeid<AzFramework::SpawnableSystemComponent>(),
            azrtti_typeid<Physics::MaterialSystemComponent>(),
            AZ::Uuid("{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}"), // ScriptDebugAgent
            });

        return components;
    }

    // UserSettingsFileLocatorBus
    AZStd::string Application::ResolveFilePath([[maybe_unused]] AZ::u32 providerId)
    {
        AZ::IO::Path userSettingsPath;
        m_settingsRegistry->Get(userSettingsPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);
        userSettingsPath /= "UserSettings.xml";
        return userSettingsPath.Native();
    }

    AZ::Component* Application::EnsureComponentAdded(AZ::Entity* systemEntity, const AZ::Uuid& typeId)
    {
        AZ::Component* component = systemEntity->FindComponent(typeId);

        if (!component)
        {
            if (systemEntity->IsComponentReadyToAdd(typeId))
            {
                component = systemEntity->CreateComponent(typeId);
            }
            else
            {
                AZ_Assert(false, "Failed to add component of type %s because conditions are not met.", typeId.ToString<AZStd::string>().c_str());
            }
        }

        return component;
    }


    void Application::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        AZ::ComponentApplication::CreateStaticModules(outModules);

        outModules.emplace_back(aznew AzNetworking::AzNetworkingModule());
        outModules.emplace_back(aznew AzFrameworkModule());
    }

    const char* Application::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "Release";
#elif defined(_DEBUG)
        return "Debug";
#else
        return "Profile";
#endif
    }

    void Application::CreateReflectionManager()
    {
        ComponentApplication::CreateReflectionManager();
    }

    ////////////////////////////////////////////////////////////////////////////
    AZ::Uuid Application::GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        AZ::Uuid uuid(AZ::Uuid::CreateNull());
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (entity)
        {
            AZ::Component* component = entity->FindComponent(componentId);
            if (component)
            {
                uuid = component->RTTI_GetType();
            }
        }
        return uuid;
    }

    void Application::ResolveEnginePath(AZStd::string& engineRelativePath) const
    {
        auto fullPath = AZ::IO::FixedMaxPath(GetEngineRoot()) / engineRelativePath;
        engineRelativePath = fullPath.String();
    }

    void Application::CalculateBranchTokenForEngineRoot(AZStd::string& token) const
    {
        AZ::StringFunc::AssetPath::CalculateBranchToken(GetEngineRoot(), token);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathRootRelative(AZStd::string& fullPath)
    {
        MakePathRelative(fullPath, GetEngineRoot());
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathAssetRootRelative(AZStd::string& fullPath)
    {
        AZStd::string cacheAssetPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(cacheAssetPath, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
        }
        MakePathRelative(fullPath, cacheAssetPath.c_str());
        // relative file paths wrt AssetRoot are always lowercase
        AZStd::to_lower(fullPath);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::MakePathRelative(AZStd::string& fullPath, const char* rootPath)
    {
        AZ_Assert(rootPath, "Provided root path is null.");

        fullPath = AZ::IO::PathView(fullPath).LexicallyProximate(AZ::IO::PathView(rootPath)).StringAsPosix();
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::NormalizePath(AZStd::string& path)
    {
        ComponentApplication::NormalizePath(path.begin(), path.end(), true);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::NormalizePathKeepCase(AZStd::string& path)
    {
        ComponentApplication::NormalizePath(path.begin(), path.end(), false);
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::PumpSystemEventLoopOnce()
    {
        if (m_pimpl)
        {
            m_pimpl->PumpSystemEventLoopOnce();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::PumpSystemEventLoopUntilEmpty()
    {
        if (m_pimpl)
        {
            m_pimpl->PumpSystemEventLoopUntilEmpty();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::PumpSystemEventLoopWhileDoingWorkInNewThread(const AZStd::chrono::milliseconds& eventPumpFrequency,
        const AZStd::function<void()>& workForNewThread,
        const char* newThreadName)
    {
        AZ_PROFILE_FUNCTION(AzFramework);

        AZStd::thread_desc newThreadDesc;
        newThreadDesc.m_cpuId = AFFINITY_MASK_USERTHREADS;
        newThreadDesc.m_name = newThreadName;
        AZStd::binary_semaphore binarySemaphore;
        AZStd::thread newThread(
            newThreadDesc,
            [&workForNewThread, &binarySemaphore, &newThreadName]
            {
                AZ_PROFILE_SCOPE(AzFramework,
                    "Application::PumpSystemEventLoopWhileDoingWorkInNewThread:ThreadWorker %s", newThreadName);

                workForNewThread();
                binarySemaphore.release();
            });
        while (!binarySemaphore.try_acquire_for(eventPumpFrequency))
        {
            PumpSystemEventLoopUntilEmpty();
        }
        {
            AZ_PROFILE_SCOPE(AzFramework,
                "Application::PumpSystemEventLoopWhileDoingWorkInNewThread:WaitOnThread %s", newThreadName);
            newThread.join();
        }

        // Pump once at the end so we're back at 0 instead of potentially eventPumpFrequency - 1 ms since the last event pump
        PumpSystemEventLoopUntilEmpty();
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::RunMainLoop()
    {
        uint32_t frameCounter = 0;
        while (!m_exitMainLoopRequested)
        {
            PumpSystemEventLoopUntilEmpty();

            AZ_PROFILE_SCOPE(AzCore, "Frame %i", frameCounter);
            Tick();
            ++frameCounter;
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    void Application::Tick()
    {
        ComponentApplication::Tick();
    }

    ////////////////////////////////////////////////////////////////////////////
    void Application::TerminateOnError(int errorCode)
    {
        if (m_pimpl)
        {
            m_pimpl->TerminateOnError(errorCode);
        }
        else
        {
            exit(errorCode);
        }
    }

    struct DeprecatedAliasesKeyVisitor
        : AZ::SettingsRegistryVisitorUtils::ArrayVisitor
    {
        using VisitArgs = AZ::SettingsRegistryInterface::VisitArgs;

        using AZ::SettingsRegistryInterface::Visitor::Visit;

        AZ::SettingsRegistryInterface::VisitResponse Visit(const VisitArgs& visitArgs) override
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AliasPair aliasPair;
            const auto oldAliasKeyPath = FixedValueString::format("%.*s/%s", AZ_STRING_ARG(visitArgs.m_jsonKeyPath),
                ApplicationInternal::DeprecatedFileIOAliasesOldAliasKey);
            const auto newAliasKeyPath = FixedValueString::format("%.*s/%s", AZ_STRING_ARG(visitArgs.m_jsonKeyPath),
                ApplicationInternal::DeprecatedFileIOAliasesNewAliasKey);
            if (visitArgs.m_registry.Get(aliasPair.m_oldAlias, oldAliasKeyPath)
                && visitArgs.m_registry.Get(aliasPair.m_newAlias, newAliasKeyPath))
            {
                m_aliases.emplace_back(AZStd::move(aliasPair));
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        }

        struct AliasPair
        {
            AZStd::string m_oldAlias;
            AZStd::string m_newAlias;
        };
        AZStd::vector<AliasPair> m_aliases;
    };

    static void CreateUserCache(const AZ::IO::FixedMaxPath& cacheUserPath, AZ::IO::FileIOBase& fileIoBase)
    {
        constexpr const char* userCachePathFilename{ "Cache" };
        AZ::IO::FixedMaxPath userCachePath = cacheUserPath / userCachePathFilename;
#if AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
        // The number of max attempts ultimately dictates the number of application instances that can run
        // simultaneously.  This should be a reasonably high number so that it doesn't artificially limit
        // the number of instances (ex: parallel level exports via multiple Editor runs).  It also shouldn't
        // be set *infinitely* high - each cache folder is GBs in size, and finding a free directory is a
        // linear search, so the more instances we allow, the longer the search will take.
        // 128 seems like a reasonable compromise.
        constexpr int maxAttempts = 128;

        int attemptNumber;
        for (attemptNumber = 0; attemptNumber < maxAttempts; ++attemptNumber)
        {
            if (attemptNumber != 0)
            {
                userCachePath.ReplaceFilename(AZStd::string_view{ AZ::IO::FixedMaxPathString::format("%s%i", userCachePathFilename, attemptNumber) });
            }

            // if the directory already exists, check for locked file
            auto cacheLockFilePath = userCachePath / "lockfile.txt";

            constexpr auto LockFileMode = AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY
                | AZ::IO::SystemFile::SF_OPEN_CREATE
                | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH;
            if (AZ::IO::SystemFile lockFileHandle; lockFileHandle.Open(cacheLockFilePath.c_str(), LockFileMode))
            {
                break;
            }
        }

        if (attemptNumber >= maxAttempts)
        {
            userCachePath.ReplaceFilename(userCachePathFilename);
            AZ_TracePrintf("Application", "Couldn't find a valid asset cache folder after %i attempts."
                " Setting cache folder to %s\n", maxAttempts, userCachePath.c_str());
        }
#endif

        fileIoBase.SetAlias("@usercache@", userCachePath.c_str());
    }

    void Application::SetFileIOAliases()
    {
        if (auto fileIoBase = m_archiveFileIO.get(); fileIoBase)
        {
            // Set up the default file aliases based on the settings registry

            AZ::IO::FixedMaxPath engineRootPath;
            if (m_settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                fileIoBase->SetAlias("@engroot@", engineRootPath.c_str());
                // default project root to engine root if not set
                // It should be overridden in the next if block
                fileIoBase->SetAlias("@projectroot@", engineRootPath.c_str());
            }

            AZ::IO::FixedMaxPath projectRootPath;
            if (m_settingsRegistry->Get(projectRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
            {
                fileIoBase->SetAlias("@projectroot@", projectRootPath.c_str());
            }

            if (AZ::IO::FixedMaxPath exeFolder; m_settingsRegistry->Get(exeFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_BinaryFolder))
            {
                fileIoBase->SetAlias("@exefolder@", exeFolder.c_str());
            }

            if (AZ::IO::FixedMaxPath pathAliases; m_settingsRegistry->Get(pathAliases.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
            {
                fileIoBase->SetAlias("@products@", pathAliases.c_str());
            }

            AZ::IO::FixedMaxPath projectUserPath;
            if (!m_settingsRegistry->Get(projectUserPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
            {
                projectUserPath = engineRootPath / "user";
            }
            fileIoBase->SetAlias("@user@", projectUserPath.c_str());
            fileIoBase->CreatePath(projectUserPath.c_str());
            CreateUserCache(projectUserPath, *fileIoBase);

            AZ::IO::FixedMaxPath projectLogPath;
            if (!m_settingsRegistry->Get(projectLogPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectLogPath))
            {
                projectLogPath = projectUserPath / "log";
            }
            fileIoBase->SetAlias("@log@", projectLogPath.c_str());
            fileIoBase->CreatePath(projectLogPath.c_str());

            if (DeprecatedAliasesKeyVisitor visitor;
                m_settingsRegistry->Visit(visitor, ApplicationInternal::DeprecatedFileIOAliasesRoot))
            {
                for (const auto& [oldAlias, newAlias] : visitor.m_aliases)
                {
                    fileIoBase->SetDeprecatedAlias(oldAlias, newAlias);
                }
            }

            // The following section sets the @gemroot:<gem-name>@ alias for
            // every loaded gem
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            auto AddGemAlias = [&fileIoBase](AZStd::string_view gemName, AZStd::string_view gemRootPath)
            {
                const auto gemAlias{ FixedValueString::format("@gemroot:%.*s@", AZ_STRING_ARG(gemName)) };
                const auto gemAliasPath = FixedValueString(gemRootPath);
                fileIoBase->SetAlias(gemAlias.c_str(), gemAliasPath.c_str());
            };
            AZ::SettingsRegistryMergeUtils::VisitActiveGems(*m_settingsRegistry, AddGemAlias);

            // Load any Filesystem aliases from the SettingsRegistry
            auto SetAliasesFromSettingsRegistry = [&fileIoBase, &settingsRegistry = *m_settingsRegistry]
                (const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                if (AZ::IO::FixedMaxPath aliasPath; settingsRegistry.Get(aliasPath.Native(), visitArgs.m_jsonKeyPath))
                {
                    if (AZ::IO::SystemFile::Exists(aliasPath.c_str()))
                    {
                        fileIoBase->SetAlias(FixedValueString(visitArgs.m_fieldName).c_str(), aliasPath.c_str());
                    }
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };
            AZ::SettingsRegistryVisitorUtils::VisitObject(*m_settingsRegistry, SetAliasesFromSettingsRegistry,
                ApplicationInternal::FilesystemAliasesRoot);
        }
    }

    bool Application::IsEditorModeFeedbackEnabled() const
    {
        bool value = true;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(value, ApplicationInternal::s_editorModeFeedbackKey);
        }
        return value;
    }

    bool Application::IsPrefabSystemEnabled() const
    {
        AZ_WarningOnce("Application", false, "'IsPrefabSystemEnabled' is deprecated, the editor only supports prefabs for level editing.");
        return true;
    }

    bool Application::ArePrefabWipFeaturesEnabled() const
    {
        bool value = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(value, ApplicationInternal::s_prefabWipSystemKey);
        }
        return value;
    }

    void Application::SetPrefabSystemEnabled(bool /* enable */)
    {
        AZ_WarningOnce("Application", false, "'SetPrefabSystemEnabled' is deprecated, the editor only supports prefabs for level editing.");
    }

    bool Application::IsPrefabSystemForLevelsEnabled() const
    {
        AZ_WarningOnce("Application", false, "'IsPrefabSystemForLevelsEnabled' is deprecated, the editor only supports prefabs for level editing.");
        return true;
    }

    bool Application::ShouldAssertForLegacySlicesUsage() const
    {
        bool value = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(value, ApplicationInternal::s_legacySlicesAssertKey);
        }
        return value;
    }

} // namespace AzFramework
