/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "GradientSignal_precompiled.h"
#include "EditorGradientTransformComponent.h"
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace GradientSignal
{
    void EditorGradientTransformComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorGradientTransformComponent, BaseClassType>(context, 1, &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType,1>);
    }

    void EditorGradientTransformComponent::Activate()
    {
        BaseClassType::Activate();

        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        UpdateFromShape();
    }

    void EditorGradientTransformComponent::Deactivate()
    {
        //ensure that we disconnect from any observed buses on teardown
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        BaseClassType::Deactivate();
    }

    AZ::u32 EditorGradientTransformComponent::ConfigurationChanged()
    {
        BaseClassType::ConfigurationChanged();

        UpdateFromShape();

        // Refresh attributes because changing shapes affects read-only status of bounds
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorGradientTransformComponent::OnCompositionChanged()
    {
        UpdateFromShape();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientTransformComponent::UpdateFromShape()
    {
        // Update config from shape on game component, copy that back to our config
        m_component.UpdateFromShape();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();
    }
} //namespace GradientSignal