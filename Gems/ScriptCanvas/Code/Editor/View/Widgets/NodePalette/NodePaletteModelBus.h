/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    struct NodePaletteModelInformation;
    using NodePaletteId = AZ::EntityId;

    class NodePaletteModelNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnAssetModelRepopulated() = 0;

        virtual void OnAssetNodeAdded(NodePaletteModelInformation* nodeIdentifier) = 0;
        virtual void OnAssetNodeRemoved(NodePaletteModelInformation* nodeIdentifier) = 0;
    };

    using NodePaletteModelNotificationBus = AZ::EBus<NodePaletteModelNotifications>;
}
