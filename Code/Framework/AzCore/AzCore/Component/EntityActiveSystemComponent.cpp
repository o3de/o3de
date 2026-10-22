/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityActiveSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void EntityActiveSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityActiveSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void EntityActiveSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("EntityActivateSystemService"));
    }

    void EntityActiveSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("EntityActivateSystemService"));
    }

    void EntityActiveSystemComponent::Activate()
    {
        EntityActiveSystemRequestBus::Handler::BusConnect();
    }

    void EntityActiveSystemComponent::Deactivate()
    {
        EntityActiveSystemRequestBus::Handler::BusDisconnect();
        m_activeTypeNameToIndex.clear();
    }

    size_t EntityActiveSystemComponent::GetActiveTypeIndexByName(AZStd::string typeName)
    {
        return GetActiveTypeIndexById(AZ::Crc32{ typeName });
    }

    size_t EntityActiveSystemComponent::GetActiveTypeIndexById(AZ::Crc32 typeNameId)
    {
        // Always resolve an already-registered type, even when the registry is full - otherwise a
        // fully-populated registry could no longer return existing indices (including index 0 "Entity").
        size_t index = ScanListForIndex(typeNameId);
        if (index != kInvalidIndex)
        {
            return index;
        }

        // Not registered yet: only hand out a new index if there is still room under the cap.
        if (m_activeTypeNameToIndex.size() >= s_maxStateFlags)
        {
            return kInvalidIndex;
        }

        m_activeTypeNameToIndex.push_back(typeNameId);

        return ScanListForIndex(typeNameId);
    }

    size_t EntityActiveSystemComponent::ScanListForIndex(AZ::Crc32 typeNameId)
    {
        for (size_t i = 0; i < m_activeTypeNameToIndex.size(); i++)
        {
            if (m_activeTypeNameToIndex[i] == typeNameId)
            {
                return i;
            }
        }

        return kInvalidIndex;
    }

} // namespace AZ
