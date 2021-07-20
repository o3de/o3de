/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Viewport/MaterialViewportModule.h>
#include <Viewport/MaterialViewportComponent.h>
#include <Viewport/PerformanceMonitorComponent.h>

namespace MaterialEditor
{
    MaterialViewportModule::MaterialViewportModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            MaterialViewportComponent::CreateDescriptor(),
            PerformanceMonitorComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList MaterialViewportModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<MaterialViewportComponent>(),
            azrtti_typeid<PerformanceMonitorComponent>(),
        };
    }
}
