/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BootstrapSystemComponent.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <ISystem.h>

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderSystem.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Console/IConsole.h>
#include <BootstrapSystemComponent_Traits_Platform.h>

void cvar_r_renderPipelinePath_Changed(const AZ::CVarFixedString& newPipelinePath)
{
    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!viewportContextManager)
    {
        return;
    }
    auto viewportContext = viewportContextManager->GetDefaultViewportContext();
    if (!viewportContext)
    {
        return;
    }

    AZ::Data::Asset<AZ::RPI::AnyAsset> pipelineAsset =
        AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(newPipelinePath.data(), AZ::RPI::AssetUtils::TraceLevel::Error);
    if (pipelineAsset)
    {
        AZ::RPI::RenderPipelineDescriptor renderPipelineDescriptor =
            *AZ::RPI::GetDataFromAnyAsset<AZ::RPI::RenderPipelineDescriptor>(pipelineAsset); // Copy descriptor from asset

        AZ::Render::Bootstrap::RequestBus::Broadcast(&AZ::Render::Bootstrap::RequestBus::Events::SwitchRenderPipeline, renderPipelineDescriptor, viewportContext);
    }
    else
    {
        AZ_Warning("SetDefaultPipeline", false, "Failed to switch default render pipeline to %s: can't load the asset", newPipelinePath.data());
    }
}

void cvar_r_antiAliasing_Changed(const AZ::CVarFixedString& newAntiAliasing)
{
    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!viewportContextManager)
    {
        return;
    }
    auto viewportContext = viewportContextManager->GetDefaultViewportContext();
    if (!viewportContext)
    {
        return;
    }

    AZ::Render::Bootstrap::RequestBus::Broadcast(&AZ::Render::Bootstrap::RequestBus::Events::SwitchAntiAliasing, newAntiAliasing.c_str(), viewportContext);
}

void cvar_r_multiSample_Changed(const uint16_t& newSampleCount)
{
    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    if (!viewportContextManager)
    {
        return;
    }
    auto viewportContext = viewportContextManager->GetDefaultViewportContext();
    if (!viewportContext)
    {
        return;
    }
    if (newSampleCount > 0 && ((newSampleCount & (newSampleCount - 1))) == 0)
    {
        AZ::Render::Bootstrap::RequestBus::Broadcast(&AZ::Render::Bootstrap::RequestBus::Events::SwitchMultiSample, AZStd::clamp(newSampleCount, uint16_t(1), uint16_t(8)), viewportContext);
    }
    else
    {
        AZ_Warning("SetMultiSampleCount", false, "Failed to set multi-sample count %d: invalid multi-sample count", newSampleCount);
    }
}


void cvar_r_resolution_Changed([[maybe_unused]] const uint32_t& newValue)
{
    AZ::Render::Bootstrap::RequestBus::Broadcast(&AZ::Render::Bootstrap::RequestBus::Events::RefreshWindowResolution);
}

void cvar_r_renderScale_Changed(const float& newRenderScale)
{
    AZ_Error("AtomBootstrap", newRenderScale > 0, "RenderScale should be greater than 0");
    if (newRenderScale > 0)
    {
        AZ::Render::Bootstrap::RequestBus::Broadcast(&AZ::Render::Bootstrap::RequestBus::Events::RefreshWindowResolution);
    }
}

