/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DebugSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <CrySystemBus.h>

namespace Vegetation
{
    void DebugSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationDebugSystemService", 0x8cac3d67));
    }

    void DebugSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationDebugSystemService", 0x8cac3d67));
    }

    void DebugSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {

    }

    void DebugSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<DebugSystemComponent>("Vegetation Debug System", "Stores and manages vegetation debug data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    DebugSystemComponent::DebugSystemComponent()
    {
    }

    DebugSystemComponent::~DebugSystemComponent()
    {
    }

    Vegetation::DebugData* DebugSystemComponent::GetDebugData()
    {
        return &m_debugData;
    }

    void DebugSystemComponent::Activate()
    {
        DebugSystemDataBus::Handler::BusConnect();
    }

    void DebugSystemComponent::Deactivate()
    {
        DebugSystemDataBus::Handler::BusDisconnect();
    }

}
