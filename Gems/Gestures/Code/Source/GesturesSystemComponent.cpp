/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GesturesSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GestureInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GestureInputService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("InputSystemService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GesturesSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("DoublePressConfig", &GesturesSystemComponent::m_doublePressConfig)
                ->Field("DragConfig", &GesturesSystemComponent::m_dragConfig)
                ->Field("HoldConfig", &GesturesSystemComponent::m_holdConfig)
                ->Field("PinchConfig", &GesturesSystemComponent::m_pinchConfig)
                ->Field("RotateConfig", &GesturesSystemComponent::m_rotateConfig)
                ->Field("SwipeConfig", &GesturesSystemComponent::m_swipeConfig)
                ->Field("CustomGestureConfigsByName", &GesturesSystemComponent::m_customGestureConfigsByName)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<GesturesSystemComponent>("Gestures", "Interprets raw mouse/touch input in order to detect common gestures like drag, hold, swipe, etc.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &GesturesSystemComponent::m_doublePressConfig,
                        "Double Press", "The config used to create the default double press gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_dragConfig,
                        "Drag", "The config used to create the default drag gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_holdConfig,
                        "Hold", "The config used to create the default hold gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_pinchConfig,
                        "Pinch", "The config used to create the default pinch gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_rotateConfig,
                        "Rotate", "The config used to create the default rotate gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_swipeConfig,
                        "Swipe", "The config used to create the default swipe gesture input channel.")
                    ->DataElement(0, &GesturesSystemComponent::m_customGestureConfigsByName,
                        "Custom Gestures", "Custom gesture name/config pairs that will be used to create additional gesture input channels, in addition to the default gestures that are provided 'out of the box'.")
                ;
            }
        }

        InputDeviceGestures::Reflect(context);
        InputChannelGesture::Type::Reflect(context);

        InputChannelGestureClickOrTap::TypeAndConfig::Reflect(context);
        InputChannelGestureDrag::TypeAndConfig::Reflect(context);
        InputChannelGestureHold::TypeAndConfig::Reflect(context);
        InputChannelGesturePinch::TypeAndConfig::Reflect(context);
        InputChannelGestureRotate::TypeAndConfig::Reflect(context);
        InputChannelGestureSwipe::TypeAndConfig::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    GesturesSystemComponent::GesturesSystemComponent()
    {
        m_doublePressConfig.minClicksOrTaps = 2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    GesturesSystemComponent::~GesturesSystemComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::Activate()
    {
        // Insert all default gesture configs into a map
        InputDeviceGestures::ConfigsByNameMap configsByName;
        configsByName[InputDeviceGestures::Gesture::DoublePress.GetName()] = &m_doublePressConfig;
        configsByName[InputDeviceGestures::Gesture::Drag.GetName()] = &m_dragConfig;
        configsByName[InputDeviceGestures::Gesture::Hold.GetName()] = &m_holdConfig;
        configsByName[InputDeviceGestures::Gesture::Pinch.GetName()] = &m_pinchConfig;
        configsByName[InputDeviceGestures::Gesture::Rotate.GetName()] = &m_rotateConfig;
        configsByName[InputDeviceGestures::Gesture::Swipe.GetName()] = &m_swipeConfig;

        // Now insert any custom name/config pairs. If any of the names are the same as one of the
        // existing default gesture input channel ids, it will not be added to avoid any conflicts.
        configsByName.insert(m_customGestureConfigsByName.begin(), m_customGestureConfigsByName.end());

        // Create the gesture input device using the map of gesture input channels name/config pairs
        m_gesturesDevice.reset(aznew InputDeviceGestures(configsByName));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void GesturesSystemComponent::Deactivate()
    {
        // Destroy the gesture input device
        m_gesturesDevice.reset(nullptr);
    }
} // namespace Gestures
