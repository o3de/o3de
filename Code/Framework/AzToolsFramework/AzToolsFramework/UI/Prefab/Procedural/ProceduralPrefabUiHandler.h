/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/Prefab/PrefabUiHandler.h>

#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;
    };

    //! Implements the Editor UI for Procedural Prefabs.
    class ProceduralPrefabUiHandler
        : public PrefabUiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ProceduralPrefabUiHandler, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::ProceduralPrefabUiHandler, "{3A3DF9FF-9C2E-4439-B7B4-72173B5A3502}", PrefabUiHandler);

        ProceduralPrefabUiHandler();
        ~ProceduralPrefabUiHandler() override = default;

        QString GenerateItemTooltip(AZ::EntityId entityId) const override;
    };
} // namespace AzToolsFramework
