/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/AzFrameworkNativeUIModule.h>
#include <AzCore/Debug/Budget.h>

// Component includes
#include <AzFramework/Components/NativeUISystemComponent.h>

AZ_DEFINE_BUDGET(AzFrameworkNativeUI);

namespace AzFramework
{
    AzFrameworkNativeUIModule::AzFrameworkNativeUIModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzFramework::NativeUISystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzFrameworkNativeUIModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components{};

        components.insert(
            components.end(),
            {
                azrtti_typeid<AzFramework::NativeUISystemComponent>(),
            });

        return components;
    }
}
