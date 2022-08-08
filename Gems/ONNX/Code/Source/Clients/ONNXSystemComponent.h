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
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ImGuiBus.h>
#include <LYImGuiUtils/HistogramGroup.h>
#include <ONNX/ONNXBus.h>
#include <imgui/imgui.h>

namespace ONNX
{
    class ONNXSystemComponent
        : public AZ::Component
        , protected ONNXRequestBus::Handler
        , public AZ::TickBus::Handler
        , public ImGui::ImGuiUpdateListenerBus::Handler
    {
    public:
        AZ_COMPONENT(ONNXSystemComponent, "{CB6735F4-D404-4EE9-A37A-439EDDCC655D}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void OnImGuiUpdate() override;
        void OnImGuiMainMenuUpdate() override;

        AZStd::unique_ptr<Ort::Env> m_env;
        Ort::Env* GetEnv() override;

        AZStd::unique_ptr<Ort::AllocatorWithDefaultOptions> m_allocator;
        Ort::AllocatorWithDefaultOptions* GetAllocator() override;

        ONNXSystemComponent();
        ~ONNXSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ONNXRequestBus interface implementation
        ImGui::LYImGuiUtils::HistogramGroup m_timingStats;

        void AddTimingSample(const char* modelName, float inferenceTimeInMilliseconds, AZ::Color modelColor) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace ONNX
