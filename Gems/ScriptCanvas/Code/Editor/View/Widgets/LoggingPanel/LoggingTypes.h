/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/hash.h>

#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

namespace ScriptCanvasEditor
{
    struct ExecutionIdentifier
    {
        ExecutionIdentifier() = default;
    };

    constexpr AZ::ComponentId k_dynamicallySpawnedControllerId = static_cast<AZ::ComponentId>(-1);

    typedef AZStd::unordered_multimap<AZ::NamedEntityId, ScriptCanvas::GraphIdentifier> EntityGraphRegistrationMap;

    typedef AZStd::unordered_multimap<AZ::NamedEntityId, ScriptCanvas::GraphIdentifier> LoggingEntityMap;
    typedef AZStd::unordered_set<ScriptCanvas::GraphIdentifier> LoggingAssetSet;
    typedef AZ::EntityId LoggingDataId;
}
