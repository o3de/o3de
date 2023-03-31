/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <${Name}Module.h>
#include <Components/${Name}SystemComponent.h>
#include <Components/${Name}Component.h>

#ifdef ${NameUpper}_EDITOR
#include <EditorComponents/Editor${Name}Component.h>
#endif

namespace AZ::Render
{
    ${Name}Module::${Name}Module()
    {
        m_descriptors.insert(m_descriptors.end(),
            {
                ${Name}SystemComponent::CreateDescriptor(),
                ${Name}Component::CreateDescriptor(),

                #ifdef ${NameUpper}_EDITOR
                    Editor${Name}Component::CreateDescriptor(),
                #endif
            });
    }

    AZ::ComponentTypeList ${Name}Module::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{ azrtti_typeid<${Name}SystemComponent>() };
    }
}

