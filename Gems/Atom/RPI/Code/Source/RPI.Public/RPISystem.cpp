/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/RPISystem.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Reflect/System/AssetAliases.h>
#include <Atom/RPI.Reflect/System/PipelineRenderSettings.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/PerformanceCollectionNotificationBus.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/XRRenderingInterface.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Time/ITime.h>

#include <AzFramework/Asset/AssetSystemBus.h>

AZ_DEFINE_BUDGET(AzRender);
AZ_DEFINE_BUDGET(RPI);

// This will cause the RPI System to print out global state (like the current pass hierarchy) when an assert is hit
// This is useful for rendering engineers debugging a crash in the RPI/RHI layers
#define AZ_RPI_PRINT_GLOBAL_STATE_ON_ASSERT 0

namespace AZ
{
    namespace RPI
    {
        RPISystemInterface* RPISystemInterface::Get()
        {
            return Interface<RPISystemInterface>::Get();
        }

        void RPISystem::Reflect(ReflectContext* context)
        {
            AssetReference::Reflect(context);

            BufferSystem::Reflect(context);
            ImageSystem::Reflect(context);
            MaterialSystem::Reflect(context);
            ModelSystem::Reflect(context);
            ShaderSystem::Reflect(context);
            PassSystem::Reflect(context);

            ResourcePoolAsset::Reflect(context);

            SceneDescriptor::Reflect(context);
            PipelineRenderSettings::Reflect(context);
            RenderPipelineDescriptor::Reflect(context);
            AssetAliases::Reflect(context);

            RPISystemDescriptor::Reflect(context);
            GpuQuerySystemDescriptor::Reflect(context);

            PipelineStatisticsResult::Reflect(context);

            PerformaceCollectionNotification::Reflect(context);
        }

        void RPISystem::Initialize(const RPISystemDescriptor& rpiSystemDescriptor)
        {
            // Init RHI device(s)
            auto commandLineMultipleDevicesValue{ RHI::GetCommandLineValue("device-count") };
            m_rhiSystem.InitDevices((commandLineMultipleDevicesValue != "") ? AZStd::stoi(commandLineMultipleDevicesValue) : 1);

            // Gather asset handlers from sub-systems.
            ImageSystem::GetAssetHandlers(m_assetHandlers);
            BufferSystem::GetAssetHandlers(m_assetHandlers);
            MaterialSystem::GetAssetHandlers(m_assetHandlers);
            ModelSystem::GetAssetHandlers(m_assetHandlers);
            PassSystem::GetAssetHandlers(m_assetHandlers);
            ShaderSystem::GetAssetHandlers(m_assetHandlers);
            m_assetHandlers.emplace_back(MakeAssetHandler<ResourcePoolAssetHandler>());
            m_assetHandlers.emplace_back(MakeAssetHandler<AnyAssetHandler>());

            m_materialSystem.Init();
            m_modelSystem.Init();
            m_shaderSystem.Init();
            m_passSystem.Init();
            m_featureProcessorFactory.Init();
            m_querySystem.Init(m_descriptor.m_gpuQuerySystemDescriptor);

            Interface<RPISystemInterface>::Register(this);

            SystemTickBus::Handler::BusConnect();

#if AZ_RPI_PRINT_GLOBAL_STATE_ON_ASSERT
            Debug::TraceMessageBus::Handler::BusConnect();
#endif
            m_descriptor = rpiSystemDescriptor;
        }

        void RPISystem::Shutdown()
        {
            m_viewportContextManager.Shutdown();
            m_viewSrgLayout = nullptr;
            m_sceneSrgLayout = nullptr;
            m_commonShaderAssetForSrgs.Reset();

#if AZ_RPI_PRINT_GLOBAL_STATE_ON_ASSERT
            Debug::TraceMessageBus::Handler::BusDisconnect();
#endif
            SystemTickBus::Handler::BusDisconnect();

            Interface<RPISystemInterface>::Unregister(this);

            m_featureProcessorFactory.Shutdown();
            m_passSystem.Shutdown();
            m_dynamicDraw.Shutdown();
            m_bufferSystem.Shutdown();
            m_materialSystem.Shutdown();
            m_modelSystem.Shutdown();
            m_shaderSystem.Shutdown();
            m_imageSystem.Shutdown();
            m_querySystem.Shutdown();
            m_rhiSystem.Shutdown();

            /**
             * [LY-86745] We need to pump the asset manager queue here, because
             * it uses AZStd::function<> with vtable pointers embedded from this DLL.
             * If we allow the DLL to shutdown with queued events, they will be pumped
             * later by the asset manager component, which will then reference garbage
             * vtable pointers.
             *
             * Note that it's necessary to pump before *and* after we clear the handlers,
             * since the handler clear could result in more events dispatched.
             */
            Data::AssetManager::Instance().DispatchEvents();
            m_assetHandlers.clear();
            Data::AssetManager::Instance().DispatchEvents();
        }

