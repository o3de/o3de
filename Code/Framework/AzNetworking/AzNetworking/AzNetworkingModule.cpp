/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        };
    }
}
