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
    //! Handles the rendering of supported drawable entity components to the mask with the given draw tag.
    class EditorStateMaskRenderer
    {
    public:
        //! Constructs the mask renderer for the specified draw tag.
        EditorStateMaskRenderer(const Name& name, Data::Instance<RPI::Material> maskMaterial);

        //! Renders the specified entities to this mask.
        void RenderMaskEntities(const AzToolsFramework::EntityIdSet& entityIds);
    private:

        //! The drawable components of the entities tagged for rendering to this mask.
        AZStd::unordered_map<EntityId, DrawableMeshEntity> m_drawableEntities;

        Name m_drawTag; //!< The draw tag for this mask.
        Data::Instance<RPI::Material> m_maskMaterial = nullptr; //!< The material for this mask.
    };
} // namespace AZ::Render
