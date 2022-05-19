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
        void AddPassesToRenderPipeline(RPI::RenderPipeline* renderPipeline) const;

        //! 
        EntityMaskMap GetEntitiesForEditorStatePasses() const;

    private:
        //! Parent passes for each editor state (ordering in vector is ordering of rendering).
        EditorStateParentPassList m_editorStateParentPasses;
    };
} // namespace AZ::Render
