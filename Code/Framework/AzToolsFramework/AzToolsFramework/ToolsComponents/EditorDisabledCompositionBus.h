/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    class EditorDisabledCompositionRequests
        : public AZ::ComponentBus
    {
    public:
        virtual void GetDisabledComponents(AZStd::vector<AZ::Component*>& components) = 0;
        virtual void AddDisabledComponent(AZ::Component* componentToAdd) = 0;
        virtual void RemoveDisabledComponent(AZ::Component* componentToRemove) = 0;
    };

    using EditorDisabledCompositionRequestBus = AZ::EBus<EditorDisabledCompositionRequests>;
} // namespace AzToolsFramework
