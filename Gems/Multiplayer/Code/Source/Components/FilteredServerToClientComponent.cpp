/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Multiplayer/Components/FilteredServerToClientComponent.h>

namespace Multiplayer
{
    void FilteredServerToClientComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<FilteredServerToClientComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* ptrEdit = serializeContext->GetEditContext())
            {
                using namespace AZ::Edit;
                ptrEdit->Class<FilteredServerToClientComponent>("FilteredServerToClientComponent", "Enables filtering of entities.")
                    ->ClassElement(ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Networking")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
            }
        }
    }

    void FilteredServerToClientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("FilteredServerToClientService"));
    }

    void FilteredServerToClientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("FilteredServerToClientService"));
    }

    void FilteredServerToClientComponent::Activate()
    {
        FilteredServerToClientRequestBus::Handler::BusConnect(GetEntityId());

        FilteredServerToClientNotificationBus::Broadcast(&FilteredServerToClientNotificationBus::Events::OnFilteredServerToClientActivated, GetEntityId());
    }

    void FilteredServerToClientComponent::Deactivate()
    {
        FilteredServerToClientRequestBus::Handler::BusDisconnect();
    }

    void FilteredServerToClientComponent::SetFilteredReplicationHandlerChanged(FilteredReplicationHandlerChanged::Handler handler)
    {
        handler.Connect(m_filteringHandlerChanged);
    }

    void FilteredServerToClientComponent::SetFilteredInterface(FilteredReplicationInterface* filteredReplication)
    {
        m_filteringHandler = filteredReplication;
        m_filteringHandlerChanged.Signal(m_filteringHandler);
    }

    FilteredReplicationInterface* FilteredServerToClientComponent::GetFilteredInterface()
    {
        return m_filteringHandler;
    }
}
