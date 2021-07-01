/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
