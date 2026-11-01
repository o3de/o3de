/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "SceneColorViewModule.h"
#include "EditorSceneColorViewComponent.h"

namespace SceneColorView
{
    class SceneColorViewEditorModule final : public SceneColorViewModule
    {
    public:
        AZ_RTTI(SceneColorViewEditorModule, "{3EE3D712-9E37-4D71-BAE7-6B1BE91BC7E6}", SceneColorViewModule);
        AZ_CLASS_ALLOCATOR(SceneColorViewEditorModule, AZ::SystemAllocator);
        SceneColorViewEditorModule()
        { m_descriptors.push_back(EditorSceneColorViewComponent::CreateDescriptor()); }
    };
}
AZ_DECLARE_MODULE_CLASS(Gem_SceneColorView_Editor, SceneColorView::SceneColorViewEditorModule)
