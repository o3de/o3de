/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/BlastSystemComponent.h>

#include <Asset/BlastAsset.h>
#include <Asset/BlastAssetHandler.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Blast/BlastActorData.h>
#include <Blast/BlastDebug.h>
#include <Blast/BlastFamilyComponentBus.h>
#include <Blast/BlastMaterial.h>
#include <IConsole.h>
#include <NvBlastExtPxSerialization.h>
#include <NvBlastExtTkSerialization.h>
#include <NvBlastPxCallbacks.h>
#include <NvBlastTkJoint.h>
#include <PhysX/SystemComponentBus.h>
#include <System/PhysXCpuDispatcher.h>
#ifdef BLAST_EDITOR
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#endif
#include <CryCommon/ISystem.h>

namespace Blast
{
    static const char* const DefaultConfigurationPath = "default.blastconfiguration";

    void BlastSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        BlastGlobalConfiguration::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BlastSystemComponent, AZ::Component>()->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<BlastSystemComponent>("Blast", "Adds support for the NVIDIA Blast destruction system")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void BlastGlobalConfiguration::Reflect(AZ::ReflectContext* context)
    {
        BlastMaterialLibraryAsset::Reflect(context);
        BlastMaterialConfiguration::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BlastGlobalConfiguration>()
                ->Field("BlastMaterialLibrary", &BlastGlobalConfiguration::m_materialLibrary)
                ->Field("StressSolverIterations", &BlastGlobalConfiguration::m_stressSolverIterations)
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<BlastGlobalConfiguration>(
                      "Blast global configuration", "Set of configuration that are applied globally within Blast gem.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastGlobalConfiguration::m_materialLibrary,
                        "Blast material library", "Material library asset to be used globally.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastGlobalConfiguration::m_stressSolverIterations,
                        "Stress solver iterations",
                        "Number of iterations stress solver on each family runs for each tick.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 50000);
            }
        }
    }

    void BlastSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BlastService"));
    }

    void BlastSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BlastService"));
    }

    void BlastSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysXService"));
    }

    void BlastSystemComponent::Init()
    {
        // Route NvBlast allocations through the AZ system allocator
        NvBlastGlobalSetAllocatorCallback(&m_blastAllocatorCallback);
        m_debugRenderMode = DebugRenderDisabled;
    }

    void BlastSystemComponent::Activate()
    {
        AZ_PROFILE_FUNCTION(Physics);
        auto blastAssetHandler = aznew BlastAssetHandler();
        blastAssetHandler->Register();
        m_assetHandlers.emplace_back(blastAssetHandler);

        auto materialAsset = aznew AzFramework::GenericAssetHandler<BlastMaterialLibraryAsset>(
            "Blast Material", "Blast", "blastmaterial");
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
        if (assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<BlastAsset>::Uuid());
            assetCatalog->AddExtension("blast");
        }

        m_registered = false;

        BlastSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();

        InitPhysics();
    }

    void BlastSystemComponent::Deactivate()
    {
        AZ_PROFILE_FUNCTION(Physics);
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        BlastSystemRequestBus::Handler::BusDisconnect();

        SaveConfiguration();
        DeactivatePhysics();

        m_assetHandlers.clear();
    };

    void BlastSystemComponent::InitPhysics()
    {
        // Create blast singletons
        m_tkFramework.reset(NvBlastTkFrameworkCreate());
        m_dispatcher = PhysX::PhysXCpuDispatcherCreate();

        physx::PxCooking* cooking = nullptr;
        PhysX::SystemRequestsBus::BroadcastResult(cooking, &PhysX::SystemRequests::GetCooking);

        m_defaultTaskManager.reset(physx::PxTaskManager::createTaskManager(NvBlastGetPxErrorCallback(), m_dispatcher));
        m_extSerialization.reset(NvBlastExtSerializationCreate());

        if (m_extSerialization != nullptr)
        {
            NvBlastExtPxSerializerLoadSet(*m_tkFramework, PxGetPhysics(), *cooking, *m_extSerialization);
            NvBlastExtTkSerializerLoadSet(*m_tkFramework, *m_extSerialization);
        }

        NvBlastProfilerSetCallback(&m_blastProfilerCallback);
        NvBlastProfilerSetDetail(Nv::Blast::ProfilerDetail::HIGH);
    }

    void BlastSystemComponent::DeactivatePhysics()
    {
        m_extSerialization = nullptr;
        m_defaultTaskManager = nullptr;
        m_tkFramework = nullptr;
        delete m_dispatcher;
        m_dispatcher = nullptr;
    }

    void BlastSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ_PROFILE_FUNCTION(Physics);

        AZ::JobCompletion jobCompletion;

        BlastFamilyComponentRequestBus::EnumerateHandlers(
            [&jobCompletion](BlastFamilyComponentRequests* handler)
            {
                auto stressDamageJob = AZ::CreateJobFunction(
                    [handler]() -> void
                    {
                        handler->ApplyStressDamage();
                    },
                    true);
                stressDamageJob->SetDependent(&jobCompletion);
                stressDamageJob->Start();

                return true;
            });

        BlastFamilyComponentRequestBus::EnumerateHandlers(
            [](BlastFamilyComponentRequests* handler)
            {
                handler->SyncMeshes();

                return true;
            });

        jobCompletion.StartAndWaitForCompletion();

        // Run groups
        for (auto i = 0; i < m_groups.size(); ++i)
        {
            if (m_groups[i].m_tkGroup->getActorCount() == 0)
            {
                AZStd::swap(m_groups[i], m_groups.back());
                m_groups.pop_back();
            }
        }

        for (auto& group : m_groups)
        {
            AZ_PROFILE_SCOPE(Physics, "ExtGroupTaskManager::process");
            group.m_extGroupTaskManager->process();
        }
        for (auto& group : m_groups)
        {
            AZ_PROFILE_SCOPE(Physics, "ExtGroupTaskManager::wait");
            group.m_extGroupTaskManager->wait();
        }

        // Clean up damage descriptions and program params now that groups have run.
        {
            AZ_PROFILE_SCOPE(Physics, "BlastSystemComponent::OnTick::Cleanup");
            m_radialDamageDescs.clear();
            m_capsuleDamageDescs.clear();
            m_shearDamageDescs.clear();
            m_triangleDamageDescs.clear();
            m_impactDamageDescs.clear();
            m_programParams.clear();
        }

        if (gEnv && m_debugRenderMode)
        {
            AZ_PROFILE_SCOPE(Physics, "BlastSystemComponent::OnTick::DebugRender");
            DebugRenderBuffer buffer;
            BlastFamilyComponentRequestBus::Broadcast(
                &BlastFamilyComponentRequests::FillDebugRenderBuffer, buffer, m_debugRenderMode);

            // This is a system component, and thus is not associated with a specific scene, so use the bootstrap scene
            // for the debug drawing
            const auto mainScene = AZ::RPI::RPISystemInterface::Get()->GetSceneByName(AZ::Name("Main"));
            if (mainScene)
            {
                auto drawQueue = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(mainScene);

                for (DebugLine& line : buffer.m_lines)
                {
                    AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArguments;
                    drawArguments.m_verts = &line.m_p0;
                    drawArguments.m_vertCount = 2;
                    drawArguments.m_colors = &line.m_color;
                    drawArguments.m_colorCount = 1;
                    drawQueue->DrawLines(drawArguments);
                }
            }
        }
    }

    void BlastSystemComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (m_configuration.m_materialLibrary == asset)
        {
            m_configuration.m_materialLibrary = asset;
        }
    }

    void BlastSystemComponent::LoadConfiguration()
    {
        BlastGlobalConfiguration globalConfiguration;
        [[maybe_unused]] bool loaded = AZ::Utils::LoadObjectFromFileInPlace<BlastGlobalConfiguration>(
            DefaultConfigurationPath, globalConfiguration);
        AZ_Warning("Blast", loaded, "Failed to load Blast configuration, initializing with default configs.");

        ApplyGlobalConfiguration(globalConfiguration);
        if (!loaded)
        {
            SaveConfiguration();
        }
    }

    void BlastSystemComponent::SaveConfiguration()
    {
        auto assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@");

        if (!assetRoot)
        {
            return;
        }

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(assetRoot, DefaultConfigurationPath, fullPath);

        [[maybe_unused]] bool saved = AZ::Utils::SaveObjectToFile<BlastGlobalConfiguration>(
            fullPath.c_str(), AZ::DataStream::ST_XML, &m_configuration);
        AZ_Warning("BlastSystemComponent", saved, "Failed to save Blast configuration");
    }

    void BlastSystemComponent::CheckoutConfiguration()
    {
        const auto assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@");

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(assetRoot, DefaultConfigurationPath, fullPath);

#ifdef BLAST_EDITOR
        AzToolsFramework::SourceControlCommandBus::Broadcast(
            &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, fullPath.c_str(), true,
            [](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& info)
            {
                // File is checked out
            });
#endif
    }

    void BlastSystemComponent::OnCrySystemInitialized(ISystem&, const SSystemInitParams&)
    {
        LoadConfiguration();
        RegisterCommands();
    }

    void BlastSystemComponent::OnCryEditorInitialized()
    {
        CheckoutConfiguration();
    }

    Nv::Blast::TkFramework* BlastSystemComponent::GetTkFramework() const
    {
        return m_tkFramework.get();
    }

    Nv::Blast::ExtSerialization* BlastSystemComponent::GetExtSerialization() const
    {
        return m_extSerialization.get();
    }

    Nv::Blast::TkGroup* BlastSystemComponent::GetTkGroup()
    {
        m_groups.emplace_back();
        Nv::Blast::TkGroupDesc groupDesc;
        groupDesc.workerCount = m_defaultTaskManager->getCpuDispatcher()->getWorkerCount();
        m_groups.back().m_tkGroup.reset(m_tkFramework->createGroup(groupDesc));
        m_groups.back().m_extGroupTaskManager.reset(
            Nv::Blast::ExtGroupTaskManager::create(*m_defaultTaskManager, m_groups.back().m_tkGroup.get()));

        return m_groups.back().m_tkGroup.get();
    }

    void BlastSystemComponent::AddDamageDesc(AZStd::unique_ptr<NvBlastExtRadialDamageDesc> desc)
    {
        m_radialDamageDescs.push_back(AZStd::move(desc));
    }

    void BlastSystemComponent::AddDamageDesc(AZStd::unique_ptr<NvBlastExtCapsuleRadialDamageDesc> desc)
    {
        m_capsuleDamageDescs.push_back(AZStd::move(desc));
    }

    void BlastSystemComponent::AddDamageDesc(AZStd::unique_ptr<NvBlastExtShearDamageDesc> desc)
    {
        m_shearDamageDescs.push_back(AZStd::move(desc));
    }

    void BlastSystemComponent::AddDamageDesc(AZStd::unique_ptr<NvBlastExtTriangleIntersectionDamageDesc> desc)
    {
        m_triangleDamageDescs.push_back(AZStd::move(desc));
    }

    void BlastSystemComponent::AddDamageDesc(AZStd::unique_ptr<NvBlastExtImpactSpreadDamageDesc> desc)
    {
        m_impactDamageDescs.push_back(AZStd::move(desc));
    }

    void BlastSystemComponent::AddProgramParams(AZStd::unique_ptr<NvBlastExtProgramParams> program)
    {
        m_programParams.emplace_back(AZStd::move(program));
    }

    const BlastGlobalConfiguration& BlastSystemComponent::GetGlobalConfiguration() const
    {
        return m_configuration;
    }

    void BlastSystemComponent::SetGlobalConfiguration(const BlastGlobalConfiguration& globalConfiguration)
    {
        ApplyGlobalConfiguration(globalConfiguration);
        SaveConfiguration();
    }

    void BlastSystemComponent::ApplyGlobalConfiguration(const BlastGlobalConfiguration& globalConfiguration)
    {
        m_configuration = globalConfiguration;

        {
            AZ::Data::Asset<Blast::BlastMaterialLibraryAsset>& materialLibrary = m_configuration.m_materialLibrary;

            if (!materialLibrary.GetId().IsValid())
            {
                AZ_Warning("Blast", false, "LoadDefaultMaterialLibrary: Default Material Library asset ID is invalid.");
                return;
            }

            materialLibrary = AZ::Data::AssetManager::Instance().GetAsset<Blast::BlastMaterialLibraryAsset>(
                materialLibrary.GetId(), AZ::Data::AssetLoadBehavior::QueueLoad);
            materialLibrary.BlockUntilLoadComplete();

            // Listen for material library asset modification events
            if (!AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(materialLibrary.GetId()))
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect();
                AZ::Data::AssetBus::MultiHandler::BusConnect(materialLibrary.GetId());
            }

            AZ_Warning(
                "Blast", (materialLibrary.GetData() != nullptr),
                "LoadDefaultMaterialLibrary: Default Material Library asset data is invalid.");
        }
