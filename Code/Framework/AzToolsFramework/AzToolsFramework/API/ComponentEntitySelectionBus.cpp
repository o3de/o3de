/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>

DECLARE_EBUS_INSTANTIATION(AzToolsFramework::EditorComponentSelectionRequests);

namespace AzToolsFramework
{
    //! Returns the union of all editor selection bounds on a given Entity.
    //! @note The returned Aabb is in world space.
    AZ::Aabb CalculateEditorEntitySelectionBounds(const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ::EBusReduceResult<AZ::Aabb, AzFramework::AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        EditorComponentSelectionRequestsBus::EventResult(
            aabbResult, entityId, &EditorComponentSelectionRequestsBus::Events::GetEditorSelectionBoundsViewport, viewportInfo);

        return aabbResult.value;
    }
}
