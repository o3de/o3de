/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StarsModule.h>
#include <EditorStarsSystemComponent.h>
#include <EditorStarsComponent.h>

namespace AZ::Render
{
    class EditorStarsModule
        : public StarsModule
    {
    public:
        AZ_RTTI(EditorStarsModule, "{3D5FD4A5-4405-408A-BD64-F3B27006DB4A}", StarsModule);
        AZ_CLASS_ALLOCATOR(EditorStarsModule, AZ::SystemAllocator);

        EditorStarsModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                EditorStarsSystemComponent::CreateDescriptor(),
                EditorStarsComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<EditorStarsSystemComponent>(),
            };
        }
    };
}// namespace AZ::Render 

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AZ::Render::EditorStarsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Stars_Editor, AZ::Render::EditorStarsModule)
#endif
