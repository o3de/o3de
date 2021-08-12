/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace Multiplayer
{
    class MultiplayerDependentSpawnableController final : public AzFramework::DependentSpawnableController
    {
    public:
        static AZ::Name GetControllerName();

    private:
        AZ::Name GetName() const override;
        void ProcessSpawnable(
            const AzFramework::Spawnable& dependentSpawnable,
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& entityIdMap,
            AZ::SerializeContext* serializeContext) override;
    };
} // namespace Multiplayer
