/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/ImGui/SystemBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Input/Events/InputTextEventListener.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace Render
    {
        class ImGuiFeatureProcessor;
        class ImGuiPass;

        class ImGuiSystemComponent final
            : public AZ::Component
            , public ImGuiSystemRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ImGuiSystemComponent, "1A8549B6-B8CC-4C45-9312-DD8A032DA71F");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile);

            ImGuiSystemComponent();
            ~ImGuiSystemComponent() override = default;

            /// AZ::Component
            void Activate() override;
            void Deactivate() override;

        private:
            /// ImGuiSystemRequestBus::Handler
            void SetGlobalSizeScale(float scale) override;
            void SetGlobalFontScale(float scale) override;

            void HideAllImGuiPasses() override;
            void ShowAllImGuiPasses() override;

            void PushDefaultImGuiPass(ImGuiPass* imguiPass) override;
            void RemoveDefaultImGuiPass(ImGuiPass* imguiPass) override;
            ImGuiPass* GetDefaultImGuiPass() override;

            bool PushActiveContextFromDefaultPass() override;
            bool PushActiveContextFromPass(const AZStd::vector<AZStd::string>& passHierarchy) override;
            bool PopActiveContext() override;
            ImGuiContext* GetActiveContext() override;

            bool RenderImGuiBuffersToCurrentViewport(const ImDrawData& drawData) override;                        

            using PassFunction = AZStd::function<void(ImGuiPass* pass)>;
            void ForAllImGuiPasses(PassFunction func);

            AZStd::vector<ImGuiContext*> m_activeContextStack;
            AZStd::vector<ImGuiPass*> m_defaultImguiPassStack;
            float m_fontScale = 1.0f;  ///< Global scale applied only to the font
            float m_sizeScale = 1.0f;  ///< Global size scale. This doesn't apply to fonts.
        };
    } // namespace RPI
} // namespace AZ
