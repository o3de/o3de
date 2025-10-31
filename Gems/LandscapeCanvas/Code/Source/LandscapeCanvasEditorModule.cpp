/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LandscapeCanvas::LandscapeCanvasEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LandscapeCanvas, LandscapeCanvas::LandscapeCanvasEditorModule)
#endif
