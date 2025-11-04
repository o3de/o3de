/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <QInputDialog>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

namespace GraphCanvas
{
    ////////////////////////
    // AddPresetMenuAction
    ////////////////////////

    static const size_t s_maximumDisplaySize = 150;

    AddPresetMenuAction::AddPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath)
        : ConstructContextMenuAction("Add Preset", contextMenu)
        , m_subMenuPath(subMenuPath)
        , m_preset(preset)
    {
        if (preset->GetDisplayName().size() > s_maximumDisplaySize)
        {
            AZStd::string substring = preset->GetDisplayName();
            substring = substring.substr(0, s_maximumDisplaySize - 3);

            substring.append("...");

            setText(substring.c_str());
        }
        else
        {
            setText(preset->GetDisplayName().c_str());
        }

        QPixmap* pixmap = preset->GetDisplayIcon(contextMenu->GetEditorId());

        if (pixmap)
        {     
            QIcon icon;
            icon.addPixmap(*pixmap, QIcon::Normal);
            icon.addPixmap(*pixmap, QIcon::Active);
            setIcon(icon);
        }

        m_isInToolbar = contextMenu->IsToolBarMenu();
    }

    bool AddPresetMenuAction::IsInSubMenu() const
    {
        return !m_isInToolbar;
    }

    AZStd::string AddPresetMenuAction::GetSubMenuPath() const
    {
        return m_subMenuPath;
    }

    ContextMenuAction::SceneReaction AddPresetMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        AZ::Entity* graphCanvasEntity = CreateEntityForPreset();
        
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Preset Entity");

        if (graphCanvasEntity)
        {
            if (m_preset)
            {
                m_preset->ApplyPreset(graphCanvasEntity->GetId());
            }

            AddEntityToGraph(graphId, graphCanvasEntity, scenePos);
        }

        if (graphCanvasEntity != nullptr)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }

    //////////////////////////
    // ApplyPresetMenuAction
    //////////////////////////

    ApplyPresetMenuAction::ApplyPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset, AZStd::string_view subMenuPath)
        : ConstructContextMenuAction("Apply Preset", contextMenu)
        , m_subMenuPath(subMenuPath)
        , m_preset(preset)
    {
        if (preset->GetDisplayName().size() > s_maximumDisplaySize)
        {
            AZStd::string substring = preset->GetDisplayName();
            substring = substring.substr(0, s_maximumDisplaySize - 3);

            substring.append("...");

            setText(substring.c_str());
        }
        else
        {
            setText(preset->GetDisplayName().c_str());
        }

        QPixmap* pixmap = preset->GetDisplayIcon(contextMenu->GetEditorId());

        if (pixmap)
        {
            QIcon icon;
            icon.addPixmap(*pixmap, QIcon::Normal);
            icon.addPixmap(*pixmap, QIcon::Active);
            setIcon(icon);
        }
    }

    bool ApplyPresetMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string ApplyPresetMenuAction::GetSubMenuPath() const
    {
        return m_subMenuPath;
    }

    ConstructContextMenuAction::SceneReaction ApplyPresetMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        if (m_preset->IsValidEntityForPreset(GetTargetId()))
        {
            const auto& presetSaveData = m_preset->GetPresetData();

            EntitySaveDataRequestBus::Event(GetTargetId(), &EntitySaveDataRequests::ApplyPresetData, presetSaveData);
            GraphCanvasPropertyInterfaceNotificationBus::Event(GetTargetId(), &GraphCanvasPropertyInterfaceNotifications::OnPropertyComponentChanged);

            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    //////////////////////////////
    // CreatePresetFromSelection
    //////////////////////////////

    CreatePresetFromSelection::CreatePresetFromSelection(QObject* parent)
        : ContextMenuAction("Create Preset From", parent)
    {
    }

    CreatePresetFromSelection::~CreatePresetFromSelection()
    {
    }

    ContextMenuAction::SceneReaction CreatePresetFromSelection::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        bool acceptText = true;

        const AZ::EntityId& targetId = GetTargetId();
        const GraphId& graphId = GetGraphId();

        EditorId editorId = GetEditorId();

        ViewId viewId;
        SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

        GraphCanvasGraphicsView* graphicsView = nullptr;
        ViewRequestBus::EventResult(graphicsView, viewId, &ViewRequests::AsGraphicsView);

        QString presetName;

        if (graphicsView)
        {
            while (true)
            {
                presetName = QInputDialog::getText(graphicsView, tr("Set Preset Name"), tr("Preset Name"), QLineEdit::Normal, "", &acceptText);

                if (acceptText)
                {
                    if (!presetName.isEmpty())
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        if (presetName.isEmpty())
        {
            return SceneReaction::Nothing;
        }

        EditorConstructPresets* presets = nullptr;
        AssetEditorSettingsRequestBus::EventResult(presets, editorId, &AssetEditorSettingsRequests::GetConstructPresets);

        if (presets)
        {
            presets->CreatePresetFrom(targetId, presetName.toUtf8().data());
        }

        return SceneReaction::Nothing;
    }

    ///////////////////////////
    // PresetsMenuActionGroup
    ///////////////////////////

    PresetsMenuActionGroup::PresetsMenuActionGroup(ConstructType constructType)
        : m_contextMenu(nullptr)
        , m_constructType(constructType)
        , m_isDirty(false)
    {
    }

    PresetsMenuActionGroup::~PresetsMenuActionGroup()
    {
    }

    void PresetsMenuActionGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        m_contextMenu = contextMenu;
        AssetEditorPresetNotificationBus::Handler::BusConnect(m_contextMenu->GetEditorId());

        m_isDirty = true;
        RefreshPresets();
    }

    void PresetsMenuActionGroup::RefreshPresets()
    {
        if (m_isDirty && m_contextMenu)
        {
            // Bit of a hack right now.
            // Need to rework the base menu a little bit more to make it
            // more dynamic. For now I'll just bypass most of the underlying logic
            // and treat it like a normal QMenu.
            bool isFinalized = m_contextMenu->IsFinalized();

            if (isFinalized)
            {
                if (m_contextMenu->IsToolBarMenu())
                {
                    m_contextMenu->clear();
                }
                else
                {
                    for (const AZStd::string& subMenu : m_subMenus)
                    {
                        // Remove all of the previous preset actions to avoid needing to figure out what actually changed.
                        if (QMenu* menu = m_contextMenu->FindSubMenu(subMenu))
                        {
                            menu->clear();
                        }
                    }
                }
            }

            const ConstructTypePresetBucket* presetBucket = nullptr;
            AssetEditorSettingsRequestBus::EventResult(
                presetBucket, m_contextMenu->GetEditorId(), &AssetEditorSettingsRequests::GetConstructTypePresetBucket, m_constructType);

            if (presetBucket)
            {
                for (auto preset : presetBucket->GetPresets())
                {
                    if (ConstructContextMenuAction* menuAction = CreatePresetMenuAction(m_contextMenu, preset))
                    {
                        if (isFinalized)
                        {
                            if (m_contextMenu->IsToolBarMenu())
                            {
                                m_contextMenu->addAction(menuAction);
                            }
                            else
                            {
                                if (QMenu* menu = m_contextMenu->FindSubMenu(menuAction->GetSubMenuPath()))
                                {
                                    menu->addAction(menuAction);
                                    m_menus.insert(menu);
                                }
                            }
                        }
                        else
                        {
                            m_subMenus.insert(menuAction->GetSubMenuPath());
                            m_contextMenu->AddMenuAction(menuAction);
                        }
                    }
                }
            }

            m_isDirty = false;
        }
    }

    void PresetsMenuActionGroup::OnPresetsChanged()
    {
        m_isDirty = true;
    }

    void PresetsMenuActionGroup::OnConstructPresetsChanged(ConstructType constructType)
    {
        if (m_constructType == constructType)
        {
            m_isDirty = true;
        }
    }

    void PresetsMenuActionGroup::SetEnabled(bool enabled)
    {
        if (m_contextMenu->IsToolBarMenu())
        {
            m_contextMenu->setEnabled(false);
        }
        else
        {
            for (QMenu* menu : m_menus)
            {
                menu->setEnabled(enabled);
            }

            for (AZStd::string subMenu : m_subMenus)
            {
                QMenu* menu = m_contextMenu->FindSubMenu(subMenu);
                menu->setEnabled(enabled);
            }
        }
    }

    ///////////////////////////////
    // ApplyPresetMenuActionGroup
    ///////////////////////////////

    ApplyPresetMenuActionGroup::ApplyPresetMenuActionGroup(ConstructType constructType)
        : PresetsMenuActionGroup(constructType)
    {
    }

    ApplyPresetMenuActionGroup::~ApplyPresetMenuActionGroup()
    {
    }

    void ApplyPresetMenuActionGroup::RefreshActionGroup(const GraphId& graphId, const AZ::EntityId& /*targetId*/)
    {
        bool hasMultipleSelection = false;
        SceneRequestBus::EventResult(hasMultipleSelection, graphId, &SceneRequests::HasMultipleSelection);

        if (hasMultipleSelection)
        {
            SetEnabled(false);
        }
    }

    ConstructContextMenuAction* ApplyPresetMenuActionGroup::CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
    {
        return aznew ApplyPresetMenuAction(contextMenu, preset, "Apply Preset");
    }

    ///////////////////////////////
    // AddCommentPresetMenuAction
    ///////////////////////////////

    AddCommentPresetMenuAction::AddCommentPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
        : AddPresetMenuAction(contextMenu, preset, "Add Comment")
    {
    }

    AZ::Entity* AddCommentPresetMenuAction::CreateEntityForPreset() const
    {
        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvasRequests::CreateCommentNodeAndActivate);

        return graphCanvasEntity;
    }

    void AddCommentPresetMenuAction::AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const
    {
        AZ::EntityId groupTarget;
        SceneRequestBus::EventResult(groupTarget, graphId, &SceneRequests::FindTopmostGroupAtPoint, ConversionUtils::AZToQPoint(scenePos));

        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);
        SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos, false);

        if (groupTarget.IsValid())
        {
            NodeGroupRequestBus::Event(groupTarget, &NodeGroupRequests::AddElementToGroup, graphCanvasEntity->GetId());
        }

        if (IsInSubMenu())
        {
            CommentUIRequestBus::Event(graphCanvasEntity->GetId(), &CommentUIRequests::SetEditable, true);
        }
        else
        {
            CommentRequestBus::Event(graphCanvasEntity->GetId(), &CommentRequests::SetComment, "New Comment");
            SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &SceneMemberUIRequests::SetSelected, true);
        }
    }

    ///////////////////////////////////////
    // CreateCommentPresetMenuActionGroup
    ///////////////////////////////////////

    CreateCommentPresetMenuActionGroup::CreateCommentPresetMenuActionGroup()
        : PresetsMenuActionGroup(ConstructType::CommentNode)
    {

    }

    ConstructContextMenuAction* CreateCommentPresetMenuActionGroup::CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
    {
        return aznew AddCommentPresetMenuAction(contextMenu, preset);
    }

    /////////////////////////////////
    // AddNodeGroupPresetMenuAction
    /////////////////////////////////

    AddNodeGroupPresetMenuAction::AddNodeGroupPresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
        : AddPresetMenuAction(contextMenu, preset, "Group")
    {
    }

    AZ::Entity* AddNodeGroupPresetMenuAction::CreateEntityForPreset() const
    {
        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvasRequests::CreateNodeGroupAndActivate);

        return graphCanvasEntity;
    }

    void AddNodeGroupPresetMenuAction::AddEntityToGraph(const GraphId& graphId, AZ::Entity* graphCanvasEntity, const AZ::Vector2& scenePos) const
    {
        AZStd::vector< AZ::EntityId > selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

        AZ::EntityId topMostGroup;

        // If we don't have any selected nodes, we'll create this at the center of the view. Need to account for when a group
        // already exists there.
        if (selectedNodes.empty())
        {
            SceneRequestBus::EventResult(topMostGroup, graphId, &SceneRequests::FindTopmostGroupAtPoint, ConversionUtils::AZToQPoint(scenePos));

            if (topMostGroup == graphCanvasEntity->GetId())
            {
                topMostGroup.SetInvalid();
            }
        }

        AZStd::unordered_set<AZ::EntityId> selectedGroups;
        AZStd::unordered_set<AZ::EntityId> groupableSet;

        // Phase one find all of the groups
        for (AZ::EntityId selectedNode : selectedNodes)
        {
            if (GraphUtils::IsNodeGroup(selectedNode))
            {
                selectedGroups.insert(selectedNode);
            }
        }

        SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos, false);

        // Work our way up the group parent chain. If we find a single group which will be the parent for our new group.
        AZStd::unordered_multiset<AZ::EntityId> previousGroups;
        AZStd::vector< AZ::EntityId > previousGroupOrdering;
        bool setupPreviousGroup = true;

        for (AZ::EntityId selectedNode : selectedNodes)
        {
            AZ::EntityId groupableElement = GraphUtils::FindOutermostNode(selectedNode);

            if (GraphUtils::IsGroupableElement(groupableElement))
            {
                bool manageGroupOwnership = true;

                AZStd::vector< AZ::EntityId> parentGroupOrdering;
                
                AZ::EntityId groupId;
                GroupableSceneMemberRequestBus::EventResult(groupId, groupableElement, &GroupableSceneMemberRequests::GetGroupId);

                while (groupId.IsValid())
                {
                    parentGroupOrdering.emplace_back(groupId);

                    if (selectedGroups.find(groupId) != selectedGroups.end())
                    {
                        manageGroupOwnership = false;
                        break;
                    }

                    GroupableSceneMemberRequestBus::EventResult(groupId, groupId, &GroupableSceneMemberRequests::GetGroupId);
                }

                // If one of our parent groups is apart of the selections, do not modify our group.
                if (manageGroupOwnership)
                {
                    groupableSet.insert(groupableElement);

                    previousGroups.insert(parentGroupOrdering.begin(), parentGroupOrdering.end());

                    if (setupPreviousGroup)
                    {
                        previousGroupOrdering = parentGroupOrdering;
                        setupPreviousGroup = false;
                    }                    
                    
                    GroupableSceneMemberRequestBus::Event(groupableElement, &GroupableSceneMemberRequests::RemoveFromGroup);
                }
            }
        }

        // IF all of our selected nodes have the same parent group. We want to create a new subgroup inside of the previous group.
        AZ::EntityId parentGroup;

        for (AZ::EntityId groupId : previousGroupOrdering)
        {
            if (previousGroups.count(groupId) == groupableSet.size())
            {
                parentGroup = groupId;
                break;
            }
        }

        // If our parent group is invalid. Assign it to the topmost group.
        if (!parentGroup.IsValid())
        {
            parentGroup = topMostGroup;
        }

        NodeGroupRequestBus::Event(graphCanvasEntity->GetId(), &NodeGroupRequests::AddElementsToGroup, groupableSet);

        // Initialize the title size so when we resize, we're accounting for our title space.
        // There's a weird edge case here: Coming from a context menu, this won't be initialized.
        // But if we're coming from the tool bar, the title is the correct size.
        //
        // Both flows more or less invoke this exact path, so I'm guessing it's the context menu causing some interruption
        // with the qt application update that causes this to be slightly delayed at this point?
        if (IsInSubMenu())
        {
            NodeGroupRequestBus::Event(graphCanvasEntity->GetId(), &NodeGroupRequests::AdjustTitleSize);
        }

        const bool growGroupOnly = false;
        NodeGroupRequestBus::Event(graphCanvasEntity->GetId(), &NodeGroupRequests::ResizeGroupToElements, growGroupOnly);

        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

        CommentRequestBus::Event(graphCanvasEntity->GetId(), &CommentRequests::SetComment, "New Group");

        if (parentGroup.IsValid())
        {
            NodeGroupRequestBus::Event(parentGroup, &NodeGroupRequests::AddElementToGroup, graphCanvasEntity->GetId());

            NodeGroupRequestBus::Event(parentGroup, &NodeGroupRequests::ResizeGroupToElements, true);
        }
        
        if (IsInSubMenu())
        {
            CommentUIRequestBus::Event(graphCanvasEntity->GetId(), &CommentUIRequests::SetEditable, true);
        }
        else
        {
            SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &SceneMemberUIRequests::SetSelected, true);
        }
    }

    /////////////////////////////////////////
    // CreateNodeGroupPresetMenuActionGroup
    /////////////////////////////////////////

    CreateNodeGroupPresetMenuActionGroup::CreateNodeGroupPresetMenuActionGroup()
        : PresetsMenuActionGroup(ConstructType::NodeGroup)
    {
    }

    ConstructContextMenuAction* CreateNodeGroupPresetMenuActionGroup::CreatePresetMenuAction(EditorContextMenu* contextMenu, AZStd::shared_ptr<ConstructPreset> preset)
    {
        return aznew AddNodeGroupPresetMenuAction(contextMenu, preset);
    }
}
