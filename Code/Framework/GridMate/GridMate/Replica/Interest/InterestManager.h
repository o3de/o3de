/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_INTERESTMANAGER_H
#define GM_REPLICA_INTERESTMANAGER_H

#include <GridMate/Containers/list.h>

#include <GridMate/Replica/Interest/InterestDefs.h>

namespace GridMate
{
    class BaseRulesHandler;

    /**
    *  Interest manager initialization parameters
    */
    struct InterestManagerDesc
    {
        ReplicaManager* m_rm; ///< Replica manager instance

        InterestManagerDesc()
            : m_rm(nullptr)
        {

        }
    };

    /**
    *  InterestManager: responsible for matching of replicas and peers pairs based on rules and attribute provided.
    *  InterestManager allows registration of up to 32 custom rules handler. Each rules handler is responsible of matching attributes
    *  and rules that user provides. InterestManager is responsible for merging results of matching from every registered handler and
    *  maintaining valid forwarding targets cache on every Replica.
    */
    class InterestManager
    {
    public:

        GM_CLASS_ALLOCATOR(InterestManager);

        InterestManager();
        ~InterestManager();

        /**
        * Initialize manager with a descriptor
        */
        void Init(const InterestManagerDesc& desc);

        /**
        * Returns true if InterestManager is initialized and is ready to use
        */
        bool IsReady() const;

        /**
        * Register new handler with a given type and instance
        */
        void RegisterHandler(BaseRulesHandler* handler);

        /**
        * Unregister handler
        */
        void UnregisterHandler(BaseRulesHandler* handler);

        /**
        * Call to update current replica->peers cache
        */
        void Update();

        /**
        *  Returns replica manager IM is bount to
        */
        ReplicaManager* GetReplicaManager() { return m_rm; }
    private:
        InterestManager(const InterestManager&) = delete;
        InterestManager& operator=(const InterestManager&) = delete;

        InterestHandlerSlot GetNewSlot();
        void FreeSlot(InterestHandlerSlot slot);
        bool ShouldForward(Replica* replica, ReplicaPeer* peer) const;

        ReplicaManager* m_rm;
        vector<BaseRulesHandler*> m_handlers;
        InterestHandlerSlot m_freeSlots;
    };
} // namespace GridMate

#endif // GM_REPLICA_INTERESTMANAGER_H
