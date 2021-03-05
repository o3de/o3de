/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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