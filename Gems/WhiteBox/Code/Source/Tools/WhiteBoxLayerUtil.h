/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>

namespace WhiteBox
{
    //! Create a child White Box entity under @p parentEntityId, select it and enter its edit mode (next
    //! tick). One undo step. @return the new entity (invalid on failure).
    AZ::EntityId CreateChildWhiteBoxLayer(AZ::EntityId parentEntityId);

    //! Move (or copy, with @p keepInSource) a layer onto a new sibling entity with the same transform,
    //! White Box settings and collider. One undo step. @return the new entity (invalid + @p error on failure).
    AZ::EntityId ExportWhiteBoxLayerToEntity(
        AZ::EntityId sourceEntityId, int layerIndex, bool keepInSource, AZStd::string& error);
} // namespace WhiteBox
