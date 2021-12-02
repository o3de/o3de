/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructContextMenuAction.h>

namespace GraphCanvas
{
    class AddCommentMenuAction
        : public ConstructContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddCommentMenuAction, AZ::SystemAllocator, 0);

        AddCommentMenuAction(QObject* parent);
        virtual ~AddCommentMenuAction() = default;

        using ConstructContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
