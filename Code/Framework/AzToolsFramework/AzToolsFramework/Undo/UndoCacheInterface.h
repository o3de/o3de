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

#include <AzCore/Component/Entity.h>
#include <AzCore/Interface/Interface.h>

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

