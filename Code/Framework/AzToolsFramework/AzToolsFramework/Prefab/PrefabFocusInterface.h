/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    /*!
     * PrefabFocusInterface
     */
    class PrefabFocusInterface
    {
    public:
        AZ_RTTI(PrefabFocusInterface, "{F3CFA37B-5FD8-436A-9C30-60EB54E350E1}");

        // TODO - Add comment
        virtual void FocusOnOwningPrefab(AZ::EntityId entityId) = 0;

        // TODO - Add comment
        virtual TemplateId GetFocusedPrefabTemplateId() = 0;

        // TODO - Add comment
        virtual InstanceOptionalReference GetFocusedPrefabInstance() = 0;

        // TODO - Add comment
        virtual bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) = 0;
    };

} // namespace AzToolsFramework::Prefab