        void RPISystem::RegisterScene(ScenePtr scene)
        {
            for (auto& sceneItem : m_scenes)
            {
                if (sceneItem == scene)
                {
                    AZ_Assert(false, "Scene was already registered");
                    return;
                }
                else if (!scene->GetName().IsEmpty() && scene->GetName() == sceneItem->GetName())
                {
                    // only report a warning if there is a scene with duplicated name
                    AZ_Warning("RPISystem", false, "There is a registered scene with same name [%s]", scene->GetName().GetCStr());
                }
            }

            m_scenes.push_back(scene);
        }

        void RPISystem::UnregisterScene(ScenePtr scene)
        {
            for (auto itr = m_scenes.begin(); itr != m_scenes.end(); itr++)
            {
                if (*itr == scene)
                {
                    m_scenes.erase(itr);
                    return;
                }
            }
            AZ_Assert(false, "Can't unregister scene which wasn't registered");
        }

        Scene* RPISystem::GetScene(const SceneId& sceneId) const
        {
            for (const auto& scene : m_scenes)
            {
                if (scene->GetId() == sceneId)
                {
                    return scene.get();
                }
            }
            return nullptr;
        }

        Scene* RPISystem::GetSceneByName(const AZ::Name& name) const
        {
            for (const auto& scene : m_scenes)
            {
                if (scene->GetName() == name)
                {
                    return scene.get();
                }
            }
            return nullptr;
        }

        uint32_t RPISystem::GetNumScenes() const
        {
            return aznumeric_cast<uint32_t>(m_scenes.size());
        }
        
        ScenePtr RPISystem::GetDefaultScene() const
        {
            for (const auto& scene : m_scenes)
            {
                if (scene->GetName() == AZ::Name("Main"))
                {
                    return scene;
                }
            }
            return nullptr;
        }

        RenderPipelinePtr RPISystem::GetRenderPipelineForWindow(AzFramework::NativeWindowHandle windowHandle)
        {
            RenderPipelinePtr renderPipeline;
            for (auto& scene : m_scenes)
            {
                renderPipeline = scene->FindRenderPipelineForWindow(windowHandle);
                if (renderPipeline)
                {
                    return renderPipeline;
                }
            }
            return nullptr;
        }

        Data::Asset<ShaderAsset> RPISystem::GetCommonShaderAssetForSrgs() const
        {
            AZ_Assert(m_systemAssetsInitialized, "InitializeSystemAssets() should be called once when asset catalog loaded'");
            return m_commonShaderAssetForSrgs;
        }

        RHI::Ptr<RHI::ShaderResourceGroupLayout> RPISystem::GetSceneSrgLayout() const
        {
            AZ_Assert(m_systemAssetsInitialized, "InitializeSystemAssets() should be called once when asset catalog loaded'");
            return m_sceneSrgLayout;
        }

        RHI::Ptr<RHI::ShaderResourceGroupLayout> RPISystem::GetViewSrgLayout() const
        {
            AZ_Assert(m_systemAssetsInitialized, "InitializeSystemAssets() should be called once when asset catalog loaded'");
            return m_viewSrgLayout;
        }

        void RPISystem::OnSystemTick()
        {
            AZ_PROFILE_SCOPE(RPI, "RPISystem: OnSystemTick");

            // Image system update is using system tick but not game tick so it can stream images in background even game is pausing
            m_imageSystem.Update();
        }

        void RPISystem::SimulationTick()
        {
            if (!m_systemAssetsInitialized || IsNullRenderer())
            {
                return;
            }
            AZ_PROFILE_SCOPE(RPI, "RPISystem: SimulationTick");

            AssetInitBus::Broadcast(&AssetInitBus::Events::PostLoadInit);

            m_currentSimulationTime = GetCurrentTime();

            for (auto& scene : m_scenes)
            {
                scene->Simulate(m_simulationJobPolicy, m_currentSimulationTime);
            }
        }

        float RPISystem::GetCurrentTime() const
        {
            const AZ::TimeUs currentSimulationTimeUs = AZ::GetRealElapsedTimeUs();
            return AZ::TimeUsToSeconds(currentSimulationTimeUs);
        }

