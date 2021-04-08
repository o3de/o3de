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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AZ
{
    class Entity;
}

namespace AzToolsFramework
{
    using EntityList = AZStd::vector<AZ::Entity*>;

    namespace Prefab
    {
        //! Collects the entities added during deserialization
        class InstanceEntityScrubber
        {
        public:
            AZ_RTTI(InstanceEntityScrubber, "{0BC12562-C240-48AD-89C6-EDF572C9B485}");

            explicit InstanceEntityScrubber(Instance::EntityList& entities);

            void AddEntitiesToScrub(const EntityList& entities);

        private:
            Instance::EntityList& m_entities;
        };
    }
 }
