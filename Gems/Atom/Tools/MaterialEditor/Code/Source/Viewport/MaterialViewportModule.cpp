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
