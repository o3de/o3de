/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/Component/Component.h>

#include <AzFramework/Render/RenderSystemBus.h>
#include <AzFramework/Components/DeprecatedComponentsBus.h>

#include <AtomBridge/AtomBridgeBus.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

namespace AZ
{
    namespace AtomBridge
    {
        // forward declares
        class AtomDebugDisplayViewportInterface;
        class PerViewportDynamicDrawManager;

        class AtomBridgeSystemComponent
            : public Component
            , public AzFramework::Render::RenderSystemRequestBus::Handler
            , public AzFramework::Components::DeprecatedComponentsRequestBus::Handler
            , public Render::Bootstrap::NotificationBus::Handler
            , protected AtomBridgeRequestBus::Handler
            , public AZ::RPI::ViewportContextManagerNotificationsBus::Handler
        {
        public:
            AZ_COMPONENT(AtomBridgeSystemComponent, "{FFB99CE4-2C9E-476D-8140-50A8A696E242}");

            static void Reflect(ReflectContext* context);

            AtomBridgeSystemComponent();
            ~AtomBridgeSystemComponent() override;

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // Component overrides ...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // AzFramework::Render::RenderSystemRequestBus::Handler overrides ...
            AZStd::string GetRendererName() const override;

            // AzFramework::Components::DeprecatedComponentsRequestBus::Handler overrides ...
            void EnumerateDeprecatedComponents(AzFramework::Components::DeprecatedComponentsList& list) const override;

            // AtomBridgeRequestBus::Handler overrides ...
            // (Empty)

            // AZ::Render::Bootstrap::NotificationBus overrides
            void OnBootstrapSceneReady(RPI::Scene* bootstrapScene) override;

            // ViewportContextManagerNotificationsBus overrides
            void OnViewportContextAdded(AZ::RPI::ViewportContextPtr viewportContext) override;
            void OnViewportContextRemoved(AzFramework::ViewportId viewportId) override;

            AzFramework::EntityContextId m_entityContextId;

            RPI::ViewPtr m_view = nullptr;

            AZStd::unordered_map<AzFramework::ViewportId, AZStd::shared_ptr<AtomDebugDisplayViewportInterface> > m_activeViewportsList;
            AZStd::unique_ptr<PerViewportDynamicDrawManager> m_dynamicDrawManager;
        };
    }
} // namespace AZ
