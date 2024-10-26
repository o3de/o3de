/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
    }

    void EditorSurfaceAltitudeGradientComponent::UpdateFromShape()
    {
        // Update config from shape on game component, copy that back to our config
        m_component.UpdateFromShape();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();
    }
}
