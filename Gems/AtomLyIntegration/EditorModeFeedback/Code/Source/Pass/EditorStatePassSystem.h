/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Pass/State/EditorStateParentPassBase.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AZ::Render
{
    //!
    using EditorStateParentPassList = AZStd::vector<AZStd::unique_ptr<EditorStateParentPassBase>>;

    //!
    using EntityMaskMap = AZStd::unordered_map<Name, AzToolsFramework::EntityIdSet>;

    class EditorStatePassSystem
    {
    public:
        //!
        EditorStatePassSystem(EditorStateParentPassList&& editorStateParentPasses);

        //!
        void AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline);

        //! 
        EntityMaskMap GetEntitiesForEditorStatePasses() const;

        const AZStd::unordered_set<Name>& GetMasks() const
        {
            return m_masks;
        }

    private:
        //! Parent passes for each editor state (ordering in vector is ordering of rendering).
        EditorStateParentPassList m_editorStateParentPasses;

        AZStd::unordered_set<Name> m_masks;
    };
} // namespace AZ::Render
