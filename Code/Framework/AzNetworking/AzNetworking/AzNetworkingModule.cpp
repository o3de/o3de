/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <AzNetworking/AzNetworkingModule.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>

namespace AzNetworking
{
    AzNetworkingModule::AzNetworkingModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            NetworkingSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzNetworkingModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<NetworkingSystemComponent>(),
			// TargetManagementComponent. We explicitly type here to workaround a dependency loop with AzFramework
            AZ::TypeId("{39899133-42B3-4e92-A579-CDDC85A23277}"),
        };
    }
} // namespace AzNetworking

// DO NOT MODIFY THIS LINE
// The first parameter should be Gem_AzNetworking
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AzNetworking, AzNetworking::AzNetworkingModule)
