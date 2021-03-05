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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <LandscapeCanvasEditorModule.h>
#include <EditorLandscapeCanvasComponent.h>
#include <LandscapeCanvasSystemComponent.h>

namespace LandscapeCanvas
{
    LandscapeCanvasEditorModule::LandscapeCanvasEditorModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            LandscapeCanvasSystemComponent::CreateDescriptor(),
            EditorLandscapeCanvasComponent::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList LandscapeCanvasEditorModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<LandscapeCanvasSystemComponent>(),
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(LandscapeCanvas_19c2b2d5018940108baf252934b8e6bf, LandscapeCanvas::LandscapeCanvasEditorModule)