        void RPISystem::InitXRSystem()
        {
            // The creation of an XR Session requires an asset that defines
            // the action bindings for the application. This means the asset catalog
            // must be available before creating the XR Session.
            AZ_Assert(m_systemAssetsInitialized, "IntXRSystem should not be called before the asset system is ready.");

            if (!m_xrSystem)
            {
                return;
            }

            auto xrRender = m_xrSystem->GetRHIXRRenderingInterface();
            if (!xrRender)
            {
                return;
            }

            RHI::Ptr<RHI::XRDeviceDescriptor> xrDescriptor = m_rhiSystem.GetDevice()->BuildXRDescriptor();
            [[maybe_unused]] auto result = xrRender->CreateDevice(xrDescriptor.get());
            AZ_Error("RPISystem", result == RHI::ResultCode::Success, "Failed to initialize XR device");
            AZ::RHI::XRSessionDescriptor sessionDescriptor;
            result = xrRender->CreateSession(&sessionDescriptor);
            AZ_Error("RPISystem", result == RHI::ResultCode::Success, "Failed to initialize XR session");
            result = xrRender->CreateSwapChain();
            AZ_Error("RPISystem", result == RHI::ResultCode::Success, "Failed to initialize XR swapchain");
        }

        void RPISystem::RenderTick()
        {
            if (!m_systemAssetsInitialized || IsNullRenderer())
            {
                m_dynamicDraw.FrameEnd();
                return;
            }

            AZ_PROFILE_SCOPE(RPI, "RPISystem: RenderTick");

            // Query system update is to increment the frame count
            m_querySystem.Update();

            // Collect draw packets for each scene and prepare RPI system SRGs
            // [GFX TODO] We may parallel scenes' prepare render.
            for (auto& scenePtr : m_scenes)
            {
                scenePtr->PrepareRender(m_prepareRenderJobPolicy, m_currentSimulationTime);
            }

            //Collect all the active pipelines running in this frame.
            uint16_t numActiveRenderPipelines = 0;
            for (auto& scenePtr : m_scenes)
            {
                numActiveRenderPipelines += scenePtr->GetActiveRenderPipelines();
            }
            m_rhiSystem.SetNumActiveRenderPipelines(numActiveRenderPipelines);

            m_rhiSystem.FrameUpdate(
                [this](RHI::FrameGraphBuilder& frameGraphBuilder)
                {
                    // Pass system's frame update, which includes the logic of adding scope producers, has to be added here since the
                    // scope producers only can be added to the frame when frame started which cleans up previous scope producers.
                    m_passSystem.FrameUpdate(frameGraphBuilder);

                    // Update Scene and View Srgs
                    for (auto& scenePtr : m_scenes)
                    {
                        scenePtr->UpdateSrgs();
                    }
                });

            {
                AZ_PROFILE_SCOPE(RPI, "RPISystem: FrameEnd");
                m_dynamicDraw.FrameEnd();
                m_passSystem.FrameEnd();

                for (auto& scenePtr : m_scenes)
                {
                    scenePtr->OnFrameEnd();
                }
            }

            m_renderTick++;
        }

        void RPISystem::SetSimulationJobPolicy(RHI::JobPolicy jobPolicy)
        {
            m_simulationJobPolicy = jobPolicy;
        }

        RHI::JobPolicy RPISystem::GetSimulationJobPolicy() const
        {
            return m_simulationJobPolicy;
        }

        void RPISystem::SetRenderPrepareJobPolicy(RHI::JobPolicy jobPolicy)
        {
            m_prepareRenderJobPolicy = jobPolicy;
        }

        RHI::JobPolicy RPISystem::GetRenderPrepareJobPolicy() const
        {
            return m_prepareRenderJobPolicy;
        }

        const RPISystemDescriptor& RPISystem::GetDescriptor() const
        {
            return m_descriptor;
        }

        Name RPISystem::GetRenderApiName() const
        {
            return RHI::Factory::Get().GetName();
        }

