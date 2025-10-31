/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AZ
{
    class Entity;
}

namespace UnitTest
{
    class SpawnableTestFixture : public ToolsApplicationFixture<>
    {
    public:
        constexpr static const char* PathString = "path/to/template";

    protected:
        AZStd::unique_ptr<AZ::Entity> CreateEntity(const char* entityName, AZ::EntityId parentId = AZ::EntityId());
        AZ::Data::Asset<AzFramework::Spawnable> CreateSpawnableAsset(uint64_t entityCount);
    };
} // namespace UnitTest
