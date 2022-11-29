/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>

namespace AzToolsFramework
{
    //! The AZ::Interface for component mode collection queries.
    class ComponentModeCollectionInterface
    {
    public:
        AZ_RTTI(ComponentModeCollectionInterface, "{DFAA4450-BBCD-47C0-9B91-FEA2DBD9B152}");

        virtual ~ComponentModeCollectionInterface() = default;

        //! Retrieves the list of all component types (usually one).
        //! @note If called outside of component mode, an empty vector will be returned.
        virtual AZStd::vector<AZ::Uuid> GetComponentTypes() const = 0;

        //! Calls the handler function for each component mode that is currently active.
        using ActiveComponentModeCB = AZStd::function<void(const AZ::EntityComponentIdPair&, const AZ::Uuid&)>;
        virtual void EnumerateActiveComponents(const ActiveComponentModeCB& callBack) const = 0;
    };
} // namespace AzToolsFramework
