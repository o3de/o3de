/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XRControllerAnimationsComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>

#include <Integration/ActorComponentBus.h>

#include <Integration/AnimGraphComponentBus.h>

#include "OpenXRVk/OpenXRVkUtils.h"

namespace OpenXRVk
{
    void XRControllerAnimationsComponent::Reflect(AZ::ReflectContext* context)
    {
        XRControllersConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<XRControllersConfig>();

            serializeContext->Class<XRControllerAnimationsComponent, AZ::Component>()
                ->Version(1)
                ->Field("Controller Items config", &XRControllerAnimationsComponent::m_controllersConfig)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XRControllerAnimationsComponent>("XR Controller Animation", "Provides animations for controls on VR controller")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "XR")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRControllerAnimationsComponent::m_controllersConfig, "Controller Items config", "Configuration for each item of the controller")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<XRControllerAnimationsComponent>("XR Component Animation Group")
                ->Attribute(AZ::Script::Attributes::Category, "OpenXRVk Gem Group")
                ;
        }
    }

    void XRControllerAnimationsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XRControllerAnimationService"));
    }

    void XRControllerAnimationsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("XRControllerAnimationService"));
    }

    void XRControllerAnimationsComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void XRControllerAnimationsComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void XRControllerAnimationsComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void XRControllerAnimationsComponent::Deactivate()
    {
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    extern AZ::Transform ReadActionHandlePose(IOpenXRActions* iface, IOpenXRActions::ActionHandle actionHandle);

    void XRControllerAnimationsComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        auto actionsIFace = OpenXRActionsInterface::Get();
        if (!actionsIFace)
        {
            return;
        }

        for (XRControllersConfig& cfg : m_controllersConfig)
        {
            if (!cfg.m_actionHandle.IsValid())
            {
                cfg.m_actionHandle = actionsIFace->GetActionHandle("main_action_set", cfg.m_actionName);
                if (!cfg.m_actionHandle.IsValid())
                {
                    continue;
                }
            }
            switch (cfg.m_controlItemType)
            {
                case XRControllersConfig::ControlItemType::Boolean:
                {
                    auto outcome = actionsIFace->GetActionStateBoolean(cfg.m_actionHandle);
                    if (outcome.IsSuccess())
                    {
                        bool res = outcome.GetValue();
                        if (res != cfg.m_prevBoolean)
                        {
                            EMotionFX::Integration::AnimGraphComponentRequestBus::Event(GetEntityId(), &EMotionFX::Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterBool, cfg.m_animGraphParameter.c_str(), res);
                            cfg.m_prevBoolean = res;
                        }
                    }
                    break;
                }
                case XRControllersConfig::ControlItemType::Float:
                {
                    float res = ReadActionHandleFloat(actionsIFace, cfg.m_actionHandle, 0.001f);
                    if (res != cfg.m_prevFloat)
                    {
                        EMotionFX::Integration::AnimGraphComponentRequestBus::Event(GetEntityId(), &EMotionFX::Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterFloat, cfg.m_animGraphParameter.c_str(), res);
                        cfg.m_prevFloat = res;
                    }
                    break;
                }
                case XRControllersConfig::ControlItemType::Vector2:
                {
                    float res = (ReadActionHandleFloat(actionsIFace, cfg.m_actionHandle, 0.001f) + 1.0f) / 2.0f;
                    if (res != cfg.m_prevFloat)
                    {
                        EMotionFX::Integration::AnimGraphComponentRequestBus::Event(GetEntityId(), &EMotionFX::Integration::AnimGraphComponentRequestBus::Events::SetNamedParameterFloat, cfg.m_animGraphParameter.c_str(), res);
                        cfg.m_prevFloat = res;
                    }
                    break;
                }
            }
        }
    }
} // namespace OpenXRVk
