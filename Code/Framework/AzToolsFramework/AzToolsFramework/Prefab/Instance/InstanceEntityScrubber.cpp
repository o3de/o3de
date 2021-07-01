/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceEntityScrubber.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        InstanceEntityScrubber::InstanceEntityScrubber(Instance::EntityList& entities)
            : m_entities(entities)
        {}

        void InstanceEntityScrubber::AddEntitiesToScrub(const EntityList& entities)
        {
            m_entities.insert(m_entities.end(), entities.begin(), entities.end());
        }
    }
}
