/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Window/ShaderManagementConsoleWindowModule.h>
#include <Window/ShaderManagementConsoleWindowComponent.h>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindowModule::ShaderManagementConsoleWindowModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            ShaderManagementConsoleWindowComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList ShaderManagementConsoleWindowModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<ShaderManagementConsoleWindowComponent>(),
        };
    }
}
