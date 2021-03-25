/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MultiplayerCompressionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MultiplayerCompressionService"));
    }

    void MultiplayerCompressionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MultiplayerCompressionService"));
    }

    MultiplayerCompressionSystemComponent::MultiplayerCompressionSystemComponent()
    {
        m_multiplayerCompressionFactory = new MultiplayerCompressionFactory();
        AZ::Interface<AzNetworking::INetworking>::Get()->RegisterCompressorFactory(m_multiplayerCompressionFactory);
    }

    MultiplayerCompressionSystemComponent::~MultiplayerCompressionSystemComponent()
    {
        AZ::Interface<AzNetworking::INetworking>::Get()->UnregisterCompressorFactory(m_multiplayerCompressionFactory->GetFactoryName());
        delete m_multiplayerCompressionFactory;
    }
}
