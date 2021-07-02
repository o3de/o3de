/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzGameFramework/AzGameFrameworkModule.h>

// Component includes
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Driller/DrillToFileComponent.h>

namespace AzGameFramework
{
    AzGameFrameworkModule::AzGameFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzFramework::DrillToFileComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzGameFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AzFramework::DrillToFileComponent>(),
        };
    }
}
