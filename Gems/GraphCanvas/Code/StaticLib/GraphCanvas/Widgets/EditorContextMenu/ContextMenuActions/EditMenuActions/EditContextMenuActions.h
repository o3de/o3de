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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditContextMenuAction.h>

namespace GraphCanvas
{
    class CutGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CutGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        CutGraphSelectionMenuAction(QObject* parent);
        virtual ~CutGraphSelectionMenuAction() = default;

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class CopyGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CopyGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        CopyGraphSelectionMenuAction(QObject* parent);
        virtual ~CopyGraphSelectionMenuAction() = default;

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class PasteGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        using ContextMenuAction::RefreshAction;

        AZ_CLASS_ALLOCATOR(PasteGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        PasteGraphSelectionMenuAction(QObject* parent);
        virtual ~PasteGraphSelectionMenuAction() = default;

        void RefreshAction() override;
        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
    
    class DeleteGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DeleteGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        DeleteGraphSelectionMenuAction(QObject* parent);
        virtual ~DeleteGraphSelectionMenuAction() = default;

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };

    class DuplicateGraphSelectionMenuAction
        : public EditContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DuplicateGraphSelectionMenuAction, AZ::SystemAllocator, 0);
        
        DuplicateGraphSelectionMenuAction(QObject* parent);
        virtual ~DuplicateGraphSelectionMenuAction() = default;

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;
    };
}