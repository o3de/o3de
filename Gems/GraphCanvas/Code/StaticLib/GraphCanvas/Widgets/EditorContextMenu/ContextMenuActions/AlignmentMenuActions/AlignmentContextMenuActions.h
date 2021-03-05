/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        AZ_CLASS_ALLOCATOR(AlignSelectionMenuAction, AZ::SystemAllocator, 0);
    
        static void CreateAlignmentSubMenu(EditorContextMenu* contextMenu);
    
        AlignSelectionMenuAction(AZStd::string_view name, GraphUtils::VerticalAlignment verAlign, GraphUtils::HorizontalAlignment horAlign, QObject* parent);
        virtual ~AlignSelectionMenuAction() = default;
        
        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;
    
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:
        
        GraphUtils::VerticalAlignment m_verAlign;
        GraphUtils::HorizontalAlignment m_horAlign;
    };    
}