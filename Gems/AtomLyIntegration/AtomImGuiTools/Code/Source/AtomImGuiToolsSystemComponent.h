/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <CrySystemBus.h>

#if defined(IMGUI_ENABLED)
#include <ImGuiBus.h>
#include <imgui/imgui.h>
#include <Atom/Utils/ImGuiGpuProfiler.h>
#include <Atom/Utils/ImGuiPassTree.h>
#include <Atom/Utils/ImGuiShaderMetrics.h>
#include <Atom/Utils/ImGuiTransientAttachmentProfiler.h>
#endif

namespace AtomImGuiTools
{
    class AtomImGuiToolsSystemComponent
        : public AZ::Component
#if defined(IMGUI_ENABLED)
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif
        , public CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(AtomImGuiToolsSystemComponent, "{AFA2493D-DF1C-4DBB-BC13-0AF990B3D5FC}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

#if defined(IMGUI_ENABLED)
        // ImGuiUpdateListenerBus overrides...
        void OnImGuiUpdate() override;
        void OnImGuiMainMenuUpdate() override;
#endif

        // CrySystemEventBus overrides...
        void OnCryEditorInitialized() override;
        
    private:

#if defined(IMGUI_ENABLED)
        AZ::Render::ImGuiPassTree m_imguiPassTree;
        bool m_showPassTree = false;

        AZ::Render::ImGuiGpuProfiler m_imguiGpuProfiler;
        bool m_showGpuProfiler = false;

        AZ::Render::ImGuiTransientAttachmentProfiler m_imguiTransientAttachmentProfiler;
        bool m_showTransientAttachmentProfiler = false;

        AZ::Render::ImGuiShaderMetrics m_imguiShaderMetrics;
        bool m_showShaderMetrics = false;
#endif
    };

} // namespace AtomImGuiTools