AZ_CVAR(AZ::CVarFixedString, r_renderPipelinePath, AZ_TRAIT_BOOTSTRAPSYSTEMCOMPONENT_PIPELINE_NAME, cvar_r_renderPipelinePath_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "The asset (.azasset) path for default render pipeline");
AZ_CVAR(AZ::CVarFixedString, r_default_openxr_pipeline_name, "passes/MultiViewRenderPipeline.azasset", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default openXr render pipeline name");
AZ_CVAR(AZ::CVarFixedString, r_default_openxr_left_pipeline_name, "passes/XRLeftRenderPipeline.azasset", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default openXr Left eye render pipeline name");
AZ_CVAR(AZ::CVarFixedString, r_default_openxr_right_pipeline_name, "passes/XRRightRenderPipeline.azasset", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default openXr Right eye render pipeline name");
AZ_CVAR(uint32_t, r_width, 1920, cvar_r_resolution_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "Starting window width in pixels.");
AZ_CVAR(uint32_t, r_height, 1080, cvar_r_resolution_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "Starting window height in pixels.");
AZ_CVAR(uint32_t, r_fullscreen, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Starting fullscreen state.");
AZ_CVAR(uint32_t, r_resolutionMode, 0, cvar_r_resolution_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "0: render resolution same as window client area size, 1: render resolution use the values specified by r_width and r_height");
AZ_CVAR(float, r_renderScale, 1.0f, cvar_r_renderScale_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "Scale to apply to the window resolution.");
AZ_CVAR(AZ::CVarFixedString, r_antiAliasing, "", cvar_r_antiAliasing_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "The anti-aliasing to be used for the current render pipeline. Available options: MSAA, TAA, SMAA");
AZ_CVAR(uint16_t, r_multiSampleCount, 0, cvar_r_multiSample_Changed, AZ::ConsoleFunctorFlags::DontReplicate, "The multi-sample count to be used for the current render pipeline."); // 0 stands for unchanged, load the default setting from the pipeline itself

namespace AZ
{
    namespace Render
    {
        namespace Bootstrap
        {
            void BootstrapSystemComponent::Reflect(ReflectContext* context)
            {
                if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
                {
                    serialize->Class<BootstrapSystemComponent, Component>()
                        ->Version(1)
                    ;

                    if (EditContext* ec = serialize->GetEditContext())
                    {
                        ec->Class<BootstrapSystemComponent>("Atom RPI", "Atom Renderer")
                            ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
                    }
                }
            }

            void BootstrapSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("BootstrapSystemComponent"));
            }

            void BootstrapSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("RPISystem"));
                required.push_back(AZ_CRC_CE("SceneSystemComponentService"));
            }

            void BootstrapSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC_CE("ImGuiSystemComponent"));
                dependent.push_back(AZ_CRC_CE("PrimitiveSystemComponent"));
                dependent.push_back(AZ_CRC_CE("MeshSystemComponent"));
                dependent.push_back(AZ_CRC_CE("CoreLightsService"));
                dependent.push_back(AZ_CRC_CE("DynamicDrawService"));
                dependent.push_back(AZ_CRC_CE("CommonService"));
                dependent.push_back(AZ_CRC_CE("HairService"));
            }

            void BootstrapSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("BootstrapSystemComponent"));
            }

            BootstrapSystemComponent::BootstrapSystemComponent()
            {
            }

            BootstrapSystemComponent::~BootstrapSystemComponent()
            {
                m_viewportContext.reset();
            }

            //! Helper function that parses the command line arguments
            //! looking for r_width, r_height and r_fullscreen.
            //! It is important to call this before using r_width, r_height or r_fullscreen
            //! because at the moment this system component initializes before Legacy System.cpp gets to parse
            //! command line arguments into cvars.
            static void UpdateCVarsFromCommandLine()
            {
                AZ::CommandLine* pCmdLine = nullptr;
                ComponentApplicationBus::BroadcastResult(pCmdLine, &AZ::ComponentApplicationBus::Events::GetAzCommandLine);
                if (!pCmdLine)
                {
                    return;
                }

                const AZStd::string fullscreenCvarName("r_fullscreen");
                if (pCmdLine->HasSwitch(fullscreenCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(fullscreenCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(fullscreenCvarName);
                        if (AZ::StringFunc::LooksLikeBool(valueStr.c_str()))
                        {
                            r_fullscreen = AZ::StringFunc::ToBool(valueStr.c_str());
                        }
                    }
                }

                const AZStd::string widthCvarName("r_width");
                if (pCmdLine->HasSwitch(widthCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(widthCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(widthCvarName);
                        if (AZ::StringFunc::LooksLikeInt(valueStr.c_str()))
                        {
                            auto width = AZ::StringFunc::ToInt(valueStr.c_str());
                            if (width > 0)
                            {
                                r_width = width;
                            }
                        }
                    }
                }

                const AZStd::string heightCvarName("r_height");
                if (pCmdLine->HasSwitch(heightCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(heightCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(heightCvarName);
                        if (AZ::StringFunc::LooksLikeInt(valueStr.c_str()))
                        {
                            auto height = AZ::StringFunc::ToInt(valueStr.c_str());
                            if (height > 0)
                            {
                                r_height = height;
                            }
                        }
                    }
                }

                const AZStd::string resolutionModeCvarName("r_resolutionMode");
                if (pCmdLine->HasSwitch(resolutionModeCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(resolutionModeCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(resolutionModeCvarName);
                        if (AZ::StringFunc::LooksLikeInt(valueStr.c_str()))
                        {
                            auto resolutionMode = AZ::StringFunc::ToInt(valueStr.c_str());
                            if (resolutionMode >= 0)
                            {
                                r_resolutionMode = resolutionMode;
                            }
                        }
                    }
                }

                const AZStd::string renderScaleCvarName("r_renderScale");
                if (pCmdLine->HasSwitch(renderScaleCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(renderScaleCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(renderScaleCvarName);
                        if (AZ::StringFunc::LooksLikeFloat(valueStr.c_str()))
                        {
                            auto renderScale = AZ::StringFunc::ToFloat(valueStr.c_str());
                            if (renderScale > 0)
                            {
                                r_renderScale = renderScale;
                            }
                        }
                    }
                }

                const AZStd::string multiSampleCvarName("r_multiSampleCount");
                if (pCmdLine->HasSwitch(multiSampleCvarName))
                {
                    auto numValues = pCmdLine->GetNumSwitchValues(multiSampleCvarName);
                    if (numValues > 0)
                    {
                        auto valueStr = pCmdLine->GetSwitchValue(multiSampleCvarName);
                        if (AZ::StringFunc::LooksLikeInt(valueStr.c_str()))
                        {
                            auto multiSample = AZ::StringFunc::ToInt(valueStr.c_str());
                            if (multiSample > 0)
                            {
                                r_multiSampleCount = static_cast<uint16_t>(multiSample);
                            }
                        }
                    }
                }
            }

            void BootstrapSystemComponent::Activate()
            {
                // Create a native window only if it's a launcher (or standalone)
                // LY editor create its own window which we can get its handle through AzFramework::WindowSystemNotificationBus::Handler's OnWindowCreated() function

                // Query the application type to determine if this is a headless application
                AZ::ApplicationTypeQuery appType;
                ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);

                if (appType.IsHeadless())
                {
                    m_nativeWindow = nullptr;
                }
                else if (appType.IsConsoleMode())
                {
                    m_nativeWindow = nullptr;

                    // If we are running without a native window, the application multisamplestate still needs to be set and
                    // initialized so that the shader's SuperVariant name is set and the scene's render pipelines are re-initialized
                    AZ::RHI::MultisampleState multisampleState;
                    AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(multisampleState);
                }
                else if (!appType.IsValid() || appType.IsGame())
                {
                    // GFX TODO - investigate window creation being part of the GameApplication.

                    auto projectTitle = AZ::Utils::GetProjectDisplayName();

                    // It is important to call this before using r_width, r_height or r_fullscreen
                    // because at the moment this system component initializes before Legacy System.cpp gets to parse
                    // command line arguments into cvars.
                    UpdateCVarsFromCommandLine();

                    m_nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(
                        projectTitle.c_str(), AzFramework::WindowGeometry(0, 0, r_width, r_height));
                    AZ_Assert(m_nativeWindow, "Failed to create the game window\n");

                    m_nativeWindow->Activate();

                    m_windowHandle = m_nativeWindow->GetWindowHandle();
                }
                else
                {
                    // Disable default scene creation for non-games projects
                    // This can be manually overridden via the DefaultWindowBus.
                    m_createDefaultScene = false;
                }

                TickBus::Handler::BusConnect();

                // Listen for window system requests (e.g. requests for default window handle)
                AzFramework::WindowSystemRequestBus::Handler::BusConnect();

                // Listen for window system notifications (e.g. window being created by Editor)
                AzFramework::WindowSystemNotificationBus::Handler::BusConnect();

                Render::Bootstrap::DefaultWindowBus::Handler::BusConnect();
                Render::Bootstrap::RequestBus::Handler::BusConnect();

                // Listen for application's window creation/destruction (e.g. window is created/destroyed on Android when suspending the app)
                AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();

                // delay one frame for Initialize which asset system is ready by then
                AZ::TickBus::QueueFunction(
                    [this]()
                    {
                        Initialize();
                        SetWindowResolution();
                    });
            }

            void BootstrapSystemComponent::Deactivate()
            {
                AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
                Render::Bootstrap::RequestBus::Handler::BusDisconnect();
                Render::Bootstrap::DefaultWindowBus::Handler::BusDisconnect();

                AzFramework::WindowSystemRequestBus::Handler::BusDisconnect();
                AzFramework::WindowSystemNotificationBus::Handler::BusDisconnect();
                TickBus::Handler::BusDisconnect();

                m_brdfTexture = nullptr;
                RemoveRenderPipeline();
                DestroyDefaultScene();

                m_viewportContext.reset();
                m_nativeWindow = nullptr;
                m_windowHandle = nullptr;
            }

            void BootstrapSystemComponent::Initialize()
            {
                if (m_isInitialized)
                {
                    return;
                }

                m_isInitialized = true;

                if (!RPI::RPISystemInterface::Get()->IsInitialized())
                {
                    AZ::OSString msgBoxMessage;
                    msgBoxMessage.append("RPI System could not initialize correctly. Check log for detail.");

                    AZ::NativeUI::NativeUIRequestBus::Broadcast(
                        &AZ::NativeUI::NativeUIRequestBus::Events::DisplayOkDialog, "O3DE Fatal Error", msgBoxMessage.c_str(), false);
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);

                    return;
                }

                // In the case of the game we want to call create and register the scene as a soon as we can
                // because a level could be loaded in autoexec.cfg and that will assert if there is no scene registered
                // to get the feature processors for the components. So we can't wait until the tick (whereas the Editor wants to wait)

                if (m_createDefaultScene)
                {
                    CreateDefaultScene();
                }

                if (m_windowHandle)
                {
                    CreateViewportContext();
                    if (m_createDefaultScene)
                    {
                        CreateDefaultRenderPipeline();
                    }
                }
                else
                {
                    AZ::ApplicationTypeQuery appType;
                    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                    // Run BRDF pipeline for the app in console mode, to use it in render-to-texture pipelines.
                    if (appType.IsConsoleMode())
                    {
                        RunBRDFPipeline(m_defaultScene, nullptr);
                    }
                }
            }

            void BootstrapSystemComponent::OnWindowCreated(AzFramework::NativeWindowHandle windowHandle)
            {
                // only handle the first window (default) created
                if (m_windowHandle == nullptr)
                {
                    m_windowHandle = windowHandle;

                    if (m_isInitialized)
                    {
                        CreateViewportContext();
                        if (m_createDefaultScene)
                        {
                            CreateDefaultRenderPipeline();
                        }
                    }
                    SetWindowResolution();
                }
            }

            void BootstrapSystemComponent::OnApplicationWindowCreated()
            {
                if (!m_nativeWindow)
                {
                    auto projectTitle = AZ::Utils::GetProjectDisplayName();
                    m_nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(projectTitle.c_str(), AzFramework::WindowGeometry(0, 0, r_width, r_height));
                    AZ_Assert(m_nativeWindow, "Failed to create the game window\n");

                    m_nativeWindow->Activate();

                    OnWindowCreated(m_nativeWindow->GetWindowHandle());
                }
            }

            void BootstrapSystemComponent::OnApplicationWindowDestroy()
            {
                m_nativeWindow = nullptr;
            }

            void BootstrapSystemComponent::CreateViewportContext()
            {
                RHI::Device* device = RHI::RHISystemInterface::Get()->GetDevice();
                RPI::ViewportContextRequestsInterface::CreationParameters params;
                params.device = device;
                params.windowHandle = m_windowHandle;
                params.renderScene = m_defaultScene;
                // Setting the default ViewportContextID to an arbitrary and otherwise invalid (negative) value to ensure its uniqueness
                params.id = -10;

                auto viewContextManager = AZ::Interface<RPI::ViewportContextRequestsInterface>::Get();
                m_viewportContext = viewContextManager->CreateViewportContext(
                    viewContextManager->GetDefaultViewportContextName(), params);

                DefaultWindowNotificationBus::Broadcast(&DefaultWindowNotificationBus::Events::DefaultWindowCreated);

                // Listen to window notification so we can request exit application when window closes
                AzFramework::WindowNotificationBus::Handler::BusConnect(GetDefaultWindowHandle());
            }

            void BootstrapSystemComponent::SetWindowResolution()
            {
                if (m_nativeWindow)
                {
                    // wait until swapchain has been created before setting fullscreen state
                    AzFramework::WindowSize resolution;
                    float scale = AZStd::max(static_cast<float>(r_renderScale), 0.f);
                    if (r_resolutionMode > 0u)
                    {
                        resolution.m_width = static_cast<uint32_t>(r_width * scale);
                        resolution.m_height = static_cast<uint32_t>(r_height * scale);
                    }
                    else
                    {
                        const auto& windowSize = m_nativeWindow->GetClientAreaSize();
                        resolution.m_width = static_cast<uint32_t>(windowSize.m_width * scale);
                        resolution.m_height = static_cast<uint32_t>(windowSize.m_height * scale);
                    }
                    m_nativeWindow->SetRenderResolution(resolution);
                    m_nativeWindow->SetFullScreenState(r_fullscreen);
                }
            }

            AZ::RPI::ScenePtr BootstrapSystemComponent::GetOrCreateAtomSceneFromAzScene(AzFramework::Scene* scene)
            {
                // Get or create a weak pointer to our scene
                // If it's valid, we're done, if not we need to create an Atom scene and update our scene map
                auto& atomSceneHandle = m_azSceneToAtomSceneMap[scene];
                if (!atomSceneHandle.expired())
                {
                    return atomSceneHandle.lock();
                }

                // Create and register a scene with all available feature processors
                RPI::SceneDescriptor sceneDesc;
                sceneDesc.m_nameId = AZ::Name("Main");
                AZ::RPI::ScenePtr atomScene = RPI::Scene::CreateScene(sceneDesc);
                atomScene->EnableAllFeatureProcessors();
                atomScene->Activate();

                // Register scene to RPI system so it will be processed/rendered per tick
                RPI::RPISystemInterface::Get()->RegisterScene(atomScene);
                scene->SetSubsystem(atomScene);

                atomSceneHandle = atomScene;

                return atomScene;
            }

            void BootstrapSystemComponent::CreateDefaultScene()
            {
                // Bind atomScene to the GameEntityContext's AzFramework::Scene
                m_defaultFrameworkScene = AzFramework::SceneSystemInterface::Get()->GetScene(AzFramework::Scene::MainSceneName);
                // This should never happen unless scene creation has changed.
                AZ_Assert(m_defaultFrameworkScene, "Error: Scenes missing during system component initialization");
                m_sceneRemovalHandler = AzFramework::Scene::RemovalEvent::Handler(
                    [this](AzFramework::Scene&, AzFramework::Scene::RemovalEventType eventType)
                    {
                        if (eventType == AzFramework::Scene::RemovalEventType::Zombified)
                        {
                            m_defaultFrameworkScene.reset();
                        }
                    });
                m_defaultFrameworkScene->ConnectToEvents(m_sceneRemovalHandler);
                m_defaultScene = GetOrCreateAtomSceneFromAzScene(m_defaultFrameworkScene.get());
            }

            bool BootstrapSystemComponent::EnsureDefaultRenderPipelineInstalledForScene(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext)
            {
                AZ::RPI::XRRenderingInterface* xrSystem = AZ::RPI::RPISystemInterface::Get()->GetXRSystem();
                const bool loadDefaultRenderPipeline = !xrSystem || xrSystem->GetRHIXRRenderingInterface()->IsDefaultRenderPipelineNeeded();

                AZ::RHI::MultisampleState multisampleState;

                // Load the main default pipeline if applicable
                if (loadDefaultRenderPipeline)
                {
                    AZ::CVarFixedString pipelineName = static_cast<AZ::CVarFixedString>(r_renderPipelinePath);
                    if (xrSystem)
                    {
                        // When running launcher on PC having an XR system present then the default render pipeline is suppose to reflect
                        // what's being rendered into XR device. XR render pipeline uses multiview render pipeline.
                        AZ::ApplicationTypeQuery appType;
                        ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                        if (appType.IsGame())
                        {
                            pipelineName = r_default_openxr_pipeline_name;
                        }
                    }

                    RPI::RenderPipelinePtr renderPipeline = LoadPipeline(scene, viewportContext, pipelineName, AZ::RPI::ViewType::Default, multisampleState);
                    if (!renderPipeline)
                    {
                        return false;
                    }

                    AZ::CVarFixedString antiAliasing = static_cast<AZ::CVarFixedString>(r_antiAliasing);
                    if (antiAliasing != "")
                    {
                        renderPipeline->SetActiveAAMethod(antiAliasing.c_str());
                    }
                }

                RunBRDFPipeline(scene, viewportContext);

                // Load XR pipelines if applicable
                if (xrSystem)
                {
                    for (AZ::u32 i = 0; i < xrSystem->GetNumViews(); i++)
                    {
                        const AZ::RPI::ViewType viewType = (i == 0)
                            ? AZ::RPI::ViewType::XrLeft
                            : AZ::RPI::ViewType::XrRight;
                        const AZStd::string_view xrPipelineAssetName = (viewType == AZ::RPI::ViewType::XrLeft)
                            ? static_cast<AZ::CVarFixedString>(r_default_openxr_left_pipeline_name)
                            : static_cast<AZ::CVarFixedString>(r_default_openxr_right_pipeline_name);

                        if (!LoadPipeline(scene, viewportContext, xrPipelineAssetName, viewType, multisampleState))
                        {
                            return false;
                        }
                    }
                }

                // Apply MSAA state to all the render pipelines.
                // It's important to do this after all the pipelines have
                // been created so the same values are applied to all.
                // As it cannot be applied MSAA values per pipeline,
                // it's setting the MSAA state from the last pipeline loaded.
                const auto cvarMultiSample = static_cast<uint16_t>(r_multiSampleCount);
                if (cvarMultiSample > 0 && ((cvarMultiSample & (cvarMultiSample - 1))) == 0)
                {
                    multisampleState.m_samples = static_cast<uint16_t>(cvarMultiSample);
                }
                AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(multisampleState);

                // Send notification when the scene and its pipeline are ready.
                // Use the first created pipeline's scene as our default scene for now to allow
                // consumers waiting on scene availability to initialize.
                if (!m_defaultSceneReady)
                {
                    m_defaultScene = scene;
                    Render::Bootstrap::NotificationBus::Broadcast(
                        &Render::Bootstrap::NotificationBus::Handler::OnBootstrapSceneReady, m_defaultScene.get());
                    m_defaultSceneReady = true;
                }

                return true;
            }

            void BootstrapSystemComponent::RunBRDFPipeline(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext)
            {
                // As part of our initialization we need to create the BRDF texture generation pipeline

                // Save a reference to the generated BRDF texture so it doesn't get deleted if all the passes refering to it get deleted
                // and it's ref count goes to zero
                if (!m_brdfTexture)
                {

                    const AZStd::shared_ptr<const RPI::PassTemplate> brdfTextureTemplate =
                        RPI::PassSystemInterface::Get()->GetPassTemplate(Name("BRDFTextureTemplate"));
                    Data::Asset<RPI::AttachmentImageAsset> brdfImageAsset = RPI::AssetUtils::LoadAssetById<RPI::AttachmentImageAsset>(
                        brdfTextureTemplate->m_imageAttachments[0].m_assetRef.m_assetId, RPI::AssetUtils::TraceLevel::Error);
                    if (brdfImageAsset.IsReady())
                    {
                        m_brdfTexture = RPI::AttachmentImage::FindOrCreate(brdfImageAsset);
                    }
                }

                AZ::RPI::RenderPipelineDescriptor pipelineDesc;
                pipelineDesc.m_mainViewTagName = "MainCamera";
                pipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
                pipelineDesc.m_executeOnce = true;
                const AzFramework::ViewportId viewportId = viewportContext ? viewportContext->GetId() : AzFramework::InvalidViewportId;
                for (int deviceIndex{ 0 }; deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
                {
                    pipelineDesc.m_name = AZStd::string::format("BRDFTexturePipeline_%d_%d", viewportId, deviceIndex);

                    if (!scene->GetRenderPipeline(AZ::Name(pipelineDesc.m_name)))
                    {
                        RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
                        brdfTexturePipeline->GetRootPass()->SetDeviceIndex(deviceIndex);
                        scene->AddRenderPipeline(brdfTexturePipeline);
                    }
                }
            }

            void BootstrapSystemComponent::SwitchRenderPipeline(const AZ::RPI::RenderPipelineDescriptor& newRenderPipelineDesc, AZ::RPI::ViewportContextPtr viewportContext)
            {
                AZ::RPI::RenderPipelineDescriptor pipelineDescriptor = newRenderPipelineDesc;
                pipelineDescriptor.m_name =
                    AZStd::string::format("%s_%i", pipelineDescriptor.m_name.c_str(), viewportContext->GetId());

                if (pipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositionsCount &&
                    !RHI::RHISystemInterface::Get()->GetDevice()->GetFeatures().m_customSamplePositions)
                {
                    // Disable custom sample positions because they are not supported
                    AZ_Warning(
                        "BootstrapSystemComponent",
                        false,
                        "Disabling custom sample positions for pipeline %s because they are not supported on this device",
                        pipelineDescriptor.m_name.c_str());
                    pipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositions = {};
                    pipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositionsCount = 0;
                }

                // Create new render pipeline
                auto oldRenderPipeline = viewportContext->GetRenderScene()->GetDefaultRenderPipeline();
                RPI::RenderPipelinePtr newRenderPipeline = RPI::RenderPipeline::CreateRenderPipelineForWindow(
                        pipelineDescriptor, *viewportContext->GetWindowContext().get(), AZ::RPI::ViewType::Default);

                // Switch render pipeline
                viewportContext->GetRenderScene()->RemoveRenderPipeline(oldRenderPipeline->GetId());
                auto view = oldRenderPipeline->GetDefaultView();
                oldRenderPipeline = nullptr;
                viewportContext->GetRenderScene()->AddRenderPipeline(newRenderPipeline);
                newRenderPipeline->SetDefaultView(view);

                AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(newRenderPipeline->GetRenderSettings().m_multisampleState);
            }

            void BootstrapSystemComponent::SwitchAntiAliasing(const AZStd::string& newAntiAliasing, AZ::RPI::ViewportContextPtr viewportContext)
            {
                auto defaultRenderPipeline = viewportContext->GetRenderScene()->GetDefaultRenderPipeline();
                defaultRenderPipeline->SetActiveAAMethod(newAntiAliasing);
            }

            void BootstrapSystemComponent::SwitchMultiSample(const uint16_t newSampleCount, AZ::RPI::ViewportContextPtr viewportContext)
            {
                auto multiSampleStete = viewportContext->GetRenderScene()->GetDefaultRenderPipeline()->GetRenderSettings().m_multisampleState;
                multiSampleStete.m_samples = newSampleCount;
                AZ::RPI::RPISystemInterface::Get()->SetApplicationMultisampleState(multiSampleStete);
            }

            void BootstrapSystemComponent::RefreshWindowResolution()
            {
                SetWindowResolution();
            }

            RPI::RenderPipelinePtr BootstrapSystemComponent::LoadPipeline( AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext,
                                                    AZStd::string_view pipelineName, AZ::RPI::ViewType viewType, AZ::RHI::MultisampleState& multisampleState)
            {
                // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene.
                // When running with an Asset Processor, this will attempt to compile the asset before loading it.
                Data::Asset<RPI::AnyAsset> pipelineAsset =
                    RPI::AssetUtils::LoadCriticalAsset<RPI::AnyAsset>(pipelineName.data(), RPI::AssetUtils::TraceLevel::Error);
                if (pipelineAsset)
                {
                    RPI::RenderPipelineDescriptor renderPipelineDescriptor =
                        *RPI::GetDataFromAnyAsset<RPI::RenderPipelineDescriptor>(pipelineAsset); // Copy descriptor from asset
                    pipelineAsset.Release();

                    renderPipelineDescriptor.m_name =
                        AZStd::string::format("%s_%i", renderPipelineDescriptor.m_name.c_str(), viewportContext->GetId());

                    if (renderPipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositionsCount &&
                        !RHI::RHISystemInterface::Get()->GetDevice()->GetFeatures().m_customSamplePositions)
                    {
                        // Disable custom sample positions because they are not supported
                        AZ_Warning(
                            "BootstrapSystemComponent",
                            false,
                            "Disabling custom sample positions for pipeline %s because they are not supported on this device",
                            pipelineName.data());
                        renderPipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositions = {};
                        renderPipelineDescriptor.m_renderSettings.m_multisampleState.m_customPositionsCount = 0;
                    }
                    multisampleState = renderPipelineDescriptor.m_renderSettings.m_multisampleState;

                    // Create and add render pipeline to the scene (when not added already)
                    RPI::RenderPipelinePtr renderPipeline = scene->GetRenderPipeline(AZ::Name(renderPipelineDescriptor.m_name));
                    if (!renderPipeline)
                    {
                        renderPipeline = RPI::RenderPipeline::CreateRenderPipelineForWindow(
                            renderPipelineDescriptor, *viewportContext->GetWindowContext().get(), viewType);
                        scene->AddRenderPipeline(renderPipeline);
                    }
                    return renderPipeline;
                }
                else
                {
                    AZ_Error("AtomBootstrap", false, "Pipeline file failed to load from path: %s.", pipelineName.data());
                    return nullptr;
                }
            }

            void BootstrapSystemComponent::CreateDefaultRenderPipeline()
            {
                EnsureDefaultRenderPipelineInstalledForScene(m_defaultScene, m_viewportContext);

                const auto pipeline = m_defaultScene->FindRenderPipelineForWindow(m_viewportContext->GetWindowHandle());
                if (pipeline)
                {
                    m_renderPipelineId = pipeline->GetId();
                }
            }

            void BootstrapSystemComponent::DestroyDefaultScene()
            {
                if (m_defaultScene)
                {
                    RPI::RPISystemInterface::Get()->UnregisterScene(m_defaultScene);

                    // Unbind m_defaultScene to the GameEntityContext's AzFramework::Scene
                    if (m_defaultFrameworkScene)
                    {
                        m_defaultFrameworkScene->UnsetSubsystem(m_defaultScene);
                    }

                    m_defaultScene = nullptr;
                    m_defaultFrameworkScene = nullptr;
                }
            }

            void BootstrapSystemComponent::RemoveRenderPipeline()
            {
                if (m_defaultScene && m_defaultScene->GetRenderPipeline(m_renderPipelineId))
                {
                    m_defaultScene->RemoveRenderPipeline(m_renderPipelineId);
                }
                m_renderPipelineId = "";
            }

            void BootstrapSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
            {
                // Temp: When running in the launcher without the legacy renderer
                // we need to call RenderTick on the viewport context each frame.
                if (m_viewportContext)
                {
                    AZ::ApplicationTypeQuery appType;
                    ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                    if (appType.IsGame())
                    {
                        m_viewportContext->RenderTick();
                    }
                }
            }

            int BootstrapSystemComponent::GetTickOrder()
            {
                return TICK_LAST;
            }

            void BootstrapSystemComponent::OnWindowClosed()
            {
                m_windowHandle = nullptr;
                m_viewportContext.reset();
                // On some platforms (e.g. Android) the main window is destroyed when the app is suspended
                // but this doesn't mean that we need to exit the app. The window will be recreated when the app
                // is resumed.
#if AZ_TRAIT_BOOTSTRAPSYSTEMCOMPONENT_EXIT_ON_WINDOW_CLOSE
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
#endif
                AzFramework::WindowNotificationBus::Handler::BusDisconnect();
            }

            void BootstrapSystemComponent::OnWindowResized([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
            {
                SetWindowResolution();
            }

            AzFramework::NativeWindowHandle BootstrapSystemComponent::GetDefaultWindowHandle()
            {
                return m_windowHandle;
            }

            AZStd::shared_ptr<RPI::WindowContext>  BootstrapSystemComponent::GetDefaultWindowContext()
            {
                return m_viewportContext ? m_viewportContext->GetWindowContext() : nullptr;
            }

            void  BootstrapSystemComponent::SetCreateDefaultScene(bool create)
            {
                m_createDefaultScene = create;
            }
        } // namespace Bootstrap
    } // namespace Render
} // namespace AZ
