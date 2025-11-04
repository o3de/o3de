/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>
#include <Atom/Bootstrap/BootstrapRequestBus.h>

namespace AZ
{
    namespace Render
    {
        namespace Bootstrap
        {
            class BootstrapSystemComponent
                : public Component
                , public TickBus::Handler
                , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
                , public AzFramework::WindowNotificationBus::Handler
                , public AzFramework::WindowSystemNotificationBus::Handler
                , public AzFramework::WindowSystemRequestBus::Handler
                , public Render::Bootstrap::DefaultWindowBus::Handler
                , public Render::Bootstrap::RequestBus::Handler
            {
            public:
                AZ_COMPONENT(BootstrapSystemComponent, "{1EAFD87D-A64A-4612-93D8-B3AFFA70F09B}");

                BootstrapSystemComponent();
                ~BootstrapSystemComponent() override;

                static void Reflect(ReflectContext* context);

                static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
                static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
                static void GetDependentServices(ComponentDescriptor::DependencyArrayType& required);

                // AzFramework::WindowSystemRequestBus::Handler overrides ...
                AzFramework::NativeWindowHandle GetDefaultWindowHandle() override;

                // Render::Bootstrap::DefaultWindowBus::Handler overrides ...
                AZStd::shared_ptr<RPI::WindowContext> GetDefaultWindowContext() override;
                void SetCreateDefaultScene(bool create) override;

                // Render::Bootstrap::RequestBus::Handler overrides ...
                AZ::RPI::ScenePtr GetOrCreateAtomSceneFromAzScene(AzFramework::Scene* scene) override;
                bool EnsureDefaultRenderPipelineInstalledForScene(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext) override;
                void SwitchRenderPipeline(const AZ::RPI::RenderPipelineDescriptor& newRenderPipelineDesc, AZ::RPI::ViewportContextPtr viewportContext) override;
                void SwitchAntiAliasing(const AZStd::string& newAntiAliasing, AZ::RPI::ViewportContextPtr viewportContext) override;
                void SwitchMultiSample(const uint16_t newSampleCount, AZ::RPI::ViewportContextPtr viewportContext) override;
                void RefreshWindowResolution() override;

            protected:
                // Component overrides ...
                void Activate() override;
                void Deactivate() override;

                // WindowNotificationBus::Handler overrides ...
                void OnWindowClosed() override;
                void OnWindowResized(uint32_t width, uint32_t height) override;

                // TickBus::Handler overrides ...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
                int GetTickOrder() override;

                // AzFramework::WindowSystemNotificationBus::Handler overrides ...
                void OnWindowCreated(AzFramework::NativeWindowHandle windowHandle) override;

                // AzFramework::ApplicationLifecycleEvents::Bus::Handler overrides ...
                void OnApplicationWindowCreated() override;
                void OnApplicationWindowDestroy() override;

            private:
                void Initialize();

                void CreateDefaultRenderPipeline();
                void CreateDefaultScene();
                void DestroyDefaultScene();
                void RemoveRenderPipeline();

                void CreateViewportContext();
                void SetWindowResolution();

                //! Run the BRDF pipeline to generate the BRDF texture
                void RunBRDFPipeline(AZ::RPI::ScenePtr scene, AZ::RPI::ViewportContextPtr viewportContext);

                //! Load a render pipeline from disk and add it to the scene
                RPI::RenderPipelinePtr LoadPipeline(
                    const AZ::RPI::ScenePtr scene,
                    const AZ::RPI::ViewportContextPtr viewportContext,
                    AZStd::string_view xrPipelineName,
                    AZ::RPI::ViewType viewType,
                    AZ::RHI::MultisampleState& multisampleState);

                AzFramework::Scene::RemovalEvent::Handler m_sceneRemovalHandler;

                AZStd::unique_ptr<AzFramework::NativeWindow> m_nativeWindow;
                AzFramework::NativeWindowHandle m_windowHandle = nullptr;
                RPI::ViewportContextPtr m_viewportContext;

                RPI::ScenePtr m_defaultScene = nullptr;
                AZStd::shared_ptr<AzFramework::Scene> m_defaultFrameworkScene = nullptr;

                bool m_isInitialized = false;

                // The id of the render pipeline created by this component
                RPI::RenderPipelineId m_renderPipelineId;
                
                // Save a reference to the image created by the BRDF pipeline so it doesn't get auto deleted if it's ref count goes to zero
                // For example, if we delete all the passes, we won't have to recreate the BRDF pipeline to recreate the BRDF texture
                Data::Instance<RPI::AttachmentImage> m_brdfTexture;

                bool m_createDefaultScene = true;
                bool m_defaultSceneReady = false;

                // Maps AZ scenes to RPI scene weak pointers to allow looking up a ScenePtr instead of a raw Scene*
                AZStd::unordered_map<AzFramework::Scene*, AZStd::weak_ptr<AZ::RPI::Scene>> m_azSceneToAtomSceneMap;

                AZ::SettingsRegistryInterface::NotifyEventHandler m_componentApplicationLifecycleHandler;
            };
        } // namespace Bootstrap
    } // namespace Render
} // namespace AZ
