/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentContextMenuAction.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    class EditorContextMenu;

    class AlignSelectionMenuAction
        : public AlignmentContextMenuAction
    {
    public:

        AZ_CLASS_ALLOCATOR(AlignSelectionMenuAction, AZ::SystemAllocator);
    
        static void CreateAlignmentSubMenu(EditorContextMenu* contextMenu);
    
        AlignSelectionMenuAction(AZStd::string_view name, GraphUtils::VerticalAlignment verAlign, GraphUtils::HorizontalAlignment horAlign, QObject* parent);
        virtual ~AlignSelectionMenuAction() = default;
        
        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        using AlignmentContextMenuAction::TriggerAction;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:
        
        GraphUtils::VerticalAlignment m_verAlign;
        GraphUtils::HorizontalAlignment m_horAlign;
    };    
}