        void RPISystem::InitializeSystemAssets()
        {
            if (m_systemAssetsInitialized)
            {
                return;
            }

            m_commonShaderAssetForSrgs = AssetUtils::LoadCriticalAsset<ShaderAsset>( m_descriptor.m_commonSrgsShaderAssetPath.c_str());
            if (!m_commonShaderAssetForSrgs.IsReady())
            {
                AZ_Error("RPI system", false, "Failed to load RPI system asset %s", m_descriptor.m_commonSrgsShaderAssetPath.c_str());
                return;
            }

            m_sceneSrgLayout = m_commonShaderAssetForSrgs->FindShaderResourceGroupLayout(SrgBindingSlot::Scene);
            if (!m_sceneSrgLayout)
            {
                AZ_Error("RPISystem", false, "Failed to find SceneSrg by slot=<%u> from shader asset at path <%s>", SrgBindingSlot::Scene,
                    m_descriptor.m_commonSrgsShaderAssetPath.c_str());
                return;
            }
            m_viewSrgLayout = m_commonShaderAssetForSrgs->FindShaderResourceGroupLayout(SrgBindingSlot::View);
            if (!m_viewSrgLayout)
            {
                AZ_Error("RPISystem", false, "Failed to find ViewSrg by slot=<%u> from shader asset at path <%s>", SrgBindingSlot::View,
                    m_descriptor.m_commonSrgsShaderAssetPath.c_str());
                return;
            }
            RHI::Ptr<RHI::ShaderResourceGroupLayout> bindlessSrgLayout = m_commonShaderAssetForSrgs->FindShaderResourceGroupLayout(SrgBindingSlot::Bindless);
            if (!bindlessSrgLayout)
            {
                AZ_Error(
                    "RPISystem",
                    false,
                    "Failed to find BindlessSrg by slot=<%u> from shader asset at path <%s>",
                    SrgBindingSlot::Bindless,
                    m_descriptor.m_commonSrgsShaderAssetPath.c_str());
                return;
            }

            m_rhiSystem.Init(bindlessSrgLayout);
            m_imageSystem.Init(m_descriptor.m_imageSystemDescriptor);
            m_bufferSystem.Init();
            m_dynamicDraw.Init(m_descriptor.m_dynamicDrawSystemDescriptor);

            m_passSystem.InitPassTemplates();

            m_systemAssetsInitialized = true;
            AZ_TracePrintf("RPI system", "System assets initialized\n");

            // Now that the asset system is up and running, we can safely initialize
            // the XR System and the XR Session.
            InitXRSystem();
        }

        bool RPISystem::IsInitialized() const
        {
            return m_systemAssetsInitialized;
        }

        bool RPISystem::IsNullRenderer() const
        {
            return m_descriptor.m_isNullRenderer;
        }

        void RPISystem::InitializeSystemAssetsForTests()
        {
            if (m_systemAssetsInitialized)
            {
                AZ_Warning("RPISystem", false, "InitializeSystemAssets should only be called once'");
                return;
            }

            //Init rhi/image/buffer systems to match InitializeSystemAssets
            m_rhiSystem.Init();
            m_imageSystem.Init(m_descriptor.m_imageSystemDescriptor);
            m_bufferSystem.Init();

            // Assets aren't actually available or needed for tests, but the m_systemAssetsInitialized flag still needs to be flipped.
            m_systemAssetsInitialized = true;
            return;
        }

        bool RPISystem::OnPreAssert([[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message)
        {
#if AZ_RPI_PRINT_GLOBAL_STATE_ON_ASSERT
            AZ_Printf("RPI System", "\n--- Assert hit! Dumping RPI state ---\n\n");
            m_passSystem.DebugPrintPassHierarchy();
#endif
            return false;
        }
                
        uint64_t RPISystem::GetCurrentTick() const
        {
            return m_renderTick;
        }

        void RPISystem::SetApplicationMultisampleState(const RHI::MultisampleState& multisampleState)
        {
            m_multisampleState = multisampleState;

            bool isNonMsaaPipeline = (m_multisampleState.m_samples == 1);
            const char* supervariantName = isNonMsaaPipeline ? AZ::RPI::NoMsaaSupervariantName : "";
            AZ::RPI::ShaderSystemInterface::Get()->SetSupervariantName(AZ::Name(supervariantName));

            // reinitialize pipelines for all scenes
            for (auto& scene : m_scenes)
            {
                for (auto& renderPipeline : scene->GetRenderPipelines())
                {
                    // MSAA state set to the render pipeline at creation time from its data might be different
                    // from the one set to the application. So it can arrive here having the same new
                    // target state, but still needs to be marked as its MSAA state has changed so its passes
                    // are recreated using the new supervariant name coming from MSAA at application level just set above.
                    // In conclusion, it's not safe to skip here setting MSAA state to the render pipeline when it's the
                    // same as the target.
                    renderPipeline->GetRenderSettings().m_multisampleState = multisampleState;
                    renderPipeline->MarkPipelinePassChanges(PipelinePassChanges::MultisampleStateChanged);
                }
            }
        }

        const RHI::MultisampleState& RPISystem::GetApplicationMultisampleState() const
        {
            return m_multisampleState;
        }

        void RPISystem::RegisterXRSystem(XRRenderingInterface* xrSystemInterface)
        { 
            AZ_Assert(!m_xrSystem, "XR System is already registered");
            if (m_rhiSystem.RegisterXRSystem(xrSystemInterface->GetRHIXRRenderingInterface()))
            {
                m_xrSystem = xrSystemInterface;
            }
        }

        void RPISystem::UnregisterXRSystem()
        {
            AZ_Assert(m_xrSystem, "XR System is not registered");
            if (m_xrSystem)
            {
                m_rhiSystem.UnregisterXRSystem();
                m_xrSystem = nullptr;
            }
        }

        XRRenderingInterface* RPISystem::GetXRSystem() const
        {
            return m_xrSystem;
        } 
    } //namespace RPI
} //namespace AZ
