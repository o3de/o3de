/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SampleComponentManager.h>
#include <SampleComponentConfig.h>

#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>
#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>
#include <Atom/Feature/AuxGeom/AuxGeomFeatureProcessor.h>
#include <Atom/Feature/ImGui/ImGuiUtils.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/PostProcessing/PostProcessingConstants.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/AliasedHeapEnums.h>

#include <Automation/ScriptManager.h>

#include <RHI/AlphaToCoverageExampleComponent.h>
#include <RHI/AsyncComputeExampleComponent.h>
#include <RHI/BasicRHIComponent.h>
#include <RHI/ComputeExampleComponent.h>
#include <RHI/CopyQueueComponent.h>
#include <RHI/DualSourceBlendingComponent.h>
#include <RHI/IndirectRenderingExampleComponent.h>
#include <RHI/InputAssemblyExampleComponent.h>
#include <RHI/SubpassExampleComponent.h>
#include <RHI/MRTExampleComponent.h>
#include <RHI/MSAAExampleComponent.h>
#include <RHI/MultiThreadComponent.h>
#include <RHI/MultiViewportSwapchainComponent.h>
#include <RHI/MultipleViewsComponent.h>
#include <RHI/QueryExampleComponent.h>
#include <RHI/StencilExampleComponent.h>
#include <RHI/SwapchainExampleComponent.h>
#include <RHI/SphericalHarmonicsExampleComponent.h>
#include <RHI/Texture3dExampleComponent.h>
#include <RHI/TextureArrayExampleComponent.h>
#include <RHI/TextureExampleComponent.h>
#include <RHI/TextureMapExampleComponent.h>
#include <RHI/TriangleExampleComponent.h>
#include <RHI/TrianglesConstantBufferExampleComponent.h>
#include <RHI/BindlessPrototypeExampleComponent.h>
#include <RHI/RayTracingExampleComponent.h>
#include <RHI/MatrixAlignmentTestExampleComponent.h>

#include <Performance/100KDrawable_SingleView_ExampleComponent.h>
#include <Performance/100KDraw_10KDrawable_MultiView_ExampleComponent.h>

#include <AreaLightExampleComponent.h>
#include <AssetLoadTestComponent.h>
#include <AuxGeomExampleComponent.h>
#include <BakedShaderVariantExampleComponent.h>
#include <SponzaBenchmarkComponent.h>
#include <BloomExampleComponent.h>
#include <CheckerboardExampleComponent.h>
#include <CullingAndLodExampleComponent.h>
#include <DecalExampleComponent.h>
#include <DepthOfFieldExampleComponent.h>
#include <DynamicDrawExampleComponent.h>
#include <DynamicMaterialTestComponent.h>
#include <MaterialHotReloadTestComponent.h>
#include <ExposureExampleComponent.h>
#include <EyeMaterialExampleComponent.h>
#include <SceneReloadSoakTestComponent.h>
#include <LightCullingExampleComponent.h>
#include <MeshExampleComponent.h>
#include <MeshletsExampleComponent.h>
#include <MSAA_RPI_ExampleComponent.h>
#include <MultiRenderPipelineExampleComponent.h>
#include <MultiSceneExampleComponent.h>
#include <ParallaxMappingExampleComponent.h>
#include <RenderTargetTextureExampleComponent.h>
#include <SceneReloadSoakTestComponent.h>
#include <ShadowExampleComponent.h>
#include <ShadowedSponzaExampleComponent.h>
#include <SkinnedMeshExampleComponent.h>
#include <SsaoExampleComponent.h>
#include <StreamingImageExampleComponent.h>
#include <RootConstantsExampleComponent.h>
#include <MultiViewSingleSceneAuxGeomExampleComponent.h>
#include <TonemappingExampleComponent.h>
#include <TransparencyExampleComponent.h>
#include <DiffuseGIExampleComponent.h>
#include <SSRExampleComponent.h>
#include <ShaderReloadTestComponent.h>
#include <ReadbackExampleComponent.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/IO/IStreamerProfiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/algorithm.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <Passes/RayTracingAmbientOcclusionPass.h>

#include <Utils/Utils.h>

#include <Profiler/ProfilerImGuiBus.h>

#include "ExampleComponentBus.h"
#include <EntityUtilityFunctions.h>

namespace Platform
{
    const char* GetPipelineName();
}

namespace AtomSampleViewer
{
    namespace
    {
        constexpr const char* PassTreeToolName = "PassTree";
        constexpr const char* CpuProfilerToolName = "CPU Profiler";
        constexpr const char* GpuProfilerToolName = "GPU Profiler";
        constexpr const char* FileIoProfilerToolName = "File IO Profiler";
        constexpr const char* TransientAttachmentProfilerToolName = "Transient Attachment Profiler";
    }

    bool IsValidNumMSAASamples(int numSamples)
    {
        return (numSamples == 1) || (numSamples == 2) || (numSamples == 4) || (numSamples == 8);
    }

    template <typename T>
    static SampleEntry NewSample(SamplePipelineType type, const char* menuName, const AZStd::string& name)
    {
        SampleEntry entry;
        entry.m_sampleName = name;
        entry.m_sampleUuid = azrtti_typeid<T>();
        entry.m_pipelineType = type;
        entry.m_componentDescriptor = T::CreateDescriptor();
        entry.m_parentMenuName = menuName;
        entry.m_fullName = entry.m_parentMenuName + '/' + entry.m_sampleName;

        return entry;
    }

    template <typename T>
    static SampleEntry NewSample(SamplePipelineType type, const char* menuName, const AZStd::string& name, AZStd::function<bool()> isSupportedFunction)
    {
        SampleEntry entry = NewSample<T>(type, menuName, name);
        entry.m_isSupportedFunc = isSupportedFunction;
        return entry;
    }

    template <typename T>
    static SampleEntry NewRHISample(const AZStd::string& name)
    {
        return NewSample<T>(SamplePipelineType::RHI, "RHI", name);
    }

    template <typename T>
    static SampleEntry NewRHISample(const AZStd::string& name, AZStd::function<bool()> isSupportedFunction)
    {
        SampleEntry entry = NewSample<T>(SamplePipelineType::RHI, "RHI", name, isSupportedFunction);
        entry.m_isSupportedFunc = isSupportedFunction;
        return entry;
    }

    template <typename T>
    static SampleEntry NewRPISample(const AZStd::string& name)
    {
        return NewSample<T>(SamplePipelineType::RPI, "RPI", name);
    }

    template <typename T>
    static SampleEntry NewRPISample(const AZStd::string& name, AZStd::function<bool()> isSupportedFunction)
    {
        return NewSample<T>(SamplePipelineType::RPI, "RPI", name, isSupportedFunction);
    }

    template <typename T>
    static SampleEntry NewFeaturesSample(const AZStd::string& name)
    {
        return NewSample<T>(SamplePipelineType::RPI, "Features", name);
    }

    template <typename T>
    static SampleEntry NewFeaturesSample(const AZStd::string& name, AZStd::function<bool()> isSupportedFunction)
    {
        return NewSample<T>(SamplePipelineType::RPI, "Features", name, isSupportedFunction);
    }

    template <typename T>
    static SampleEntry NewPerfSample(const AZStd::string& name)
    {
        return NewSample<T>(SamplePipelineType::RPI, "Performance", name);
    }

