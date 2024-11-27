/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ScriptCanvasDeveloper/ScriptCanvasDeveloperGem.h>
#include <ScriptCanvasDeveloper/ScriptCanvasDeveloperComponent.h>
#include <ScriptCanvasDeveloperEditor/ScriptCanvasDeveloperEditorComponent.h>
#include <ScriptCanvasDeveloperEditor/Developer.h>

namespace ScriptCanvas::Developer
{
    ////////////////////////////////////////////////////////////////////////////
    // ScriptCanvasDeveloperModule
    ////////////////////////////////////////////////////////////////////////////

    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    ScriptCanvasDeveloperModule::ScriptCanvasDeveloperModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            ScriptCanvas::Developer::SystemComponent::CreateDescriptor(),
            ScriptCanvasDeveloperEditor::SystemComponent::CreateDescriptor()
        });

        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(ScriptCanvas::Developer::GetComponentDescriptors());
        m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
    }

    ScriptCanvasDeveloperModule::~ScriptCanvasDeveloperModule()
    {
    }

    AZ::ComponentTypeList ScriptCanvasDeveloperModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components;

        components.insert(components.end(), std::initializer_list<AZ::Uuid> {
            azrtti_typeid<ScriptCanvas::Developer::SystemComponent>(),
            azrtti_typeid<ScriptCanvasDeveloperEditor::SystemComponent>()
        });

        return components;
    }
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), ScriptCanvas::Developer::ScriptCanvasDeveloperModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvasDeveloper_Editor, ScriptCanvas::Developer::ScriptCanvasDeveloperModule)
#endif