#ifdef BLAST_EDITOR
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
#endif
    }

    void BlastSystemComponent::AZBlastProfilerCallback::zoneStart(const char* eventName)
    {
        AZ_PROFILE_BEGIN(Physics, eventName);
    }

    void BlastSystemComponent::AZBlastProfilerCallback::zoneEnd()
    {
        AZ_PROFILE_END(Physics);
    }

    static void CmdToggleBlastDebugVisualization(IConsoleCmdArgs* args)
    {
        const int argumentCount = args->GetArgCount();

        if (argumentCount == 2)
        {
            const auto userPreference = static_cast<DebugRenderMode>(strtol(args->GetArg(1), nullptr, 10));
            BlastSystemRequestBus::Broadcast(&BlastSystemRequests::SetDebugRenderMode, userPreference);
        }
        else
        {
            AZ_Warning(
                "Blast", false,
                "Invalid blast_debug Arguments. Please use blast_debug 1 to enable, blast_debug 0 to disable.");
        }
    }

    void BlastSystemComponent::RegisterCommands()
    {
        if (m_registered)
        {
            return;
        }

        if (gEnv)
        {
            IConsole* console = gEnv->pSystem->GetIConsole();
            if (console)
            {
                console->AddCommand("blast_debug", CmdToggleBlastDebugVisualization);
            }

            m_registered = true;
        }
    }

    void BlastSystemComponent::SetDebugRenderMode(DebugRenderMode debugRenderMode)
    {
        m_debugRenderMode = debugRenderMode;
    }
} // namespace Blast
