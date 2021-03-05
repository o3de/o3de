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
