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
#include "EditorSurfaceAltitudeGradientComponent.h"

namespace GradientSignal
{
    void EditorSurfaceAltitudeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorSurfaceAltitudeGradientComponent, BaseClassType>(context);
    }

    void EditorSurfaceAltitudeGradientComponent::Activate()
    {
        BaseClassType::Activate();
        UpdateFromShape();
    }

    void EditorSurfaceAltitudeGradientComponent::Deactivate()
    {
        BaseClassType::Deactivate();
    }

    AZ::u32 EditorSurfaceAltitudeGradientComponent::ConfigurationChanged()
    {
        BaseClassType::ConfigurationChanged();

        UpdateFromShape();

        // Refresh attributes because changing shapes affects read-only status of bounds
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    void EditorSurfaceAltitudeGradientComponent::OnCompositionChanged()
    {
        UpdateFromShape();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorSurfaceAltitudeGradientComponent::UpdateFromShape()
    {
        // Update config from shape on game component, copy that back to our config
        m_component.UpdateFromShape();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();
    }
}