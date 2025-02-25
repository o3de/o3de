/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Component/EntityId.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    //! Returns the union of all editor selection bounds on a given Entity.
    //! @note The returned Aabb is in world space.
    AZTF_API AZ::Aabb CalculateEditorEntitySelectionBounds(const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo);
} // namespace AzToolsFramework
