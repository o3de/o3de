/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Interface/Interface.h>

AZ_DECLARE_BUDGET(AzToolsFramework);

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        class UndoCacheInterface
        {
        public:
            AZ_RTTI(UndoCacheInterface, "{CC1E4411-A045-4906-AD39-2E0445B5EADA}");

            //! Store the new entity state or replace the old state.
            virtual void UpdateCache(const AZ::EntityId& entityId) = 0;

            //! Remove the cache line for the entity, if there is one.
            virtual void PurgeCache(const AZ::EntityId& entityId) = 0;

            //! Clear the entire cache.
            virtual void Clear() = 0;

            //! Verify if any changes to the entity was not correctly notified.
            virtual void Validate(const AZ::EntityId& entityId) = 0;
        };

    } // namespace UndoSystem
} // namespace AzToolsFramework

