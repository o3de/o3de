/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompressionSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Compression
{
    void CompressionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CompressionSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CompressionSystemComponent>("Compression", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CompressionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CompressionService"));
    }

    void CompressionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CompressionService"));
    }

    void CompressionSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void CompressionSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CompressionSystemComponent::CompressionSystemComponent()
    {
        if (CompressionInterface::Get() == nullptr)
        {
            CompressionInterface::Register(this);
        }
    }

    CompressionSystemComponent::~CompressionSystemComponent()
    {
        if (CompressionInterface::Get() == this)
        {
            CompressionInterface::Unregister(this);
        }
    }

    void CompressionSystemComponent::Init()
    {
    }

    void CompressionSystemComponent::Activate()
    {
        CompressionRequestBus::Handler::BusConnect();
    }

    void CompressionSystemComponent::Deactivate()
    {
        CompressionRequestBus::Handler::BusDisconnect();
    }

} // namespace Compression
