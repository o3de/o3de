/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    using NewLayerFunction = AZStd::function<AZ::EntityId()>;

    // Creates a pull out menu that allows the selected entities to be assigned to a new layer.
    // This expects a flattened selection hierarchy to be given,
    // no entities in the set should be children or grandchildren of other entities.
    void SetupAddToLayerMenu(
        QMenu* parentMenu,
        const AzToolsFramework::EntityIdSet& entitySelectionWithFlatHierarchy,
        NewLayerFunction newLayerFunction);
}
