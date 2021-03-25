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

#include <Source/EntityDomains/GlobalEntityDomain.h>

namespace Multiplayer
{
    GlobalEntityDomain::GlobalEntityDomain()
        : m_controllersActivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersActivated(entityHandle, entityIsMigrating); })
        , m_controllersDeactivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersDeactivated(entityHandle, entityIsMigrating); })
    {
        ;
    }

    bool GlobalEntityDomain::IsInDomain([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle) const
    {
        //if (const GlobalAccessComponent* globalAccessComp = entityHandle->FindComponent<GlobalAccessComponent>())
        //{
        //    return globalAccessComp->GetPropagationMode() == PropagationMode::GlobalResidency;
        //}

        return false;
    }

    void GlobalEntityDomain::ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet)
    {
        for (auto& entityHandle : ownedEntitySet)
        {
            OnControllersActivated(entityHandle, EntityIsMigrating::False);
        }
        GetNetworkEntityManager()->AddControllersActivatedHandler(m_controllersActivatedHandler);
        GetNetworkEntityManager()->AddControllersDeactivatedHandler(m_controllersDeactivatedHandler);
    }

    void GlobalEntityDomain::RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const
    {
        outEntitiesNotInDomain.insert(m_entitiesNotInDomain.begin(), m_entitiesNotInDomain.end());
    }

    void GlobalEntityDomain::DebugDraw() const
    {
        // Nothing to draw
    }

    void GlobalEntityDomain::OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        if (!IsInDomain(entityHandle))
        {
            m_entitiesNotInDomain.insert(entityHandle.GetNetEntityId());
        }
    }

    void GlobalEntityDomain::OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_entitiesNotInDomain.erase(entityHandle.GetNetEntityId());
    }
}
