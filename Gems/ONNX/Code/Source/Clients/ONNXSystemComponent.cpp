/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ONNXSystemComponent.h"
#include "Model.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

namespace ONNX
{
    void ONNXSystemComponent::AddTimingSample(const char* modelName, float inferenceTimeInMilliseconds, AZ::Color modelColor)
    {
        m_timingStats.PushHistogramValue(modelName, inferenceTimeInMilliseconds, modelColor);
    }

    void ONNXSystemComponent::OnImGuiUpdate()
    {
        if (!m_timingStats.m_show)
        {
            return;
        }

        if (ImGui::Begin("ONNX"))
        {
            m_timingStats.OnImGuiUpdate();
        }
    }

    void ONNXSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("ONNX"))
        {
            ImGui::MenuItem(m_timingStats.GetName(), "", &m_timingStats.m_show);
            ImGui::EndMenu();
        }
    }

    void ONNXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ONNXSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ONNXSystemComponent>("ONNX", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void ONNXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ONNXSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ONNXSystemComponent::ONNXSystemComponent()
    {
        if (ONNXInterface::Get() == nullptr)
        {
            ONNXInterface::Register(this);
        }

        m_timingStats.SetName("ONNX Inference Timing Statistics");
        m_timingStats.SetHistogramBinCount(200);

        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    ONNXSystemComponent::~ONNXSystemComponent()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();

        if (ONNXInterface::Get() == this)
        {
            ONNXInterface::Unregister(this);
        }
    }

    Ort::Env* ONNXSystemComponent::GetEnv()
    {
        return m_env.get();
    }

    Ort::AllocatorWithDefaultOptions* ONNXSystemComponent::GetAllocator()
    {
        return m_allocator.get();
    }

    void OnnxLoggingFunction(void*, OrtLoggingLevel, const char* category, const char* logId, const char* codeLocation, const char* message)
    {
        AZ_Printf("ONNX", "%s %s %s %s\n", category, logId, codeLocation, message);
    }

    // The global environment and memory allocator are initialised with the system component, and are accessed via the EBus from within the
    // model. m_precomputedTimingData and m_precomputedTimingDataCuda are structs holding the test inference statistics run before the
    // editor starts up, and used by the ImGui dashboard.
    void ONNXSystemComponent::Init()
    {
        m_env = AZStd::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_VERBOSE, "test_log", OnnxLoggingFunction, nullptr);
        m_allocator = AZStd::make_unique<Ort::AllocatorWithDefaultOptions>();
    }

    void ONNXSystemComponent::Activate()
    {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void ONNXSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ONNXRequestBus::Handler::BusDisconnect();
    }

    void ONNXSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace ONNX
