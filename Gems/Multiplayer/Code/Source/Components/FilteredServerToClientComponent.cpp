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
