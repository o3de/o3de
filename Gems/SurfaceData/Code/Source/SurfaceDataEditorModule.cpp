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

#include "SurfaceData_precompiled.h"
#include <SurfaceDataEditorModule.h>
#include <SurfaceDataSystemComponent.h>
#include <Editor/EditorSurfaceDataSystemComponent.h>
#include <Editor/EditorSurfaceDataColliderComponent.h>
#include <Editor/EditorSurfaceDataShapeComponent.h>

namespace SurfaceData
{
    SurfaceDataEditorModule::SurfaceDataEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            EditorSurfaceDataSystemComponent::CreateDescriptor(),
            EditorSurfaceDataColliderComponent::CreateDescriptor(),
            EditorSurfaceDataShapeComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList SurfaceDataEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = SurfaceDataModule::GetRequiredSystemComponents();
        requiredComponents.push_back(azrtti_typeid<EditorSurfaceDataSystemComponent>());
        return requiredComponents;
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_SurfaceDataEditor, SurfaceData::SurfaceDataEditorModule)
