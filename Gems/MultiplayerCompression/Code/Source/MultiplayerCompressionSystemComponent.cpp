/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzNetworking/Framework/INetworking.h>

#include "MultiplayerCompressionSystemComponent.h"
#include "LZ4Compressor.h"
#include "MultiplayerCompressionFactory.h"

namespace MultiplayerCompression
{
    void MultiplayerCompressionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MultiplayerCompressionSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MultiplayerCompressionSystemComponent>("MultiplayerCompression", "Provides packet compression via an open source library for the Multiplayer Gem")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MultiplayerCompressionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerCompressionService"));
    }

    void MultiplayerCompressionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerCompressionService"));
    }

    void MultiplayerCompressionSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));  // Required for getting AzNetworking::INetworking interface
    }

    void MultiplayerCompressionSystemComponent::Activate()
    {
        // The registering class is responsible for managing the lifetime of the factory.
        // We'll save the factory name required for unregistering later, but not manage the pointer ourselves.
        auto* compressionFactory = new MultiplayerCompressionFactory();
        m_multiplayerCompressionFactoryName = compressionFactory->GetFactoryName();
        AZ::Interface<AzNetworking::INetworking>::Get()->RegisterCompressorFactory(compressionFactory);
    }

    void MultiplayerCompressionSystemComponent::Deactivate()
    {
        AZ::Interface<AzNetworking::INetworking>::Get()->UnregisterCompressorFactory(m_multiplayerCompressionFactoryName);
    }
}
