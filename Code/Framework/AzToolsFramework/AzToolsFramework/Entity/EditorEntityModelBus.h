/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Entity.h>

namespace AzToolsFramework
{
    class EditorEntityModelRequests : public AZ::EBusTraits
    {
    public:
        virtual ~EditorEntityModelRequests() = default;

        virtual void AddToChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId) = 0;
        virtual void RemoveFromChildrenWithOverrides(const EntityIdList& parentEntityIds, const AZ::EntityId& entityId) = 0;
    };
    using EditorEntityModelRequestBus = AZ::EBus<EditorEntityModelRequests>;
}
