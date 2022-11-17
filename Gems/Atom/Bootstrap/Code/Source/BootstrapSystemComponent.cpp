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

AZ_CVAR(AZ::CVarFixedString, r_default_pipeline_name, AZ_TRAIT_BOOTSTRAPSYSTEMCOMPONENT_PIPELINE_NAME, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Default Render pipeline name");
AZ_CVAR(uint32_t, r_width, 1920, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Starting window width in pixels.");
AZ_CVAR(uint32_t, r_height, 1080, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Starting window height in pixels.");
AZ_CVAR(uint32_t, r_fullscreen, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Starting fullscreen state.");

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
                            ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                        ;
                    }
                }
            }

            void BootstrapSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("BootstrapSystemComponent", 0xb8f32711));
            }

            void BootstrapSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("RPISystem", 0xf2add773));
                required.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
            }

            void BootstrapSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC("ImGuiSystemComponent", 0x2f08b9a7));
                dependent.push_back(AZ_CRC("PrimitiveSystemComponent", 0xc860fa59));
                dependent.push_back(AZ_CRC("MeshSystemComponent", 0x21e5bbb6));
                dependent.push_back(AZ_CRC("CoreLightsService", 0x91932ef6));
                dependent.push_back(AZ_CRC("DynamicDrawService", 0x023c1673));
                dependent.push_back(AZ_CRC("CommonService", 0x6398eec4));
                dependent.push_back(AZ_CRC_CE("HairService"));
            }

            void BootstrapSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("BootstrapSystemComponent", 0xb8f32711));
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

            }

            void BootstrapSystemComponent::Activate()
            {
                // Create a native window only if it's a launcher (or standalone)
                // LY editor create its own window which we can get its handle through AzFramework::WindowSystemNotificationBus::Handler's OnWindowCreated() function
                AZ::ApplicationTypeQuery appType;
                ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                if (!appType.IsValid() || appType.IsGame())
                {
                    // GFX TODO - investigate window creation being part of the GameApplication.

                    auto projectTitle = AZ::Utils::GetProjectDisplayName();

                    // It is important to call this before using r_width, r_height or r_fullscreen
                    // because at the moment this system component initializes before Legacy System.cpp gets to parse
                    // command line arguments into cvars.
                    UpdateCVarsFromCommandLine();

                    m_nativeWindow = AZStd::make_unique<AzFramework::NativeWindow>(projectTitle.c_str(), AzFramework::WindowGeometry(0, 0, r_width, r_height));
                    AZ_Assert(m_nativeWindow, "Failed to create the game window\n");
                    m_nativeWindow->SetFullScreenState(r_fullscreen);

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

                // If the settings registry isn't available, something earlier in startup will report that failure.
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    // Automatically register the event if it's not registered, because
                    // this system is initialized before the settings registry has loaded the event list.
                    AZ::ComponentApplicationLifecycle::RegisterHandler(
                        *settingsRegistry, m_componentApplicationLifecycleHandler,
                        [this](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
                        {
                            Initialize();
                        },
                        "CriticalAssetsCompiled");
                }
            }

            void BootstrapSystemComponent::Deactivate()
            {
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
                    RPI::RPISystemInterface::Get()->InitializeSystemAssets();
                }

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
                }
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
                    AZ::CVarFixedString pipelineName = static_cast<AZ::CVarFixedString>(r_default_pipeline_name);
                    if (xrSystem)
                    {
                        // When running launcher on PC having an XR system present then the default render pipeline is suppose to reflect
                        // what's being rendered into XR device. XR render pipeline uses low end render pipeline.
                        AZ::ApplicationTypeQuery appType;
                        ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
                        if (appType.IsGame())
                        {
                            pipelineName = "passes/LowEndRenderPipeline.azasset";
                        }
                    }

                    if (!LoadPipeline(scene, viewportContext, pipelineName, AZ::RPI::ViewType::Default, multisampleState))
                    {
                        return false;
                    }

                    // As part of our initialization we need to create the BRDF texture generation pipeline
                    AZ::RPI::RenderPipelineDescriptor pipelineDesc;
                    pipelineDesc.m_mainViewTagName = "MainCamera";
                    pipelineDesc.m_name = AZStd::string::format("BRDFTexturePipeline_%i", viewportContext->GetId());
                    pipelineDesc.m_rootPassTemplate = "BRDFTexturePipeline";
                    pipelineDesc.m_executeOnce = true;

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

                    if (!scene->GetRenderPipeline(AZ::Name(pipelineDesc.m_name)))
                    {
                        RPI::RenderPipelinePtr brdfTexturePipeline = AZ::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);
                        scene->AddRenderPipeline(brdfTexturePipeline);
                    }
                }

                // Load XR pipelines if applicable
                if (xrSystem)
                {
                    for (AZ::u32 i = 0; i < xrSystem->GetNumViews(); i++)
                    {
                        const AZ::RPI::ViewType viewType = (i == 0)
                            ? AZ::RPI::ViewType::XrLeft
                            : AZ::RPI::ViewType::XrRight;
                        const AZStd::string_view xrPipelineAssetName = (viewType == AZ::RPI::ViewType::XrLeft)
                            ? "passes/XRLeftRenderPipeline.azasset"
                            : "passes/XRRightRenderPipeline.azasset";

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

            bool BootstrapSystemComponent::LoadPipeline( AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext,
                                                    AZStd::string_view pipelineName, AZ::RPI::ViewType viewType, AZ::RHI::MultisampleState& multisampleState)
            {
                // Create a render pipeline from the specified asset for the window context and add the pipeline to the scene.
                // When running with no Asset Processor (for example in release), CompileAssetSync will return AssetStatus_Unknown.
                AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
                AzFramework::AssetSystemRequestBus::BroadcastResult(
                    status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, pipelineName.data());
                AZ_Assert(
                    status == AzFramework::AssetSystem::AssetStatus_Compiled || status == AzFramework::AssetSystem::AssetStatus_Unknown,
                    "Could not compile the default render pipeline at '%s'",
                    pipelineName.data());

                Data::Asset<RPI::AnyAsset> pipelineAsset =
                    RPI::AssetUtils::LoadAssetByProductPath<RPI::AnyAsset>(pipelineName.data(), RPI::AssetUtils::TraceLevel::Error);
                if (pipelineAsset)
                {
                    RPI::RenderPipelineDescriptor renderPipelineDescriptor =
                        *RPI::GetDataFromAnyAsset<RPI::RenderPipelineDescriptor>(pipelineAsset); // Copy descriptor from asset
                    pipelineAsset.Release();

                    renderPipelineDescriptor.m_name =
                        AZStd::string::format("%s_%i", renderPipelineDescriptor.m_name.c_str(), viewportContext->GetId());

                    multisampleState = renderPipelineDescriptor.m_renderSettings.m_multisampleState;

                    // Create and add render pipeline to the scene (when not added already)
                    if (!scene->GetRenderPipeline(AZ::Name(renderPipelineDescriptor.m_name)))
                    {
                        RPI::RenderPipelinePtr renderPipeline = RPI::RenderPipeline::CreateRenderPipelineForWindow(
                            renderPipelineDescriptor, *viewportContext->GetWindowContext().get(), viewType);
                        scene->AddRenderPipeline(renderPipeline);
                    }
                    return true;
                }
                else
                {
                    AZ_Error("AtomBootstrap", false, "Pipeline file failed to load from path: %s.", pipelineName.data());
                    return false;
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
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
                AzFramework::WindowNotificationBus::Handler::BusDisconnect();
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
