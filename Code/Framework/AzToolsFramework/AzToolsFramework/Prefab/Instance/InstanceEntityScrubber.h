/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    namespace Prefab
    {
        //! Collects the entities added during deserialization
        class InstanceEntityScrubber
        {
        public:
            AZ_TYPE_INFO(InstanceEntityScrubber, "{0BC12562-C240-48AD-89C6-EDF572C9B485}");

            explicit InstanceEntityScrubber(EntityList& entities);

            void AddEntitiesToScrub(const AZStd::span<AZ::Entity*>& entities);

        private:
            EntityList& m_entities;
        };
    }
 }
