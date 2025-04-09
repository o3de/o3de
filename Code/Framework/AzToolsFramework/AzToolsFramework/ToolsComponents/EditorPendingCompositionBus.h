/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h> // for Entity::ComponentArrayType

namespace AzToolsFramework
{
    class EditorPendingCompositionRequests
        : public AZ::ComponentBus
    {
    public:
        virtual void GetPendingComponents(AZ::Entity::ComponentArrayType& components) = 0;
        virtual void AddPendingComponent(AZ::Component* componentToAdd) = 0;
        virtual void RemovePendingComponent(AZ::Component* componentToRemove) = 0;
        virtual bool IsComponentPending(const AZ::Component* component) = 0;
    };

    using EditorPendingCompositionRequestBus = AZ::EBus<EditorPendingCompositionRequests>;
} // namespace AzToolsFramework
