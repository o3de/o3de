/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework
{
    namespace Prefab
    {
        enum class PatchType
        {
            Add,
            Remove,
            Edit // Replace
        };

        enum class OverrideType
        {
            AddEntity,
            RemoveEntity,
            EditEntity,
            AddComponent,
            RemoveComponent,
            EditComponent,
            RemovePrefab
        };
    } // namespace Prefab
} // namespace AzToolsFramework
