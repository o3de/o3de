/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorGradientTransformComponent::UpdateFromShape()
    {
        if (m_runtimeComponentActive)
        {
            // Update config from shape on game component, copy that back to our config.
            bool notifyDependentsOfChange = true;
            m_component.UpdateFromShape(notifyDependentsOfChange);

            auto oldConfig = m_configuration;
            m_component.WriteOutConfig(&m_configuration);

            if (oldConfig != m_configuration)
            {
                SetDirty();
            }
        }
    }
} //namespace GradientSignal
