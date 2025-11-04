/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "TickBusOrderViewerSystemComponent.h"

namespace TickBusOrderViewer
{
    void TickBusOrderViewerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TickBusOrderViewerSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TickBusOrderViewerSystemComponent>(
                    "TickBusOrderViewer", 
                    "Provides a console command for viewing tick bus order, print_tickbus_handlers.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void TickBusOrderViewerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TickBusOrderViewerService"));
    }

    void TickBusOrderViewerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TickBusOrderViewerService"));
    }

    void TickBusOrderViewerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void TickBusOrderViewerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void TickBusOrderViewerSystemComponent::Init()
    {
    }

    void TickBusOrderViewerSystemComponent::Activate()
    {
        TickBusOrderViewerRequestBus::Handler::BusConnect();
    }

    void TickBusOrderViewerSystemComponent::Deactivate()
    {
        TickBusOrderViewerRequestBus::Handler::BusDisconnect();
    }
}
