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

#pragma once

#include <Source/EntityDomains/IEntityDomain.h>

namespace Multiplayer
{
    class GlobalEntityDomain
        : public IEntityDomain
    {
    public:
        GlobalEntityDomain();

        //! IEntityDomain overrides.
        //! @{
        bool IsInDomain(const ConstNetworkEntityHandle& entityHandle) const override;
        void ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet) override;
        void RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const override;
        void DebugDraw() const override;
        //! @}

    private:
        void OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);
        void OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating);

        EntitiesNotInDomain m_entitiesNotInDomain;
        ControllersActivatedEvent::Handler m_controllersActivatedHandler;
        ControllersDeactivatedEvent::Handler m_controllersDeactivatedHandler;
    };
}
