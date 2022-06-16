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
        EditorStateMaskRenderer(Name name, Data::Instance<RPI::Material> maskMaterial);
        void RenderMaskEntities(const AzToolsFramework::EntityIdSet& entityIds);
    private:

        //!
        AZStd::unordered_map<EntityId, DrawableMeshEntity> m_drawableEntities;

        Name m_drawTag;
        Data::Instance<RPI::Material> m_maskMaterial = nullptr;
    };
} // namespace AZ::Render
