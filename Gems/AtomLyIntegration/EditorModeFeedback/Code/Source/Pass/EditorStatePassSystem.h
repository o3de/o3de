/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateBase.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AZ::Render
{
    //! Container for specialized editor state effect parent pass classes.
    using EditorStateList = AZStd::vector<AZStd::unique_ptr<EditorStateBase>>;

    //! Mapping for mask draw tags to entities rendered to that mask.
    using EntityMaskMap = AZStd::unordered_map<Name, AzToolsFramework::EntityIdSet>;

    //! System for constructing the passes for the editor state effects.
    class EditorStatePassSystem
    {
    public:
        //! Constructs the pass system with the specified editor state effect parent pass instances.
        EditorStatePassSystem(EditorStateList&& editorStates);

        //! Adds the editor state effect parent passes to the specified render pipeline.
        void AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline);

        //! Returns the map of masks to the entities to be rendered to those masks.
        EntityMaskMap GetEntitiesForEditorStates() const;

        //! Configures the editor state pass instances for the specified render pipeline.
        void ConfigureStatePassesForPipeline(RPI::RenderPipeline* renderPipeline);

        //! Removes the editor state pass instances for the specified render pipeline.
        void RemoveStatePassesForPipeline(RPI::RenderPipeline* renderPipeline);

        //! Returns the set of all masks used by this pass system.
        const AZStd::unordered_set<Name>& GetMasks() const
        {
            return m_masks;
        }

        //! Performs any updates for the editor state effect parent pass instances for the given simulation tick.
        void Update();

        //! Returns the pass template name of the main parent pass
        const char* GetParentPassTemplateName() const;

    private:
        //! Parent passes for each editor state (ordering in vector is ordering of rendering).
        EditorStateList m_editorStates;

        //! Set of all masks used by this pass system.
        AZStd::unordered_set<Name> m_masks;
    };
} // namespace AZ::Render
