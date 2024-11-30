/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackModule.h>
#include "EditorModeFeedbackSystemComponent.h"

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorModeFeedbackModule, AZ::SystemAllocator)

        EditorModeFeedbackModule::EditorModeFeedbackModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    EditorModeFeedbackSystemComponent::CreateDescriptor()
                });
        }

        AZ::ComponentTypeList EditorModeFeedbackModule::GetRequiredSystemComponents() const
        {
            // add required SystemComponents to the SystemEntity
            return AZ::ComponentTypeList{ azrtti_typeid<EditorModeFeedbackSystemComponent>() };
        }
    } // namespace Render
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), AZ::Render::EditorModeFeedbackModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_EditorModeFeedback_Editor, AZ::Render::EditorModeFeedbackModule)
#endif
