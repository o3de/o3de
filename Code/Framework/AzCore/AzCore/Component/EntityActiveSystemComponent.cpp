/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityActiveSystemComponent.h>
#include <AzCore/std/algorithm.h>

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

    size_t EntityActiveSystemComponent::GetActiveTypeIndexByName(AZStd::string typeName) const noexcept
    {
        return GetActiveTypeIndexById(AZ::Crc32{ typeName });
    }

    size_t EntityActiveSystemComponent::GetActiveTypeIndexById(AZ::Crc32 typeNameId) const noexcept
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

    size_t EntityActiveSystemComponent::RegisterEntityActiveTypeByName(AZStd::string typeName)
    {
        return RegisterEntityActiveType(AZ::Crc32{ typeName });
    }

    size_t EntityActiveSystemComponent::RegisterEntityActiveType(AZ::Crc32 typeNameId)
    {
        if (m_activeTypeNameToIndex.size() >= kMaxStateFlags)
        {
            return kInvalidIndex;
        }

        // If already Registered just provide the index.
        auto foundindex = AZStd::find(m_activeTypeNameToIndex.begin(), m_activeTypeNameToIndex.end(), typeNameId);
        if (foundindex != m_activeTypeNameToIndex.end())
        {
            return static_cast<size_t>(foundindex - m_activeTypeNameToIndex.begin());
        }

        // Otherwise, Register New
        const size_t newIndex = m_activeTypeNameToIndex.size();
        m_activeTypeNameToIndex.push_back(typeNameId);
        return newIndex;
    }

} // namespace AZ
