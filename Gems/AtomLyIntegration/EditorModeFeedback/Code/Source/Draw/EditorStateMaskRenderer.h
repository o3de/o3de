/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/DrawableMeshEntity.h>
#include <Pass/EditorStatePassSystem.h>

#include <AzCore/std/containers/unordered_map.h>

namespace AZ::Render
{
    class EditorStateMaskRenderer
    {
    public:
        void RenderMaskEntities(const EntityMaskMap& entityMaskMap);
    private:
        //!
        using DrawableMeshEntityMap = AZStd::unordered_map<EntityId, DrawableMeshEntity>;

        //!
        AZStd::unordered_map<Name, DrawableMeshEntityMap> m_drawableEntityMaskMap;
    };
} // namespace AZ::Render
