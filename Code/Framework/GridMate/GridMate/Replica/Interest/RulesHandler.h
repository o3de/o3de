/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_RULES_HANDLER_H
#define GM_REPLICA_RULES_HANDLER_H

#include <GridMate/Replica/ReplicaCommon.h>

#include <GridMate/Replica/Interest/InterestDefs.h>

namespace GridMate
{
    class InterestManager;

    /**
    *  BaseRulesHandler: base handler class
    *  RulesHandler's job is to provide InterestManager with matching pairs of attributes and rules.
    */
    class BaseRulesHandler
    {
    public:
        BaseRulesHandler()
            : m_slot(0)
        {}

        virtual ~BaseRulesHandler() { };

        /**
        *  Ticked by interest manager to retrieve new matches or mismatches of interests
        */
        virtual void Update() = 0;

        /**
        *  Returns result of a previous update
        *  This only returns changes that happened on the previous tick not the whole world state
        */
        virtual const InterestMatchResult& GetLastResult() = 0;

        /**
        *  Called by InterestManager when the given handler instance is registered
        */
        virtual void OnRulesHandlerRegistered(InterestManager* manager) = 0;

        /**
        *  Called by InterestManager when the given handler is unregistered
        */
        virtual void OnRulesHandlerUnregistered(InterestManager* manager) = 0;

        /**
        *  Returns interest mananger this handler is bound to, or nullptr if it's unbound
        */
        virtual InterestManager* GetManager() = 0;

    private:
        friend class InterestManager;

        InterestHandlerSlot m_slot;
    };
} // namespace GridMate

#endif // GM_REPLICA_RULES_HANDLER_H
