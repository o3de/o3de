/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Window/ShaderManagementConsoleWindowModule.h>
#include <Window/ShaderManagementConsoleWindowComponent.h>

void InitShaderManagementConsoleResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(ShaderManagementConsole);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindowModule::ShaderManagementConsoleWindowModule()
    {
        InitShaderManagementConsoleResources();

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
} // namespace ShaderManagementConsole
