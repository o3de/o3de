/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Window/MaterialEditorWindowModule.h>
#include <Window/MaterialEditorWindowComponent.h>

void InitMaterialEditorResources()
{
    //Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialEditor);
    Q_INIT_RESOURCE(InspectorWidget);
}

namespace MaterialEditor
{
    MaterialEditorWindowModule::MaterialEditorWindowModule()
    {
        InitMaterialEditorResources();

        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            MaterialEditorWindowComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList MaterialEditorWindowModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<MaterialEditorWindowComponent>(),
        };
    }
}
