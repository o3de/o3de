/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <CryCommon/CrySystemBus.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Font/FontInterface.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AtomLyIntegration/AtomViewportDisplayInfo/AtomViewportInfoDisplayBus.h>

namespace AZ
{
    class TickRequests;

    namespace Render
    {
        class AtomViewportDisplayInfoSystemComponent
            : public AZ::Component
            , public AZ::RPI::ViewportContextNotificationBus::Handler
            , public AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler
        {
        public:
            AZ_COMPONENT(AtomViewportDisplayInfoSystemComponent, "{AC32F173-E7E2-4943-8E6C-7C3091978221}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;

            // AZ::RPI::ViewportContextNotificationBus::Handler overrides...
            void OnRenderTick() override;

            // AZ::AtomBridge::AtomViewportInfoDisplayRequestBus::Handler overrides...
            AtomBridge::ViewportInfoDisplayState GetDisplayState() const override;
            void SetDisplayState(AtomBridge::ViewportInfoDisplayState state) override;

        private:
            AZ::RPI::ViewportContextPtr GetViewportContext() const;
            void DrawLine(AZStd::string_view line, AZ::Color color = AZ::Colors::White);

            void UpdateFramerate();

            void DrawRendererInfo();
            void DrawCameraInfo();
            void DrawPassInfo();
            void DrawMemoryInfo();
            void DrawFramerate();

            static constexpr float BaseFontSize = 0.7f;

            AZStd::string m_rendererDescription;
            AzFramework::TextDrawParameters m_drawParams;
            AzFramework::FontDrawInterface* m_fontDrawInterface = nullptr;
            float m_lineSpacing;
            AZStd::chrono::duration<double> m_fpsInterval = AZStd::chrono::seconds(1);
            AZStd::deque<AZStd::chrono::steady_clock::time_point> m_fpsHistory;
            AZStd::optional<AZStd::chrono::steady_clock::time_point> m_lastMemoryUpdate;
            bool m_updateRootPassQuery = true;
        };
    } // namespace Render
} // namespace AZ
