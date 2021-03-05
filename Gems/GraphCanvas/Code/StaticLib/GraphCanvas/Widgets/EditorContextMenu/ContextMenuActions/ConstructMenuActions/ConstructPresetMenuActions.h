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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructContextMenuAction.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>

namespace GraphCanvas
{
    class AddPresetMenuAction
        : public ConstructContextMenuAction
    {
    protected:
        AddPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath);

    public:
        AZ_CLASS_ALLOCATOR(AddPresetMenuAction, AZ::SystemAllocator, 0);
        
        virtual ~AddPresetMenuAction() = default;
        
        bool IsInSubMenu() const override;
        AZStd::string GetSubMenuPath() const override;

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

    private:

        virtual AZ::Entity* CreateEntityForPreset() const = 0;
        virtual void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const = 0;        

        bool                               m_isInToolbar;
        AZStd::string                      m_subMenuPath;
        AZStd::shared_ptr<ConstructPreset> m_preset;
    };

    class CreatePresetFromSelection
        : public ContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreatePresetFromSelection, AZ::SystemAllocator, 0);

        CreatePresetFromSelection(QObject* parent = nullptr);
        virtual ~CreatePresetFromSelection();

        SceneReaction TriggerAction(const AZ::Vector2& scenePos) override;

        static ActionGroupId GetCreateConstructContextMenuActionGroupId()
        {
            return AZ_CRC("CreateConstructActionGroup", 0x33ff526b);
        }

        ActionGroupId GetActionGroupId() const override
        {
            return GetCreateConstructContextMenuActionGroupId();
        }
    };

    class PresetsMenuActionGroup
        : public AssetEditorPresetNotificationBus::Handler
    {
    protected:
        PresetsMenuActionGroup(ConstructType constructType);

    public:
        ~PresetsMenuActionGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);
        void RefreshPresets();

        virtual AddPresetMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) = 0;

        // PresetsMenuActionGroup
        void OnPresetsChanged() override;
        void OnConstructPresetsChanged(ConstructType constructType) override;
        ////

        void SetEnabled(bool enabled);

    private:

        AZStd::unordered_set< QMenu* > m_menus;
        AZStd::unordered_set< AZStd::string > m_subMenus;

        EditorContextMenu* m_contextMenu;
        ConstructType m_constructType;

        bool m_isDirty;
    };

    ////////////////////
    // Comment Presets
    ////////////////////

    class AddCommentPresetMenuAction
        : public AddPresetMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddCommentPresetMenuAction, AZ::SystemAllocator, 0);
        AddCommentPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset);

    private:
        AZ::Entity* CreateEntityForPreset() const override;
        void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const override;
    };

    class CommentPresetsMenuActionGroup
        : public PresetsMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(CommentPresetsMenuActionGroup, AZ::SystemAllocator, 0);

        CommentPresetsMenuActionGroup();

        AddPresetMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) override;
    };

    ///////////////////////
    // Node Group Presets
    ///////////////////////

    class AddNodeGroupPresetMenuAction
        : public AddPresetMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(AddNodeGroupPresetMenuAction, AZ::SystemAllocator, 0);
        AddNodeGroupPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset);

    private:
        AZ::Entity* CreateEntityForPreset() const override;
        void AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const override;
    };

    class NodeGroupPresetsMenuActionGroup
        : public PresetsMenuActionGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupPresetsMenuActionGroup, AZ::SystemAllocator, 0);

        NodeGroupPresetsMenuActionGroup();

        AddPresetMenuAction* CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset) override;        
    };
}
