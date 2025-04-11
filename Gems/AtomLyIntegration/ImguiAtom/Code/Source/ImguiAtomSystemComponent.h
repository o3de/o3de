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
#include <DebugConsole.h>

#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ
{
    namespace LYIntegration
    {
        class ImGuiFeatureProcessor;

        //! When Atom is enabled, ImGuiManager from LY's ImGui gem will hand over drawing of ImGui via @OtherActiveImGuiRequestBus to this system component.
        //! Note: The LY ImGui gem only supports a single ImGui context, so Atom must have a single ImGui pass marked as the default.
        class ImguiAtomSystemComponent final
            : public AZ::Component
            , public AZ::RPI::ViewportContextNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(ImguiAtomSystemComponent, "{D423E075-D971-435A-A9C1-57C3B0623A9B}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile);

            ~ImguiAtomSystemComponent() override = default;

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

        private:
            void InitializeViewportSizeIfNeeded();

            // ViewportContextNotificationBus overrides...
            void OnRenderTick() override;
            void OnViewportSizeChanged(AzFramework::WindowSize size) override;
            void OnViewportDpiScalingChanged(float dpiScale) override;

            DebugConsole m_debugConsole;
            bool m_initialized = false;
        };
    } // namespace LYIntegration
} // namespace AZ
