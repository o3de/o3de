/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Visibility/VisibleGeometryBus.h>

namespace AZ
{
    class EntityId;
}

namespace WhiteBox
{
    struct WhiteBoxRenderData;

    //! @brief Convert white box render data into visible geometry mesh data used by other systems
    //! @param entityId White box enter the id used to retrieve transform and bounds
    //! @param renderData White box render data that will be converted into visible geometry data
    //! @return Returns the populated visible geometry structure populated with the white box render mesh data.
    AzFramework::VisibleGeometry BuildVisibleGeometryFromWhiteBoxRenderData(
        const AZ::EntityId& entityId, const WhiteBoxRenderData& renderData);
} // namespace WhiteBox
