/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentContextMenuAction.h>

namespace GraphCanvas
{
    class EditCommentMenuAction
        : public CommentContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EditCommentMenuAction, AZ::SystemAllocator);

        EditCommentMenuAction(QObject* parent);
        virtual ~EditCommentMenuAction() = default;

        using ContextMenuAction::RefreshAction;

        void RefreshAction() override;

        using CommentContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
