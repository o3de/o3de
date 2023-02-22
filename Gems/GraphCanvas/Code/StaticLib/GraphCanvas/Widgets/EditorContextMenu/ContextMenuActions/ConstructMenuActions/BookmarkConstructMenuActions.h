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
    class AddBookmarkMenuAction
        : public ConstructContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddBookmarkMenuAction, AZ::SystemAllocator);

        AddBookmarkMenuAction(QObject* parent);
        virtual ~AddBookmarkMenuAction() = default;

        using ConstructContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}