    template <typename T>
    static SampleEntry NewPerfSample(const AZStd::string& name, AZStd::function<bool()> isSupportedFunction)
    {
        return NewSample<T>(SamplePipelineType::RPI, "Performance", name, isSupportedFunction);
    }

    void SampleComponentManager::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SampleComponentManager, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void SampleComponentManager::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PrototypeLmbrCentralService", 0xe35e6de0));
    }

    void SampleComponentManager::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AzFrameworkConfigurationSystemComponentService", 0xcc49c96e)); // Ensures a scene is created for the GameEntityContext
    }

    AZStd::vector<SampleEntry> SampleComponentManager::GetSamples()
    {
        return {
            NewRHISample<AlphaToCoverageExampleComponent>("AlphaToCoverage"),
            NewRHISample<AsyncComputeExampleComponent>("AsyncCompute"),
            NewRHISample<BindlessPrototypeExampleComponent>("BindlessPrototype", []() {return Utils::GetRHIDevice()->GetFeatures().m_unboundedArrays; }),
            NewRHISample<ComputeExampleComponent>("Compute"),
            NewRHISample<CopyQueueComponent>("CopyQueue"),
            NewRHISample<DualSourceBlendingComponent>("DualSourceBlending", []() {return Utils::GetRHIDevice()->GetFeatures().m_dualSourceBlending; }),
            NewRHISample<IndirectRenderingExampleComponent>("IndirectRendering", []() {return Utils::GetRHIDevice()->GetFeatures().m_indirectCommandTier > RHI::IndirectCommandTiers::Tier0; }),
            NewRHISample<InputAssemblyExampleComponent>("InputAssembly"),
            NewRHISample<MSAAExampleComponent>("MSAA"),
            NewRHISample<MultipleViewsComponent>("MultipleViews"),
            NewRHISample<MRTExampleComponent>("MultiRenderTarget"),
            NewRHISample<MultiThreadComponent>("MultiThread"),
            NewRHISample<MultiViewportSwapchainComponent>("MultiViewportSwapchainComponent", [] { return IsMultiViewportSwapchainSampleSupported(); }),
            NewRHISample<QueryExampleComponent>("Queries"),
            NewRHISample<RayTracingExampleComponent>("RayTracing", []() {return Utils::GetRHIDevice()->GetFeatures().m_rayTracing; }),
            NewRHISample<SphericalHarmonicsExampleComponent>("SphericalHarmonics"),
            NewRHISample<StencilExampleComponent>("Stencil"),
            NewRHISample<SubpassExampleComponent>("Subpass", []() {return Utils::GetRHIDevice()->GetFeatures().m_renderTargetSubpassInputSupport != AZ::RHI::SubpassInputSupportType::NotSupported; }),
            NewRHISample<SwapchainExampleComponent>("Swapchain"),
            NewRHISample<TextureExampleComponent>("Texture"),
            NewRHISample<Texture3dExampleComponent>("Texture3d"),
            NewRHISample<TextureArrayExampleComponent>("TextureArray"),
            NewRHISample<TextureMapExampleComponent>("TextureMap"),
            NewRHISample<TriangleExampleComponent>("Triangle"),
            NewRHISample<TrianglesConstantBufferExampleComponent>("TrianglesConstantBuffer"),
            NewRHISample<MatrixAlignmentTestExampleComponent>("MatrixAlignmentTest"),
            NewRPISample<AssetLoadTestComponent>("AssetLoadTest"),
            NewRPISample<AuxGeomExampleComponent>("AuxGeom"),
            NewRPISample<BakedShaderVariantExampleComponent>("BakedShaderVariant"),
            NewRPISample<SponzaBenchmarkComponent>("SponzaBenchmark"),
            NewRPISample<CullingAndLodExampleComponent>("CullingAndLod"),
            NewRPISample<DecalExampleComponent>("Decals"),
            NewRPISample<DynamicDrawExampleComponent>("DynamicDraw"),
            NewRPISample<DynamicMaterialTestComponent>("DynamicMaterialTest"),
            NewRPISample<MaterialHotReloadTestComponent>("MaterialHotReloadTest"),
            NewRPISample<MeshExampleComponent>("Mesh"),
            NewRPISample<MeshletsExampleComponent>("Meshlets"),
            NewRPISample<MSAA_RPI_ExampleComponent>("MSAA"),
            NewRPISample<MultiRenderPipelineExampleComponent>("MultiRenderPipeline"),
            NewRPISample<MultiSceneExampleComponent>("MultiScene"),
            NewRPISample<MultiViewSingleSceneAuxGeomExampleComponent>("MultiViewSingleSceneAuxGeom"),
            NewRPISample<ReadbackExampleComponent>("Readback"),
            NewRPISample<RenderTargetTextureExampleComponent>("RenderTargetTexture"),
            NewRPISample<RootConstantsExampleComponent>("RootConstants"),
            NewRPISample<SceneReloadSoakTestComponent>("SceneReloadSoakTest"),
            NewRPISample<StreamingImageExampleComponent>("StreamingImage"),
            NewRPISample<ShaderReloadTestComponent>("ShaderReloadTest"),
            NewFeaturesSample<AreaLightExampleComponent>("AreaLight"),
            NewFeaturesSample<BloomExampleComponent>("Bloom"),
            NewFeaturesSample<CheckerboardExampleComponent>("Checkerboard", []() {return (Utils::GetRHIDevice()->GetPhysicalDevice().GetDescriptor().m_vendorId != RHI::VendorId::ARM && Utils::GetRHIDevice()->GetPhysicalDevice().GetDescriptor().m_vendorId != RHI::VendorId::Qualcomm); }),
            NewFeaturesSample<DepthOfFieldExampleComponent>("DepthOfField"),
            NewFeaturesSample<DiffuseGIExampleComponent>("DiffuseGI", []() {return Utils::GetRHIDevice()->GetFeatures().m_rayTracing; }),
            NewFeaturesSample<ExposureExampleComponent>("Exposure"),
            NewFeaturesSample<EyeMaterialExampleComponent>("EyeMaterial"),
            NewFeaturesSample<LightCullingExampleComponent>("LightCulling"),
            NewFeaturesSample<ParallaxMappingExampleComponent>("Parallax"),
            NewFeaturesSample<ShadowExampleComponent>("Shadow"),
            NewFeaturesSample<ShadowedSponzaExampleComponent>("ShadowedSponza"),
            NewFeaturesSample<SkinnedMeshExampleComponent>("SkinnedMesh"),
            NewFeaturesSample<SsaoExampleComponent>("SSAO"),
            NewFeaturesSample<SSRExampleComponent>("SSR"),
            NewFeaturesSample<TonemappingExampleComponent>("Tonemapping"),
            NewFeaturesSample<TransparencyExampleComponent>("Transparency"),
            NewPerfSample<_100KDrawableExampleComponent>("100KDrawable_SingleView"),
            NewPerfSample<_100KDraw10KDrawableExampleComponent>("100KDraw_10KDrawable_MultiView"),
        };
    }

    void SampleComponentManager::RegisterSampleComponent(const SampleEntry& sample)
    {
        if (AZStd::find(m_availableSamples.begin(), m_availableSamples.end(), sample) == m_availableSamples.end())
        {
            m_availableSamples.push_back(sample);
            m_groupedSamples[sample.m_parentMenuName].push_back(static_cast<int32_t>(m_availableSamples.size() - 1));
        }
    }

    SampleComponentManager::SampleComponentManager()
        : m_imguiFrameCaptureSaver("@user@/frame_capture.xml")
        , m_imGuiFrameTimer(FrameTimeLogSize, FrameTimeLogSize, 250.0f)
    {
        m_exampleEntity = aznew AZ::Entity();

        m_entityContextId = AzFramework::EntityContextId::CreateNull();

        memset(m_alphanumericNumbersDown, 0, s_alphanumericCount);
    }

    SampleComponentManager::~SampleComponentManager()
    {
        m_exampleEntity = nullptr;
        m_cameraEntity = nullptr;
        m_windowContext = nullptr;

        m_availableSamples.clear();
    }

    void SampleComponentManager::Init()
    {
        AZStd::vector<SampleEntry> samples = GetSamples();
        for (const SampleEntry& sample : samples)
        {
            RegisterSampleComponent(sample);
        }

        m_scriptManager = AZStd::make_unique<ScriptManager>();
    }

    void SampleComponentManager::Activate()
    {
        // We can only initialize this component after the asset catalog has been loaded.
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusConnect();

        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        passSystem->AddPassCreator(Name("RayTracingAmbientOcclusionPass"), &AZ::Render::RayTracingAmbientOcclusionPass::Create);

        m_numMSAASamples = GetDefaultNumMSAASamples();
    }

    void SampleComponentManager::ActivateInternal()
    {
        using namespace AZ;

        AZ::ApplicationTypeQuery appType;
        ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (!appType.IsValid() || !appType.IsGame())
        {
            return;
        }

        // ActivateInternal() may get called twice because the OnCatalogLoaded was called twice When run AtomSampleViewer launcher. One of those two events is from ly launcher framework and the other one is from
        // LoadCatalog call in AtomSampleViewer system component. Although load the same asset catalog twice doesn't seem to cause other issue. 
        if (m_wasActivated)
        {
            return;
        }

        Render::Bootstrap::DefaultWindowBus::BroadcastResult(m_windowContext, &Render::Bootstrap::DefaultWindowBus::Events::GetDefaultWindowContext);
        AzFramework::GameEntityContextRequestBus::BroadcastResult(m_entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

        CreateDefaultCamera();

        // Add customized pass classes
        auto* passSystem = RPI::PassSystemInterface::Get();
        passSystem->AddPassCreator(Name("RHISamplePass"), &AtomSampleViewer::RHISamplePass::Create);

        // Load ASV's own pass templates mapping
        // It can be loaded here and it doesn't need be added via OnReadyLoadTemplatesEvent::Handler
        // since the first render pipeline is created after this point.
        const char* asvPassTemplatesFile = "Passes/ASV/PassTemplates.azasset";
        bool loaded = passSystem->LoadPassTemplateMappings(asvPassTemplatesFile);
        if (!loaded)
        {
            AZ_Fatal("SampleComponentManager", "Failed to load AtomSampleViewer's pass templates at %s", asvPassTemplatesFile);
            return;
        }

        // Use scene and render pipeline for RHI samples as default scene and render pipeline
        CreateSceneForRHISample();

        m_exampleEntity->Init();
        m_exampleEntity->Activate();

        m_isSampleSupported.resize(m_availableSamples.size());
        for (size_t i = 0; i < m_availableSamples.size(); ++i)
        {
            // Assume that the sample is supported if no m_isSupportedFunc is provided.
            m_isSampleSupported[i] = m_availableSamples[i].m_isSupportedFunc ? m_availableSamples[i].m_isSupportedFunc() : true;
        }

        AZ_Printf("SampleComponentManager", "Available Samples -------------------------\n");
        for (size_t i = 0; i < m_availableSamples.size(); ++i)
        {
            AZStd::string printStr = "\t[" + m_availableSamples[i].m_fullName + "]";
            if (!m_isSampleSupported[i])
            {
                printStr += " Not Supported ";
            }

            if (i < 9)
            {
                printStr += AZStd::string::format("\tctrl+%lu", i + 1);
            }

            printStr += "\n";
            AZ_Printf("SampleComponentManager", printStr.data());
        }
        AZ_Printf("SampleComponentManager", "-------------------------------------------\n");

        AzFramework::InputChannelEventListener::BusConnect();
        TickBus::Handler::BusConnect();

        bool targetSampleFound = false;

        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
        if (commandLine->HasSwitch("sample"))
        {
            AZStd::string targetSampleName = commandLine->GetSwitchValue("sample", 0);
            AZStd::to_lower(targetSampleName.begin(), targetSampleName.end());

            for (int32_t i = 0; i < m_availableSamples.size(); ++i)
            {
                AZStd::string sampleName = m_availableSamples[i].m_fullName;
                AZStd::to_lower(sampleName.begin(), sampleName.end());

                if (sampleName == targetSampleName)
                {
                    if (m_isSampleSupported[i])
                    {
                        targetSampleFound = true;
                        m_selectedSampleIndex = i;
                        m_sampleChangeRequest = true;
                    }

                    break;
                }
            }
            AZ_Warning("SampleComponentManager", targetSampleFound, "Failed find target sample %s", targetSampleName.c_str());
        }

        // Set default screenshot folder to relative path 'Screenshots'
        AZ::IO::Path screenshotFolder = "Screenshots";
        // Get folder from command line if it exists
        static const char* screenshotFolderFlagName = "screenshotFolder";
        if (commandLine->HasSwitch(screenshotFolderFlagName))
        {
            screenshotFolder = commandLine->GetSwitchValue(screenshotFolderFlagName, 0);
        }

        // Make the screenshot directory relative to the Writeable Storage Path
        // The Path::operator/ smartly knows how to concatenate two absolute paths
        // i.e <absolute path1> / <absolute path2> = <absolute path2>
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::Path writableStoragePath;
        settingsRegistry->Get(writableStoragePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage);
        screenshotFolder = writableStoragePath / screenshotFolder;

        m_imguiFrameCaptureSaver.SetDefaultFolder(screenshotFolder.Native());
        m_imguiFrameCaptureSaver.SetDefaultFileName("screenshot");
        m_imguiFrameCaptureSaver.SetAvailableExtensions({ "png", "ppm", "dds" });
        m_imguiFrameCaptureSaver.Activate();

        SampleComponentManagerRequestBus::Handler::BusConnect();
        m_scriptManager->Activate();

        m_wasActivated = true;

        SampleComponentManagerNotificationBus::Broadcast(&SampleComponentManagerNotificationBus::Events::OnSampleManagerActivated);
    }

    void SampleComponentManager::Deactivate()
    {
        DestroyEntity(m_cameraEntity);

        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AZ::Render::ImGuiSystemNotificationBus::Handler::BusDisconnect();
        m_scriptManager->Deactivate();
        m_imguiFrameCaptureSaver.Deactivate();
        SampleComponentManagerRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();

        Render::ImGuiSystemRequestBus::Broadcast(&Render::ImGuiSystemRequests::PopActiveContext);

        m_imguiPassTree.Reset();
        m_imguiFrameGraphVisualizer.Reset();

        m_windowContext = nullptr;
        m_brdfTexture.reset();

        ReleaseRHIScene();
        ReleaseRPIScene();
    }

    void SampleComponentManager::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_imGuiFrameTimer.PushValue(deltaTime * 1000.0f);

        bool screenshotRequest = false;

        if (m_ctrlModifierLDown || m_ctrlModifierRDown)
        {
            if (m_alphanumericQDown)
            {
                RequestExit();
                return;
            }
            if (m_alphanumericTDown && m_canCaptureRADTM)
            {
#ifdef AZ_PROFILE_TELEMETRY
                Utils::ToggleRadTMCapture();
                m_canCaptureRADTM = false;
#endif
            }
            else if (!m_alphanumericTDown)
            {
                m_canCaptureRADTM = true;
            }

            if (m_alphanumericPDown)
            {
                screenshotRequest = true;
            }

            for (size_t i = 0; i < m_availableSamples.size(); ++i)
            {
                if (m_alphanumericNumbersDown[i] && i < s_alphanumericCount && m_isSampleSupported[i])
                {
                    m_sampleChangeRequest = true;
                    m_selectedSampleIndex = static_cast<int32_t>(i);
                    break;
                }
            }
        }

        // Request a frame capture only once per key press, even if the keys are held down for multiple ticks.
        if (screenshotRequest)
        {
            ++m_screenshotKeyDownCount;
            if (m_screenshotKeyDownCount == 1)
            {
                RequestFrameCapture(m_imguiFrameCaptureSaver.GetNextAutoSaveFilePath(), true);
            }
        }
        else
        {
            m_screenshotKeyDownCount = 0;
        }

        RenderImGui(deltaTime);

        // SampleChange needs to happen after RenderImGui as some of the samples create sidebars on a separate ImGui context
        // which can interfere with m_imguiContext in the first frame
        if (m_sampleChangeRequest && m_canSwitchSample)
        {
            SampleChange();
            m_canSwitchSample = false;
        }
        else if (m_escapeDown && m_canSwitchSample)
        {
            Reset();
            m_canSwitchSample = false;
            Utils::ReportScriptableAction("OpenSample('')");
        }

        // Once a SampleChange/Reset request has been handled, it will not be handled again until there has been at least one frame where the sample hasn't changed
        if (!m_sampleChangeRequest && !m_escapeDown)
        {
            m_canSwitchSample = true;
        }

        // Since the event has been handled, clear the request
        m_sampleChangeRequest = false;
        m_escapeDown = false;

        m_scriptManager->TickScript(deltaTime);

        if (m_isFrameCapturePending)
        {
            if (m_countdownForFrameCapture > 0)
            {
                m_countdownForFrameCapture--;
            }
            else if (m_countdownForFrameCapture == 0)
            {
                AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect();
                AZ::Render::FrameCaptureRequestBus::Broadcast(&AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot, m_frameCaptureFilePath);
                m_countdownForFrameCapture = -1; // Don't call CaptureScreenshot again
            }
        }
    }

    bool SampleComponentManager::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const size_t samplesAvailableCount = m_availableSamples.size();

        AZStd::vector<AzFramework::InputChannelId> sampleInputMapping;
        sampleInputMapping.reserve(samplesAvailableCount);

        for (size_t i = 0; i < samplesAvailableCount; ++i)
        {
            switch (i)
            {
            case 0:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric1);
                break;
            case 1:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric2);
                break;
            case 2:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric3);
                break;
            case 3:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric4);
                break;
            case 4:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric5);
                break;
            case 5:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric6);
                break;
            case 6:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric7);
                break;
            case 7:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric8);
                break;
            case 8:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric9);
                break;
            case 9:
                sampleInputMapping.push_back(AzFramework::InputDeviceKeyboard::Key::Alphanumeric0);
                break;
            default:
                break;
            }
        }

        const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
        switch (inputChannel.GetState())
        {
        case AzFramework::InputChannel::State::Began:
        case AzFramework::InputChannel::State::Updated:
        {
            if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_ctrlModifierLDown = true;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
            {
                m_ctrlModifierRDown = true;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericQ)
            {
                m_alphanumericQDown = true;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericT)
            {
                m_alphanumericTDown = true;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericP)
            {
                m_alphanumericPDown = true;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::Escape)
            {
                m_escapeDown = true;
            }

            for (size_t i = 0; i < samplesAvailableCount; ++i)
            {
                if ((i < s_alphanumericCount) && (inputChannelId == sampleInputMapping[i]))
                {
                    m_alphanumericNumbersDown[i] = true;
                }
            }

            break;
        }
        case AzFramework::InputChannel::State::Ended:
        {
            if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_ctrlModifierLDown = false;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::ModifierCtrlR)
            {
                m_ctrlModifierRDown = false;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericQ)
            {
                m_alphanumericQDown = false;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericT)
            {
                m_alphanumericTDown = false;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericP)
            {
                m_alphanumericPDown = false;
            }
            else if (inputChannelId == AzFramework::InputDeviceKeyboard::Key::Escape)
            {
                m_escapeDown = false;
            }

            for (size_t i = 0; i < samplesAvailableCount; ++i)
            {
                if ((i < s_alphanumericCount) && (inputChannelId == sampleInputMapping[i]))
                {
                    m_alphanumericNumbersDown[i] = false;
                }
            }

            break;
        }
        default:
        {
            break;
        }
        }

        return false;
    }


    void SampleComponentManager::RenderImGui(float deltaTime)
    {
        if (!m_isImGuiAvailable)
        {
            return;
        }

        ShowMenuBar();

        if (m_exitRequested)
        {
            return;
        }

        if (m_showResizeViewportDialog)
        {
            ShowResizeViewportDialog();
        }

        if (m_showFramerateHistogram)
        {
            ShowFramerateHistogram(deltaTime);
        }

        if (m_showFrameCaptureDialog)
        {
            ShowFrameCaptureDialog();
        }

        if (m_showImGuiMetrics)
        {
            ImGui::ShowMetricsWindow(&m_showImGuiMetrics);
        }

        if (m_showSampleHelper)
        {
            ShowSampleHelper();
        }

        if (m_showAbout)
        {
            ShowAboutWindow();
        }

        if (m_showPassTree)
        {
            ShowPassTreeWindow();
        }

        if (m_showFrameGraphVisualizer)
        {
            ShowFrameGraphVisualizerWindow();
        }

        if (m_showCullingDebugWindow)
        {
            AZ::RPI::Scene* rpiScene = AZ::RPI::RPISystemInterface::Get()->GetSceneByName(AZ::Name("RPI"));
            if (rpiScene)
            {
                AZ::Render::ImGuiDrawCullingDebug(m_showCullingDebugWindow, rpiScene);
            }
        }

        if (m_showCpuProfiler)
        {
            ShowCpuProfilerWindow();
        }

        if (m_showGpuProfiler)
        {
            ShowGpuProfilerWindow();
        }

        if (m_showFileIoProfiler)
        {
            ShowFileIoProfilerWindow();
        }

        if (m_showTransientAttachmentProfiler)
        {
            ShowTransientAttachmentProfilerWindow();
        }

        m_scriptManager->TickImGui();

    }

    void SampleComponentManager::ShowMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            // If imgui doesn't have enough room to render a menu, it will fall back to the safe area which
            // is typically 3 pixels. This causes the menu to overlap the menu bar, and makes it easy to
            // accidentally select the first item on that menu bar. By altering the safe area temporarily
            // while drawing the menu, this problem can be avoided.
            
            ImVec2 cachedSafeArea = ImGui::GetStyle().DisplaySafeAreaPadding;
            ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(cachedSafeArea.x, cachedSafeArea.y + 16.0f);

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Ctrl-Q"))
                {
                    RequestExit();
                }
                if (ImGui::MenuItem("Capture Frame...", "Ctrl-P"))
                {
                    m_showFrameCaptureDialog = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (Utils::SupportsResizeClientArea() && ImGui::MenuItem("Resize Viewport..."))
                {
                    m_showResizeViewportDialog = true;
                }

                if (Utils::SupportsToggleFullScreenOfDefaultWindow() && ImGui::MenuItem("Toggle Full Screen"))
                {
                    Utils::ToggleFullScreenOfDefaultWindow();
                }

                if (ImGui::MenuItem("Frame Time Histogram"))
                {
                    m_showFramerateHistogram = !m_showFramerateHistogram;
                }

                if (ImGui::MenuItem("ImGui Metrics"))
                {
                    m_showImGuiMetrics = !m_showImGuiMetrics;
                }

                if (ImGui::MenuItem("Sample Helper"))
                {
                    m_showSampleHelper = !m_showSampleHelper;
                }

                if (ImGui::MenuItem("Frame Graph Visualizer"))
                {
                    m_showFrameGraphVisualizer = !m_showFrameGraphVisualizer;
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Samples"))
            {
                for (auto&& [parentMenuName, samples] : m_groupedSamples)
                {
                    if (ImGui::BeginMenu(parentMenuName.c_str()))
                    {
                        for (int32_t index : samples)
                        {
                            SampleEntry& sample = m_availableSamples[index];
                            const char* sampleName = sample.m_sampleName.c_str();
                            bool enabled = m_isSampleSupported[index];
                            if (index < s_alphanumericCount)
                            {
                                const AZStd::string hotkeyName = AZStd::string::format("Ctrl-%d: ", (index + 1) % 10);

                                if (ImGui::MenuItem(sampleName, hotkeyName.c_str(), false, enabled))
                                {
                                    m_selectedSampleIndex = index;
                                    m_sampleChangeRequest = true;
                                }
                            }
                            else
                            {
                                if (ImGui::MenuItem(sampleName, nullptr, false, enabled))
                                {
                                    m_selectedSampleIndex = index;
                                    m_sampleChangeRequest = true;
                                }
                            }
                        }

                        ImGui::EndMenu();
                    }
                }

                if (m_sampleChangeRequest)
                {
                    Utils::ReportScriptableAction("OpenSample('%s')", m_availableSamples[m_selectedSampleIndex].m_sampleName.c_str());
                }

                ImGui::EndMenu();
            }
#ifdef AZ_PROFILE_TELEMETRY
            if (ImGui::BeginMenu("RADTelemetry"))
            {
                if (ImGui::MenuItem("Toggle Capture", "Ctrl-T"))
                {
                    Utils::ToggleRadTMCapture();
                }
                ImGui::EndMenu();
            }
#endif // AZ_PROFILE_TELEMETRY

            if (ImGui::BeginMenu("Automation"))
            {
                if (ImGui::MenuItem("Run Script..."))
                {
                    m_scriptManager->OpenScriptRunnerDialog();
                }
                if (ImGui::MenuItem("Run Precommit Wizard..."))
                {
                    m_scriptManager->OpenPrecommitWizard();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Pass"))
            {
                if (ImGui::MenuItem(PassTreeToolName))
                {
                    m_showPassTree = !m_showPassTree;
                    Utils::ReportScriptableAction("ShowTool('%s', %s)", PassTreeToolName, m_showPassTree?"true":"false");
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Culling"))
            {
                if (ImGui::MenuItem("Culling Debug Window"))
                {
                    m_showCullingDebugWindow = !m_showCullingDebugWindow;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Profile"))
            {
                if (ImGui::MenuItem(CpuProfilerToolName))
                {
                    m_showCpuProfiler = !m_showCpuProfiler;
                    if (auto profilerSystem = AZ::Debug::ProfilerSystemInterface::Get(); profilerSystem)
                    {
                        profilerSystem->SetActive(m_showCpuProfiler);
                    }

                    Utils::ReportScriptableAction("ShowTool('%s', %s)", CpuProfilerToolName, m_showCpuProfiler ? "true" : "false");
                }

                if (AZ::IO::StreamerProfiler::Get() != nullptr)
                {
                    if (ImGui::MenuItem(FileIoProfilerToolName))
                    {
                        m_showFileIoProfiler = !m_showFileIoProfiler;
                        Utils::ReportScriptableAction(
                            "ShowTool('%s', %s)", FileIoProfilerToolName, m_showFileIoProfiler ? "true" : "false");
                    }
                }

                if (ImGui::MenuItem(GpuProfilerToolName))
                {
                    m_showGpuProfiler = !m_showGpuProfiler;

                    Utils::ReportScriptableAction("ShowTool('%s', %s)", GpuProfilerToolName, m_showGpuProfiler ? "true" : "false");
                }

                if (ImGui::MenuItem(TransientAttachmentProfilerToolName))
                {
                    m_showTransientAttachmentProfiler = !m_showTransientAttachmentProfiler;
                    AZ::RHI::RHISystemInterface::Get()->ModifyFrameSchedulerStatisticsFlags(
                        AZ::RHI::FrameSchedulerStatisticsFlags::GatherTransientAttachmentStatistics,
                        m_showTransientAttachmentProfiler);

                    Utils::ReportScriptableAction("ShowTool('%s', %s)", TransientAttachmentProfilerToolName, m_showTransientAttachmentProfiler ? "true" : "false");
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    m_showAbout = !m_showAbout;
                }
                ImGui::EndMenu();
            }

            // Restore original safe area.
            ImGui::GetStyle().DisplaySafeAreaPadding = cachedSafeArea;

            ImGui::EndMainMenuBar();
        }
    }

    void SampleComponentManager::ShowSampleHelper()
    {
        if (ImGui::Begin("Sample Helper", &m_showSampleHelper, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::Button("Reset"))
            {
                //Removes the existing sample component and 
                //resets the selection index
                Reset();
                CameraReset();
                m_selectedSampleIndex = -1;
            }
            ImGui::SameLine();

            if (ImGui::Button("Reset Sample"))
            {
                //Force a sample change event when the selection index
                //hasn't changed. This resets the sample component.
                SampleChange();
            }
            ImGui::SameLine();

            if (ImGui::Button("Reset Camera"))
            {
                CameraReset();
            }
        }
        ImGui::End();
    }

    void SampleComponentManager::ShowAboutWindow()
    {
        if (ImGui::Begin("About", &m_showAbout, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::Text("RHI API: %s", AZ::RHI::Factory::Get().GetName().GetCStr());
        }
        ImGui::End();
    }

    void SampleComponentManager::ShowPassTreeWindow()
    {
        m_imguiPassTree.Draw(m_showPassTree, AZ::RPI::PassSystemInterface::Get()->GetRootPass().get());
    }

    void SampleComponentManager::ShowFrameGraphVisualizerWindow()
    {
        AZ::RHI::Device* rhiDevice = Utils::GetRHIDevice().get();
        m_imguiFrameGraphVisualizer.Init(rhiDevice);
        m_imguiFrameGraphVisualizer.Draw(m_showFrameGraphVisualizer);
    }

    void SampleComponentManager::ShowCpuProfilerWindow()
    {
        if (auto profilerImGui = Profiler::ProfilerImGuiInterface::Get(); profilerImGui)
        {
            profilerImGui->ShowCpuProfilerWindow(m_showCpuProfiler);
        }
    }

    void SampleComponentManager::ShowFileIoProfilerWindow()
    {
        if (auto profilerImGui = AZ::IO::StreamerProfiler::Get(); profilerImGui)
        {
            profilerImGui->DrawStatistics(m_showFileIoProfiler);
        }
    }

    void SampleComponentManager::ShowGpuProfilerWindow()
    {
        m_imguiGpuProfiler.Draw(m_showGpuProfiler, AZ::RPI::PassSystemInterface::Get()->GetRootPass());
    }

    void SampleComponentManager::ShowTransientAttachmentProfilerWindow()
    {
        auto* transientStats = AZ::RHI::RHISystemInterface::Get()->GetTransientAttachmentStatistics();
        if (transientStats)
        {
            m_showTransientAttachmentProfiler = m_imguiTransientAttachmentProfiler.Draw(*transientStats);
        }
    }

    void SampleComponentManager::ShowResizeViewportDialog()
    {
        static int size[2] = { 0, 0 };

        if (size[0] <= 0)
        {
            size[0] = aznumeric_cast<int>(m_windowContext->GetViewport().m_maxX - m_windowContext->GetViewport().m_minX);
        }
        if (size[1] <= 0)
        {
            size[1] = aznumeric_cast<int>(m_windowContext->GetViewport().m_maxY - m_windowContext->GetViewport().m_minY);
        }

        bool dialogWasOpen = m_showResizeViewportDialog;

        if (ImGui::Begin("Resize Viewport", &m_showResizeViewportDialog, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::InputInt2("Size", size);

            if (ImGui::Button("Resize"))
            {
                Utils::ResizeClientArea(size[0], size[1]);

                Utils::ReportScriptableAction("ResizeViewport(%d, %d)", size[0], size[1]);

                // Re-initialize the size fields on the next frame so we can see whether the
                // correct size was achieved (should be the same values the user entered)...
                size[0] = 0;
                size[1] = 0;
            }
        }
        ImGui::End();

        if (dialogWasOpen && !m_showResizeViewportDialog)
        {
            // Re-initialize the size fields next time the dialog is shown...
            size[0] = 0;
            size[1] = 0;
        }
    }

    void SampleComponentManager::ShowFramerateHistogram(float deltaTime)
    {
        if (ImGui::Begin("Frame Time Histogram", &m_showFramerateHistogram, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGuiHistogramQueue::WidgetSettings settings;
            settings.m_reportInverse = false;
            settings.m_units = "ms";
            m_imGuiFrameTimer.Tick(deltaTime * 1000.0f, settings);
        }
        ImGui::End();
    }


    void SampleComponentManager::RequestFrameCapture(const AZStd::string& filePath, bool hideImGui)
    {
        AZ_Assert(false == m_isFrameCapturePending, "Frame capture already in progress");
        m_isFrameCapturePending = true;
        m_hideImGuiDuringFrameCapture = hideImGui;
        m_frameCaptureFilePath = filePath;

        // Don't continue the script while a frame capture is pending in case subsequent changes
        // interfere with the pending capture.
        ScriptRunnerRequestBus::Broadcast(&ScriptRunnerRequests::PauseScript);

        if (m_hideImGuiDuringFrameCapture)
        {
            AZ::Render::ImGuiSystemRequestBus::Broadcast(&AZ::Render::ImGuiSystemRequestBus::Events::HideAllImGuiPasses);

            // We also hide Open 3D Engine's debug text
            AzFramework::ConsoleRequestBus::Broadcast(&AzFramework::ConsoleRequests::ExecuteConsoleCommand, "r_DisplayInfo 0");
            // The ExecuteConsoleCommand request is handled in a deferred manner, so we have to delay the screenshot a bit.
            m_countdownForFrameCapture = 1;
        }
        else
        {
            m_countdownForFrameCapture = 0;
        }
    }

    void SampleComponentManager::OnCaptureFinished(AZ::Render::FrameCaptureResult /*result*/, const AZStd::string& /*info*/)
    {
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();

        if (m_hideImGuiDuringFrameCapture)
        {
            AZ::Render::ImGuiSystemRequestBus::Broadcast(&AZ::Render::ImGuiSystemRequestBus::Events::ShowAllImGuiPasses);

            // We also show Open 3D Engine's debug text
            AzFramework::ConsoleRequestBus::Broadcast(&AzFramework::ConsoleRequests::ExecuteConsoleCommand, "r_DisplayInfo 1");
        }

        ScriptRunnerRequestBus::Broadcast(&ScriptRunnerRequests::ResumeScript);
        m_isFrameCapturePending = false;
    }

    bool SampleComponentManager::IsFrameCapturePending()
    {
        return m_isFrameCapturePending;
    }

    void SampleComponentManager::RunMainTestSuite(const AZStd::string& suiteFilePath, bool exitOnTestEnd, int randomSeed)
    {
        if (m_scriptManager)
        {
            m_scriptManager->RunMainTestSuite(suiteFilePath, exitOnTestEnd, randomSeed);
        }
    }

    void SampleComponentManager::SetNumMSAASamples(int numMSAASamples)
    {
        AZ_Assert(IsValidNumMSAASamples(numMSAASamples), "Invalid MSAA sample setting");

        m_numMSAASamples = numMSAASamples;
    }

    void SampleComponentManager::ResetNumMSAASamples()
    {
        m_numMSAASamples = GetDefaultNumMSAASamples();
    }

    void SampleComponentManager::ResetRPIScene()
    {
        ReleaseRPIScene();
        SwitchSceneForRPISample();
    }

    void SampleComponentManager::ClearRPIScene()
    {
        ReleaseRPIScene();
    }

    void SampleComponentManager::ShowFrameCaptureDialog()
    {
        static bool requestCaptureOnNextFrame = false;
        static bool hideImGuiFromFrameCapture = true;

        if (requestCaptureOnNextFrame)
        {
            requestCaptureOnNextFrame = false;
            RequestFrameCapture(m_imguiFrameCaptureSaver.GetSaveFilePath(), hideImGuiFromFrameCapture);
        }
        else if (!m_isFrameCapturePending) // Hide this dialog while taking a capture
        {
            if (ImGui::Begin("Frame Capture", &m_showFrameCaptureDialog))
            {
                ImGui::Checkbox("Hide ImGui", &hideImGuiFromFrameCapture);

                ImGuiSaveFilePath::WidgetSettings settings;
                settings.m_labels.m_filePath = "File Path (.png, .ppm, or .dds):";
                m_imguiFrameCaptureSaver.Tick(settings);

                if (ImGui::Button("Capture"))
                {
                    // Don't actually do the capture until the next frame, so we can hide this dialog first
                    requestCaptureOnNextFrame = true;

                    if (hideImGuiFromFrameCapture)
                    {
                        Utils::ReportScriptableAction("CaptureScreenshot('%s')", m_imguiFrameCaptureSaver.GetSaveFilePath().c_str());
                    }
                    else
                    {
                        Utils::ReportScriptableAction("CaptureScreenshotWithImGui('%s')", m_imguiFrameCaptureSaver.GetSaveFilePath().c_str());
                    }
                }
            }
            ImGui::End();
        }
    }

    void SampleComponentManager::RequestExit()
    {
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);

        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();

        m_exitRequested = true;
    }

    void SampleComponentManager::ShutdownActiveSample()
    {
        m_exampleEntity->Deactivate();

        // Pointer to the m_activeSample must be nullified before m_activeSample is destroyed.
        if (m_rhiSamplePass)
        {
            m_rhiSamplePass->SetRHISample(nullptr);
        }

        if (m_activeSample != nullptr)
        {
            // Disable the camera controller just in case the active sample enabled it and didn't disable in Deactivate().
            AZ::Debug::CameraControllerRequestBus::Event(m_cameraEntity->GetId(), &AZ::Debug::CameraControllerRequestBus::Events::Disable);

            m_exampleEntity->RemoveComponent(m_activeSample);
            delete m_activeSample;
        }

        m_activeSample = nullptr;

        // Force a reset of the shader variant finder to get more consistent testing of samples every time they are run, rather
        // than the first time for each sample being "special".
        auto variantFinder = AZ::Interface<AZ::RPI::IShaderVariantFinder>::Get();
        variantFinder->Reset();
    }

    void SampleComponentManager::Reset()
    {
        ShutdownActiveSample();

        m_exampleEntity->Activate();

        // Reset to RHI sample pipeline
        SwitchSceneForRHISample();
        m_rhiSamplePass->SetRHISample(nullptr);
    }

    void SampleComponentManager::CreateDefaultCamera()
    {
        if (m_cameraEntity)
        {
            return;
        }

        // A camera entity will be created by the entity context request bus so that the component for this entity can use a feature processor.
        AzFramework::EntityContextRequestBus::EventResult(m_cameraEntity, m_entityContextId, &AzFramework::EntityContextRequestBus::Events::CreateEntity, "CameraEntity");

        //Add debug camera and controller components
        AZ::Debug::CameraComponentConfig cameraConfig(m_windowContext);
        cameraConfig.m_fovY = AZ::Constants::QuarterPi;
        cameraConfig.m_depthFar = 1000.0f;

        m_cameraEntity->CreateComponent(azrtti_typeid<AZ::Debug::CameraComponent>())
            ->SetConfiguration(cameraConfig);
        m_cameraEntity->CreateComponent(azrtti_typeid<AzFramework::TransformComponent>());
        m_cameraEntity->CreateComponent(azrtti_typeid<AZ::Debug::ArcBallControllerComponent>());
        m_cameraEntity->CreateComponent(azrtti_typeid<AZ::Debug::NoClipControllerComponent>());

        m_cameraEntity->Activate();

        m_scriptManager->SetCameraEntity(m_cameraEntity);
    }

    void SampleComponentManager::SetupImGuiContext()
    {
        AdjustImGuiFontScale();

        // Add imgui context
        Render::ImGuiSystemRequestBus::BroadcastResult(m_isImGuiAvailable, &Render::ImGuiSystemRequests::PushActiveContextFromDefaultPass);
        AZ_Assert(m_isImGuiAvailable, "Unable set default imgui context to active. Does your pipeline have an ImGui pass marked as default? Your pass assets may need to be rebuilt.");
    }

    void SampleComponentManager::ActiveImGuiContextChanged(ImGuiContext* context)
    {
        ImGui::SetCurrentContext(context);
    }

    bool SampleComponentManager::OpenSample(const AZStd::string& sampleName)
    {
        for (int32_t i = 0; i < m_availableSamples.size(); i++)
        {
            if (m_availableSamples[i].m_parentMenuName + '/' + m_availableSamples[i].m_sampleName == sampleName)
            {
                if (!m_availableSamples[i].m_isSupportedFunc || m_availableSamples[i].m_isSupportedFunc())
                {
                    m_selectedSampleIndex = i;
                    m_sampleChangeRequest = true;

                    return true;
                }
                else
                {
                    AZ_Error("SampleComponentManager", false, "Sample '%s' is not supported on this platform.", sampleName.c_str());
                }
            }
        }

        return false;
    }

    bool SampleComponentManager::ShowTool(const AZStd::string& toolName, bool enable)
    {
        if (toolName == PassTreeToolName)
        {
            m_showPassTree = enable;
            return true;
        }
        else if (toolName == CpuProfilerToolName)
        {
            m_showCpuProfiler = enable;
            return true;
        }
        else if (toolName == FileIoProfilerToolName)
        {
            m_showFileIoProfiler = enable;
            return true;
        }
        else if (toolName == GpuProfilerToolName)
        {
            m_showGpuProfiler = enable;
            return true;
        }
        else if (toolName == TransientAttachmentProfilerToolName)
        {
            m_showTransientAttachmentProfiler = enable;
            return true;
        }
        return false;
    }

    void SampleComponentManager::SampleChange()
    {
        if (m_selectedSampleIndex == -1)
        {
            return;
        }

        ShutdownActiveSample();

        // Reset the camera *before* activating the sample, because the sample's Activate() function might
        // want to reposition the camera.
        CameraReset();

        const SampleEntry& sampleEntry = m_availableSamples[m_selectedSampleIndex];

        // Create scene and render pipeline before create sample component
        if (sampleEntry.m_pipelineType == SamplePipelineType::RHI)
        {
            SwitchSceneForRHISample();
        }
        else if (sampleEntry.m_pipelineType == SamplePipelineType::RPI)
        {
            SwitchSceneForRPISample();
        }

        SampleComponentConfig config(m_windowContext, m_cameraEntity->GetId(), m_entityContextId);
        m_activeSample = m_exampleEntity->CreateComponent(sampleEntry.m_sampleUuid);
        m_activeSample->SetConfiguration(config);

        // special setup for RHI samples
        if (sampleEntry.m_pipelineType == SamplePipelineType::RHI)
        {
            BasicRHIComponent* rhiSampleComponent = static_cast<BasicRHIComponent*>(m_activeSample);
            if (rhiSampleComponent->IsSupportedRHISamplePipeline())
            {
                m_rhiSamplePass->SetRHISample(rhiSampleComponent);
            }
            else
            {
                m_rhiSamplePass->SetRHISample(nullptr);
            }
        }

        m_exampleEntity->Activate();

        // Even though this is done in CameraReset(), the example component wasn't activated at the time so we have to send this event again.
        ExampleComponentRequestBus::Event(m_exampleEntity->GetId(), &ExampleComponentRequestBus::Events::ResetCamera);
    }

    void SampleComponentManager::CameraReset()
    {
        // Reset the camera transform. Some examples do not use a controller or use a controller that doesn't override the whole transform.
        // Set to a transform that is 5 units away from the origin and looking at the origin along the Y axis.
        const AZ::EntityId cameraEntityId = m_cameraEntity->GetId();
        AZ::TransformBus::Event(cameraEntityId, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, -5.0f, 0.0f)));
        AZ::Debug::CameraControllerRequestBus::Event(cameraEntityId, &AZ::Debug::CameraControllerRequestBus::Events::Reset);

        // Tell the current example to reset the camera, any example that controls the camera and preserves controller state should implement this event
        ExampleComponentRequestBus::Event(m_exampleEntity->GetId(), &ExampleComponentRequestBus::Events::ResetCamera);
    }

    void SampleComponentManager::CreateSceneForRHISample()
    {
        // Create and register the rhi scene with only feature processors required for AtomShimRenderer (only for AtomSampleViewerLauncher)
        RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name("RHI");
        sceneDesc.m_featureProcessorNames.push_back("AuxGeomFeatureProcessor");
        m_rhiScene = RPI::Scene::CreateScene(sceneDesc);
        m_rhiScene->Activate();

        RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_name = "RHISamplePipeline";
        pipelineDesc.m_rootPassTemplate = "RHISamplePipelineTemplate";
        // Add view to pipeline since there are few RHI samples are using ViewSrg
        pipelineDesc.m_mainViewTagName = "MainCamera";

        RPI::RenderPipelinePtr renderPipeline = RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineDesc, *m_windowContext.get());
        m_rhiScene->AddRenderPipeline(renderPipeline);
        renderPipeline->SetDefaultViewFromEntity(m_cameraEntity->GetId());

        RPI::RPISystemInterface::Get()->RegisterScene(m_rhiScene);

        // Get RHISamplePass
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("RHISamplePass"), renderPipeline.get());
        m_rhiSamplePass = azrtti_cast<RHISamplePass*>(AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter));

        // Setup imGui since a new render pipeline with imgui pass was created
        SetupImGuiContext();
    }

    void SampleComponentManager::ReleaseRHIScene()
    {
        if (m_rhiScene)
        {
            m_rhiSamplePass = nullptr;
            RPI::RPISystemInterface::Get()->UnregisterScene(m_rhiScene);
            m_rhiScene = nullptr;
        }
    }

    void SampleComponentManager::SwitchSceneForRHISample()
    {
        ReleaseRPIScene();
        if (!m_rhiScene)
        {
            CreateSceneForRHISample();
        }
    }

    void SampleComponentManager::CreateSceneForRPISample()
    {
        // Create and register a scene with all available feature processors
        RPI::SceneDescriptor sceneDesc;
        sceneDesc.m_nameId = AZ::Name("RPI");
        m_rpiScene = RPI::Scene::CreateScene(sceneDesc);
        m_rpiScene->EnableAllFeatureProcessors();

        // Bind m_rpiScene to the GameEntityContext's AzFramework::Scene so the RPI Scene can be found by the entity context
        auto sceneSystem = AzFramework::SceneSystemInterface::Get();
        AZ_Assert(sceneSystem, "SampleComponentManager requires an implementation of the scene system.");
        AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
        AZ_Assert(mainScene, "Main scene missing during system component initialization"); // This should never happen unless scene creation has changed.
        // Add RPI::Scene as a sub system for the default AzFramework Scene
        [[maybe_unused]] bool result = mainScene->SetSubsystem(m_rpiScene);
        AZ_Assert(result, "SampleComponentManager failed to register the RPI scene with the general scene.");

        m_rpiScene->Activate();

        // Register scene to RPI system so it will be processed/rendered per tick
        RPI::RPISystemInterface::Get()->RegisterScene(m_rpiScene);

        // Create MainPipeline as its render pipeline
        RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_name = "RPISamplePipeline";
        pipelineDesc.m_rootPassTemplate = GetRootPassTemplateName();
        pipelineDesc.m_mainViewTagName = "MainCamera";
        pipelineDesc.m_allowModification = true;

        // set pipeline MSAA samples
        AZ_Assert(IsValidNumMSAASamples(m_numMSAASamples), "Invalid MSAA sample setting");
        pipelineDesc.m_renderSettings.m_multisampleState.m_samples = static_cast<uint16_t>(m_numMSAASamples);
        bool isNonMsaaPipeline = (pipelineDesc.m_renderSettings.m_multisampleState.m_samples == 1);
        const char* supervariantName = isNonMsaaPipeline ? AZ::RPI::NoMsaaSupervariantName : "";
        AZ::RPI::ShaderSystemInterface::Get()->SetSupervariantName(AZ::Name(supervariantName));

        RPI::RenderPipelinePtr renderPipeline = RPI::RenderPipeline::CreateRenderPipelineForWindow(pipelineDesc, *m_windowContext.get());
        m_rpiScene->AddRenderPipeline(renderPipeline);

        renderPipeline->SetDefaultViewFromEntity(m_cameraEntity->GetId());

        // As part of our initialization we need to create the BRDF texture generation pipeline
        AZ::RPI::RenderPipelineDescriptor brdfPipelineDesc;
        brdfPipelineDesc.m_mainViewTagName = "MainCamera";
        brdfPipelineDesc.m_name = "BRDFTexturePipeline";
        brdfPipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
        brdfPipelineDesc.m_executeOnce = true;
        
        RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(brdfPipelineDesc);
        m_rpiScene->AddRenderPipeline(brdfTexturePipeline);
        
        // Save a reference to the generated BRDF texture so it doesn't get deleted if all the passes refering to it get deleted and it's ref count goes to zero
        if (!m_brdfTexture)
        {
            const AZStd::shared_ptr<const RPI::PassTemplate> brdfTextureTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("BRDFTextureTemplate"));
            Data::Asset<RPI::AttachmentImageAsset> brdfImageAsset = RPI::AssetUtils::LoadAssetById<RPI::AttachmentImageAsset>(
                brdfTextureTemplate->m_imageAttachments[0].m_assetRef.m_assetId, RPI::AssetUtils::TraceLevel::Error);
            if (brdfImageAsset.IsReady())
            {
                m_brdfTexture = RPI::AttachmentImage::FindOrCreate(brdfImageAsset);
            }
        }

        // Setup imGui since a new render pipeline with imgui pass was created
        SetupImGuiContext();
    }

    void SampleComponentManager::ReleaseRPIScene()
    {
        if (m_rpiScene)
        {
            RPI::RPISystemInterface::Get()->UnregisterScene(m_rpiScene);

            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            AZ_Assert(sceneSystem, "Scene system was destroyed before SampleComponentManager was able to unregister the RPI scene.");
            AZStd::shared_ptr<AzFramework::Scene> scene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
            AZ_Assert(scene, "The main scene wasn't found in the scene system.");
            [[maybe_unused]] bool result = scene->UnsetSubsystem(m_rpiScene);
            AZ_Assert(result, "SampleComponentManager failed to unregister its RPI scene from the general scene.");
            
            m_rpiScene = nullptr;
        }
    }

    void SampleComponentManager::SwitchSceneForRPISample()
    {
        ReleaseRHIScene();
        if (!m_rpiScene)
        {
            CreateSceneForRPISample();
        }
    }

    // AzFramework::AssetCatalogEventBus::Handler overrides ...
    void SampleComponentManager::OnCatalogLoaded([[maybe_unused]] const char* catalogFile)
    {
        AZ::TickBus::QueueFunction([&]()
            {
                ActivateInternal();
            });
    }

} // namespace AtomSampleViewer
