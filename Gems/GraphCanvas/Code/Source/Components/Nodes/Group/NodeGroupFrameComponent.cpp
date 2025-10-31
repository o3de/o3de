/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCursor>
#include <QGraphicsLayout>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QScopedValueRollback>
#include <QTimer>

#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Components/Nodes/Group/NodeGroupFrameComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/LayerBus.h>
#include <GraphCanvas/Components/PersistentIdBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace GraphCanvas
{
    ////////////////////////////////////
    // NodeGroupFrameComponentSaveData
    ////////////////////////////////////

    NodeGroupFrameComponent::NodeGroupFrameComponentSaveData::NodeGroupFrameComponentSaveData()
        : m_color(AZ::Color::CreateZero())
        , m_displayHeight(-1)
        , m_displayWidth(-1)
        , m_enableAsBookmark(false)
        , m_shortcut(k_findShortcut)
        , m_isCollapsed(false)
        , m_callback(nullptr)
    {
    }

    NodeGroupFrameComponent::NodeGroupFrameComponentSaveData::NodeGroupFrameComponentSaveData(NodeGroupFrameComponent* nodeFrameComponent)
        : NodeGroupFrameComponentSaveData()
    {
        m_callback = nodeFrameComponent;
    }

    void NodeGroupFrameComponent::NodeGroupFrameComponentSaveData::operator=(const NodeGroupFrameComponentSaveData& other)
    {
        // Purposefully skipping over the callback.
        m_color = other.m_color;
        m_displayHeight = other.m_displayHeight;
        m_displayWidth = other.m_displayWidth;

        m_enableAsBookmark = other.m_enableAsBookmark;
        m_shortcut = other.m_shortcut;

        m_isCollapsed = other.m_isCollapsed;
        m_persistentGroupedIds = other.m_persistentGroupedIds;

        m_isNewGroup = other.m_isNewGroup;
    }

    void NodeGroupFrameComponent::NodeGroupFrameComponentSaveData::OnBookmarkStatusChanged()
    {
        if (m_callback)
        {
            m_callback->OnBookmarkStatusChanged();
            SignalDirty();
        }
    }

    void NodeGroupFrameComponent::NodeGroupFrameComponentSaveData::OnCollapsedStatusChanged()
    {
        if (m_callback)
        {
            if (m_isCollapsed)
            {
                m_callback->CollapseGroup();
            }
            else
            {
                m_callback->ExpandGroup();
            }
        }
    }

    ////////////////////////////
    // NodeGroupFrameComponent
    ////////////////////////////

    bool NodeGroupFrameComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 colorId = AZ_CRC_CE("Color");
            AZ::Crc32 heightId = AZ_CRC_CE("DisplayHeight");
            AZ::Crc32 widthId = AZ_CRC_CE("DisplayWidth");

            NodeGroupFrameComponent::NodeGroupFrameComponentSaveData saveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(colorId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_color);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(heightId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayHeight);
            }

            dataNode = nullptr;
            dataNode = classElement.FindSubElement(widthId);

            if (dataNode)
            {
                dataNode->GetData(saveData.m_displayWidth);
            }

            classElement.RemoveElementByName(colorId);
            classElement.RemoveElementByName(heightId);
            classElement.RemoveElementByName(widthId);

            classElement.AddElementWithData(context, "SaveData", saveData);
        }

        return true;
    }

    bool NodeGroupFrameSaveDataVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 4)
        {
            classElement.AddElementWithData(context, "IsNewGroup", false);
        }

        return true;
    }

    void NodeGroupFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NodeGroupFrameComponentSaveData, ComponentSaveData>()
                ->Version(4, &NodeGroupFrameSaveDataVersionConverter)
                ->Field("Color", &NodeGroupFrameComponentSaveData::m_color)
                ->Field("DisplayHeight", &NodeGroupFrameComponentSaveData::m_displayHeight)
                ->Field("DisplayWidth", &NodeGroupFrameComponentSaveData::m_displayWidth)
                ->Field("EnableAsBookmark", &NodeGroupFrameComponentSaveData::m_enableAsBookmark)
                ->Field("QuickIndex", &NodeGroupFrameComponentSaveData::m_shortcut)
                ->Field("Collapsed", &NodeGroupFrameComponentSaveData::m_isCollapsed)
                ->Field("PersistentGroupedId", &NodeGroupFrameComponentSaveData::m_persistentGroupedIds)
                ->Field("IsNewGroup", &NodeGroupFrameComponentSaveData::m_isNewGroup)
            ;

            serializeContext->Class<NodeGroupFrameComponent, GraphCanvasPropertyComponent>()
                ->Version(2, &NodeGroupFrameComponentVersionConverter)
                ->Field("SaveData", &NodeGroupFrameComponent::m_saveData)
                ->Field("RedirectedEndpoints", &NodeGroupFrameComponent::m_collapsedRedirectionEndpoints)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<NodeGroupFrameComponentSaveData>("NodeGroupFrameComponentSaveData", "Structure that stores all of the save information for a Node Group.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NodeGroupFrameComponentSaveData::m_enableAsBookmark, "Enable as Bookmark", "Toggles whether or not the Node Group is registered as a bookmark.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &NodeGroupFrameComponentSaveData::OnBookmarkStatusChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NodeGroupFrameComponentSaveData::m_isCollapsed, "Collapse Group", "Toggles whether or not the specified Node Group is collapsed.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &NodeGroupFrameComponentSaveData::OnCollapsedStatusChanged)
                ;

                editContext->Class<NodeGroupFrameComponent>("Node Group Frame", "A comment that applies to the visible area.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NodeGroupFrameComponent::m_saveData, "SaveData", "The modifiable information about this Node Group.")
                    ;
            }
        }
    }

    NodeGroupFrameComponent::NodeGroupFrameComponent()
        : m_saveData(this)
        , m_displayLayout(nullptr)
        , m_frameWidget(nullptr)
        , m_titleWidget(nullptr)
        , m_blockWidget(nullptr)
        , m_needsDisplayStateHighlight(false)
        , m_needsManualHighlight(false)
        , m_enableSelectionManipulation(true)
        , m_ignoreDisplayStateChanges(false)
        , m_ignoreSubSlementPostionChanged(false)
        , m_isGroupAnimating(false)
    {
    }

    void NodeGroupFrameComponent::Init()
    {
        GraphCanvasPropertyComponent::Init();

        m_displayLayout = new QGraphicsLinearLayout(Qt::Vertical);
        m_displayLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_titleWidget = aznew NodeGroupFrameTitleWidget();
        m_blockWidget = aznew NodeGroupFrameBlockAreaWidget();

        m_frameWidget = AZStd::make_unique<NodeGroupFrameGraphicsWidget>(GetEntityId(), (*this));

        m_frameWidget->setLayout(m_displayLayout);

        m_displayLayout->setSpacing(0);
        m_displayLayout->setContentsMargins(0, 0, 0, 0);

        m_displayLayout->addItem(m_titleWidget);
        m_displayLayout->addItem(m_blockWidget);

        m_blockWidget->RegisterFrame(m_frameWidget.get());
        m_titleWidget->RegisterFrame(m_frameWidget.get());

        EntitySaveDataRequestBus::Handler::BusConnect(GetEntityId());
    }

    void NodeGroupFrameComponent::Activate()
    {
        GraphCanvasPropertyComponent::Activate();

        NodeNotificationBus::Handler::BusConnect(GetEntityId());
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        NodeGroupRequestBus::Handler::BusConnect(GetEntityId());
        BookmarkRequestBus::Handler::BusConnect(GetEntityId());
        BookmarkNotificationBus::Handler::BusConnect(GetEntityId());
        SceneMemberNotificationBus::MultiHandler::BusConnect(GetEntityId());
        NodeGroupRequestBus::Handler::BusConnect(GetEntityId());
        VisualNotificationBus::MultiHandler::BusConnect(GetEntityId());

        m_frameWidget->Activate();
    }

    void NodeGroupFrameComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        m_frameWidget->Deactivate();

        VisualNotificationBus::MultiHandler::BusDisconnect();
        NodeGroupRequestBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::MultiHandler::BusDisconnect();
        BookmarkNotificationBus::Handler::BusDisconnect();
        BookmarkRequestBus::Handler::BusDisconnect();
        NodeGroupRequestBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void NodeGroupFrameComponent::SetGroupSize(QRectF blockRectangle)
    {
        QScopedValueRollback<bool> allowMovement(m_frameWidget->m_allowMovement, false);

        QRectF titleSize = m_titleWidget->boundingRect();

        if (titleSize.isEmpty())
        {
            m_titleWidget->adjustSize();
            titleSize = m_titleWidget->boundingRect();
        }

        m_saveData.m_displayHeight = aznumeric_cast<float>(blockRectangle.height() + titleSize.height());
        m_saveData.m_displayWidth = aznumeric_cast<float>(AZ::GetMax(m_frameWidget->m_minimumSize.width(), blockRectangle.width()));

        SignalDirty();

        m_frameWidget->ResizeTo(m_saveData.m_displayHeight, m_saveData.m_displayWidth);
        m_frameWidget->adjustSize();

        QPointF position = blockRectangle.topLeft();

        if (m_frameWidget->IsSnappedToGrid())
        {
            position.setY(m_frameWidget->RoundToClosestStep(aznumeric_cast<int>(position.y() - titleSize.height()), m_frameWidget->GetGridYStep()));
        }
        else
        {
            position.setY(position.y() - titleSize.height());
        }

        // Signal Bounds changed needs to happen after the set position to deal with uncollapsing of a collapsed group.
        // Uncollapsing a group triggers the 'Drag' sense, so that will cancel out the bound change reactions if it happens
        // after the bounds change.
        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, AZ::Vector2(aznumeric_cast<float>(position.x()), aznumeric_cast<float>(position.y())));
        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SignalBoundsChanged);
    }

    QRectF NodeGroupFrameComponent::GetGroupBoundingBox() const
    {
        return m_blockWidget->sceneBoundingRect();
    }

    AZ::Color NodeGroupFrameComponent::GetGroupColor() const
    {
        AZ::Color color;
        CommentRequestBus::EventResult(color, GetEntityId(), &CommentRequests::GetBackgroundColor);
        return color;
    }

    void NodeGroupFrameComponent::CollapseGroup()
    {
        if (!m_saveData.m_isCollapsed
            || !m_collapsedNodeId.IsValid())
        {
            bool isRestoring = m_saveData.m_isCollapsed;

            CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, false);

            m_saveData.m_isCollapsed = true;

            CollapsedNodeGroupConfiguration groupedConfiguration;
            groupedConfiguration.m_nodeGroupId = GetEntityId();

            AZ::Entity* nodeGroup = nullptr;
            GraphCanvasRequestBus::BroadcastResult(nodeGroup, &GraphCanvasRequests::CreateCollapsedNodeGroupAndActivate, groupedConfiguration);

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

            if (nodeGroup)
            {
                m_collapsedNodeId = nodeGroup->GetId();
                CollapsedNodeGroupNotificationBus::Handler::BusConnect(m_collapsedNodeId);

                AZ::Vector2 position = ConversionUtils::QPointToVector(m_frameWidget->sceneBoundingRect().center());

                GeometryRequestBus::Event(nodeGroup->GetId(), &GeometryRequests::SetPosition, position);

                // This needs to be called before it's added to the scene. Since the group collapses and generates the slots in it's OnAddedToScene
                if (!m_collapsedRedirectionEndpoints.empty())
                {
                    CollapsedNodeGroupRequestBus::Event(m_collapsedNodeId, &CollapsedNodeGroupRequests::ForceEndpointRedirection, m_collapsedRedirectionEndpoints);
                    m_collapsedRedirectionEndpoints.clear();
                }

                SceneRequestBus::Event(graphId, &SceneRequests::Add, nodeGroup->GetId(), false);

                SceneMemberNotificationBus::MultiHandler::BusConnect(m_collapsedNodeId);
                NodeGroupNotificationBus::Event(GetEntityId(), &NodeGroupNotifications::OnCollapsed, m_collapsedNodeId);

                // Want to add in the collapsed node id to maintain our selection information correctly.
                GraphCanvasPropertyBus::Event(GetEntityId(), &GraphCanvasPropertyInterface::AddBusId, m_collapsedNodeId);
            }
            else
            {
                m_saveData.m_isCollapsed = false;
            }

            if (m_collapsedNodeId.IsValid())
            {
                if (!isRestoring)
                {
                    SignalDirty();
                    GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
                }

                if (m_enableSelectionManipulation)
                {
                    SceneMemberUIRequestBus::Event(m_collapsedNodeId, &SceneMemberUIRequests::SetSelected, true);
                }
            }
            else
            {
                m_saveData.m_isCollapsed = false;

                if (isRestoring)
                {
                    SignalDirty();
                }
            }
        }
    }

    void NodeGroupFrameComponent::ExpandGroup()
    {
        if (m_saveData.m_isCollapsed || m_collapsedNodeId.IsValid())
        {
            if (m_collapsedNodeId.IsValid())
            {
                AZ::EntityId collapsedNodeId = m_collapsedNodeId;
                CollapsedNodeGroupRequestBus::Event(collapsedNodeId, &CollapsedNodeGroupRequests::ExpandGroup);

                GraphCanvasPropertyBus::Event(GetEntityId(), &GraphCanvasPropertyInterface::RemoveBusId, collapsedNodeId);
            }
            else
            {
                SignalExpanded();
            }
        }
    }

    void NodeGroupFrameComponent::UngroupGroup()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        {
            ScopedGraphUndoBlocker undoBlocker(graphId);

            if (m_saveData.m_isCollapsed)
            {
                ExpandGroup();
            }

            for (const AZ::EntityId& groupedElement : m_groupedElements)
            {
                OnElementUngrouped(groupedElement);
            }

            m_groupedElements.clear();

            AZStd::unordered_set< AZ::EntityId > deletionSet = { GetEntityId() };

            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deletionSet);
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
    }

    bool NodeGroupFrameComponent::IsCollapsed() const
    {
        return m_saveData.m_isCollapsed;
    }

    AZ::EntityId NodeGroupFrameComponent::GetCollapsedNodeId() const
    {
        return m_collapsedNodeId;
    }

    void NodeGroupFrameComponent::AddElementToGroup(const AZ::EntityId& groupableElement)
    {
        if (AddToGroupInternal(groupableElement))
        {
            UpdateSavedElements();
        }
    }

    void NodeGroupFrameComponent::AddElementsToGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements)
    {
        bool signalSave = false;

        for (auto groupableElement : groupableElements)
        {
            if (AddToGroupInternal(groupableElement))
            {
                signalSave = true;
            }
        }

        if (signalSave)
        {
            UpdateSavedElements();
        }
    }

    void NodeGroupFrameComponent::AddElementsVectorToGroup(const AZStd::vector<AZ::EntityId>& groupableElements)
    {
        bool signalSave = false;

        for (auto groupableElement : groupableElements)
        {
            if (AddToGroupInternal(groupableElement))
            {
                signalSave = true;
            }
        }

        if (signalSave)
        {
            UpdateSavedElements();
        }
    }

    void NodeGroupFrameComponent::RemoveElementFromGroup(const AZ::EntityId& groupableElement)
    {
        size_t eraseCount = m_groupedElements.erase(groupableElement);

        if (eraseCount > 0)
        {
            OnElementUngrouped(groupableElement);
            UpdateSavedElements();
        }
    }

    void NodeGroupFrameComponent::RemoveElementsFromGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements)
    {
        for (auto groupableElement : groupableElements)
        {
            size_t eraseCount = m_groupedElements.erase(groupableElement);

            if (eraseCount > 0)
            {
                OnElementUngrouped(groupableElement);
            }
        }

        UpdateSavedElements();
    }

    void NodeGroupFrameComponent::RemoveElementsVectorFromGroup(const AZStd::vector<AZ::EntityId>& groupableElements)
    {
        for (auto groupableElement : groupableElements)
        {
            size_t eraseCount = m_groupedElements.erase(groupableElement);

            if (eraseCount > 0)
            {
                OnElementUngrouped(groupableElement);
            }
        }

        UpdateSavedElements();
    }

    void NodeGroupFrameComponent::FindGroupedElements(AZStd::vector< NodeId >& groupedElements)
    {
        groupedElements.reserve(groupedElements.size() + m_groupedElements.size());

        for (const AZ::EntityId& entityId : m_groupedElements)
        {
            groupedElements.push_back(entityId);
        }
    }

    void NodeGroupFrameComponent::ResizeGroupToElements(bool growGroupOnly)
    {
        // 1 or 0 indicates whether to align to that direction or not.
        const int adjustVertical = 1;
        const int adjustHorizontal = 1;

        m_frameWidget->ResizeToGroup(adjustHorizontal, adjustVertical, growGroupOnly);
    }

    bool NodeGroupFrameComponent::IsInTitle(const QPointF& scenePos) const
    {
        return m_titleWidget->sceneBoundingRect().contains(scenePos);
    }

    void NodeGroupFrameComponent::AdjustTitleSize()
    {
        if (m_titleWidget)
        {
            m_titleWidget->adjustSize();
        }
    }

    void NodeGroupFrameComponent::OnCollapsed(const NodeId& collapsedNodeId)
    {
        const AZ::EntityId* busId = NodeGroupNotificationBus::GetCurrentBusId();

        if (busId)
        {
            m_groupedElements.insert(collapsedNodeId);
            OnElementGrouped(collapsedNodeId);

            size_t eraseCount = m_initializingGroups.erase((*busId));

            if (eraseCount != 0 && m_initializingGroups.empty())
            {
                RestoreCollapsedState();
            }
        }
    }

    void NodeGroupFrameComponent::OnExpanded()
    {
        const AZ::EntityId* busId = NodeGroupNotificationBus::GetCurrentBusId();

        if (busId)
        {
            auto collapsedNodeIter = m_collapsedGroupMapping.find((*busId));

            if (collapsedNodeIter != m_collapsedGroupMapping.end())
            {
                AZ::EntityId collapsedGroupId = collapsedNodeIter->second;

                // Erase this from the mapping, since at this point the collapsed node is deleted
                // but we still want the ungrouped node inside of our element.
                m_collapsedGroupMapping.erase(collapsedNodeIter);

                // Remove the collapsed node from the grouped
                m_groupedElements.erase(collapsedGroupId);
                OnElementUngrouped(collapsedGroupId);
                UpdateSavedElements();
            }
        }
    }

    void NodeGroupFrameComponent::OnNodeActivated()
    {
        const AZ::EntityId* busId = NodeNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                QGraphicsLayout* layout = nullptr;
                NodeLayoutRequestBus::EventResult(layout, GetEntityId(), &NodeLayoutRequests::GetLayout);

                layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                m_titleWidget->setLayout(layout);

                CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetCommentMode, CommentMode::BlockComment);
            }
        }
    }

    void NodeGroupFrameComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        const AZ::EntityId* busId = NodeNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                OnBookmarkStatusChanged();

                SceneNotificationBus::Handler::BusDisconnect();
                SceneNotificationBus::Handler::BusConnect(sceneId);                

                const AZ::EntityId& entityId = GetEntityId();

                CommentNotificationBus::Handler::BusConnect(entityId);
                GeometryNotificationBus::MultiHandler::BusConnect(entityId);
                RootGraphicsItemNotificationBus::Handler::BusConnect(entityId);

                GeometryRequestBus::EventResult(m_previousPosition, entityId, &GeometryRequests::GetPosition);

                m_frameWidget->ResizeTo(m_saveData.m_displayHeight, m_saveData.m_displayWidth);

                AZ::Color backgroundColor;
                CommentRequestBus::EventResult(backgroundColor, entityId, &CommentRequests::GetBackgroundColor);

                OnBackgroundColorChanged(backgroundColor);

                if (m_saveData.m_enableAsBookmark)
                {
                    BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RegisterBookmark, entityId);
                    SceneBookmarkRequestBus::Handler::BusConnect(sceneId);
                }

                m_saveData.RegisterIds(entityId, sceneId);

                SceneRequestBus::EventResult(m_editorId, sceneId, &SceneRequests::GetEditorId);
            }
        }
    }

    void NodeGroupFrameComponent::PreOnRemovedFromScene(const AZ::EntityId& /*sceneId*/)
    {
        const AZ::EntityId* busId = SceneMemberNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, false);
            }
        }
    }

    void NodeGroupFrameComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        const AZ::EntityId* busId = SceneMemberNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                bool isRegistered = false;
                BookmarkManagerRequestBus::EventResult(isRegistered, sceneId, &BookmarkManagerRequests::IsBookmarkRegistered, GetEntityId());

                if (isRegistered)
                {
                    BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::UnregisterBookmark, GetEntityId());

                    SceneBookmarkRequestBus::Handler::BusDisconnect(sceneId);
                }

                AZStd::vector< AZ::EntityId > memberIds;
                FindGroupedElements(memberIds);

                AZStd::unordered_set< AZ::EntityId > deletionIds;
                deletionIds.insert(memberIds.begin(), memberIds.end());

                SceneRequestBus::Event(sceneId, &SceneRequests::Delete, deletionIds);
            }
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberAboutToSerialize(GraphSerialization& serializationTarget)
    {
        const AZ::EntityId* busId = SceneMemberNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                AZStd::vector< AZ::EntityId > groupedElements;
                FindGroupedElements(groupedElements);

                AZStd::unordered_set< AZ::EntityId > memberIds;
                memberIds.insert(groupedElements.begin(), groupedElements.end());

                GraphUtils::ParseMembersForSerialization(serializationTarget, memberIds);
            }
            else if ((*busId) == m_collapsedNodeId)
            {
                // Groups we don't want to copy over the collapsed node. But instead we want to copy over
                // the source group(this object).
                //
                // Remove the collapsed node id. And add in the group id.
                serializationTarget.GetGraphData().m_nodes.erase(AzToolsFramework::GetEntity(m_collapsedNodeId));

                CollapsedNodeGroupRequestBus::EventResult(m_collapsedRedirectionEndpoints, m_collapsedNodeId, &CollapsedNodeGroupRequests::GetRedirectedEndpoints);

                AZStd::unordered_set< AZ::EntityId > memberIds = { GetEntityId() };
                GraphUtils::ParseMembersForSerialization(serializationTarget, memberIds);
            }
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization& /*serializationTarget*/)
    {
        const AZ::EntityId* busId = SceneMemberNotificationBus::GetCurrentBusId();

        if (busId != nullptr)
        {
            if ((*busId) == GetEntityId())
            {
                EditorId editorId;
                SceneRequestBus::EventResult(editorId, graphId, &SceneRequests::GetEditorId);

                PersistentIdNotificationBus::Handler::BusConnect(editorId);

                if (m_saveData.m_enableAsBookmark)
                {
                    AZ::EntityId conflictedId;
                    BookmarkManagerRequestBus::EventResult(conflictedId, graphId, &BookmarkManagerRequests::FindBookmarkForShortcut, m_saveData.m_shortcut);

                    if (conflictedId.IsValid() && m_saveData.m_shortcut > 0)
                    {
                        m_saveData.m_shortcut = k_findShortcut;
                    }
                }
            }
        }
    }

    void NodeGroupFrameComponent::OnStyleChanged()
    {
        auto entityId = GetEntityId();

        m_titleWidget->RefreshStyle(entityId);
        m_blockWidget->RefreshStyle(entityId);
        m_frameWidget->RefreshStyle(entityId);

        QSizeF titleMinimumSize = m_titleWidget->minimumSize();
        QSizeF blockMinimumSize = m_blockWidget->minimumSize();

        QSizeF finalMin;
        finalMin.setWidth(AZ::GetMax(titleMinimumSize.width(), blockMinimumSize.width()));
        finalMin.setHeight(titleMinimumSize.height() + blockMinimumSize.height());

        m_frameWidget->SetResizableMinimum(finalMin);
    }

    void NodeGroupFrameComponent::OnPositionChanged(const AZ::EntityId& /*entityId*/, const AZ::Vector2& position)
    {
        const AZ::EntityId* sourceId = GeometryNotificationBus::GetCurrentBusId();

        if (sourceId && (*sourceId) == GetEntityId())
        {
            if (m_frameWidget->m_allowMovement)
            {
                SetupElementsForMove();

                if (!m_movingElements.empty())
                {
                    QScopedValueRollback<bool> valueRollback(m_ignoreSubSlementPostionChanged, true);

                    AZ::Vector2 delta = position - m_previousPosition;

                    if (!delta.IsZero())
                    {
                        for (const AZ::EntityId& element : m_movingElements)
                        {
                            // Rout the position change through the graphics item to deal with animation
                            RootGraphicsItemRequestBus::Event(element, &RootGraphicsItemRequests::OffsetBy, delta);
                        }
                    }
                }
            }

            m_previousPosition = position;
        }
        else if (!m_ignoreSubSlementPostionChanged)
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void NodeGroupFrameComponent::OnBoundsChanged()
    {
        const AZ::EntityId* sourceId = GeometryNotificationBus::GetCurrentBusId();

        if (sourceId)
        {
            if ((*sourceId) != GetEntityId())
            {
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }
    }

    void NodeGroupFrameComponent::WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
    {
        NodeGroupFrameComponentSaveData* saveData = saveDataContainer.FindCreateSaveData<NodeGroupFrameComponentSaveData>();

        if (saveData)
        {
            (*saveData) = m_saveData;
        }
    }

    void NodeGroupFrameComponent::ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
    {
        NodeGroupFrameComponentSaveData* saveData = saveDataContainer.FindSaveDataAs<NodeGroupFrameComponentSaveData>();

        if (saveData)
        {
            m_saveData = (*saveData);
        }
    }

    AZ::EntityId NodeGroupFrameComponent::GetBookmarkId() const
    {
        return GetEntityId();
    }

    void NodeGroupFrameComponent::RemoveBookmark()
    {
        m_saveData.m_enableAsBookmark = false;

        OnBookmarkStatusChanged();

        SignalDirty();
    }

    int NodeGroupFrameComponent::GetShortcut() const
    {
        return m_saveData.m_shortcut;
    }

    void NodeGroupFrameComponent::SetShortcut(int shortcut)
    {
        m_saveData.m_shortcut = shortcut;
        SignalDirty();
    }

    AZStd::string NodeGroupFrameComponent::GetBookmarkName() const
    {
        AZStd::string comment;
        CommentRequestBus::EventResult(comment, GetEntityId(), &CommentRequests::GetComment);

        return comment;
    }

    void NodeGroupFrameComponent::SetBookmarkName(const AZStd::string& bookmarkName)
    {
        CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetComment, bookmarkName);
    }

    QRectF NodeGroupFrameComponent::GetBookmarkTarget() const
    {
        if (m_saveData.m_isCollapsed && m_collapsedNodeId.IsValid())
        {
            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, m_collapsedNodeId, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem)
            {
                return graphicsItem->sceneBoundingRect();
            }
        }

        return m_frameWidget->sceneBoundingRect();
    }

    QColor NodeGroupFrameComponent::GetBookmarkColor() const
    {
        AZ::Color backgroundColor;
        CommentRequestBus::EventResult(backgroundColor, GetEntityId(), &CommentRequests::GetBackgroundColor);

        return GraphCanvas::ConversionUtils::AZToQColor(backgroundColor);
    }

    void NodeGroupFrameComponent::OnBookmarkTriggered()
    {
        static const float k_gridSteps = 5.0f;
        AZ::EntityId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        AZ::EntityId gridId;
        SceneRequestBus::EventResult(gridId, graphId, &SceneRequests::GetGrid);

        AZ::Vector2 minorPitch(0,0);
        GridRequestBus::EventResult(minorPitch, gridId, &GridRequests::GetMinorPitch);

        QRectF target = GetBookmarkTarget();

        AnimatedPulseConfiguration pulseConfiguration;
        pulseConfiguration.m_enableGradient = true;
        pulseConfiguration.m_drawColor = GetBookmarkColor();
        pulseConfiguration.m_durationSec = 1.0f;
        pulseConfiguration.m_zValue = m_frameWidget->zValue() - 1;

        for (QPointF currentPoint : { target.topLeft(), target.topRight(), target.bottomRight(), target.bottomLeft()} )
        {
            QPointF directionVector = currentPoint - target.center();

            directionVector = QtVectorMath::Normalize(directionVector);

            QPointF finalPoint(currentPoint.x() + directionVector.x() * minorPitch.GetX() * k_gridSteps, currentPoint.y() + directionVector.y() * minorPitch.GetY() * k_gridSteps);

            pulseConfiguration.m_controlPoints.emplace_back(currentPoint, finalPoint);
        }

        SceneRequestBus::Event(graphId, &SceneRequests::CreatePulse, pulseConfiguration);
    }

    void NodeGroupFrameComponent::OnCommentChanged(const AZStd::string&)
    {
        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkNameChanged);
    }

    void NodeGroupFrameComponent::OnBackgroundColorChanged(const AZ::Color& color)
    {
        m_titleWidget->SetColor(color);
        m_blockWidget->SetColor(color);

        BookmarkNotificationBus::Event(GetEntityId(), &BookmarkNotifications::OnBookmarkColorChanged);
    }

    void NodeGroupFrameComponent::OnSceneMemberDragBegin()
    {
        m_ignoreSubSlementPostionChanged = true;

        // Order of operations. The selected element begins to move, before this signal happens[This signal happens in response to it being moved]. So if we get in here, disconnect from the bus in case
        // something already queued up a movement.
        AZ::SystemTickBus::Handler::BusDisconnect();

        if (m_frameWidget->IsSelected())
        {
            SetupElementsForMove();
            EnableGroupedDisplayState(true);
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberDragComplete()
    {
        m_ignoreSubSlementPostionChanged = false;

        m_movingElements.clear();
        EnableGroupedDisplayState(false);
    }

    void NodeGroupFrameComponent::OnDragSelectStart()
    {
        m_frameWidget->SetUseTitleShape(true);

        // Work around for when the drag selection starts inside of the Node Group.
        m_frameWidget->setSelected(false);
    }

    void NodeGroupFrameComponent::OnDragSelectEnd()
    {
        m_frameWidget->SetUseTitleShape(false);
    }

    void NodeGroupFrameComponent::OnNodeRemoved(const AZ::EntityId& nodeId)
    {
        OnSceneMemberRemoved(nodeId);
    }

    void NodeGroupFrameComponent::OnSceneMemberRemoved(const AZ::EntityId& sceneMemberId)
    {
        auto groupElementIter = m_groupedElements.find(sceneMemberId);
        if (groupElementIter != m_groupedElements.end())
        {
            m_groupedElements.erase(groupElementIter);
            
            OnElementUngrouped(sceneMemberId);
            UpdateSavedElements();
        }
    }

    void NodeGroupFrameComponent::OnEntitiesDeserializationComplete(const GraphSerialization&)
    {
        RestoreCollapsedState();
    }

    void NodeGroupFrameComponent::OnGraphLoadComplete()
    {
        // Version Conversion for background color
        if (!m_saveData.m_color.IsZero())
        {
            CommentRequestBus::Event(GetEntityId(), &CommentRequests::SetBackgroundColor, m_saveData.m_color);
            m_saveData.m_color = AZ::Color::CreateZero();
        }
        ////

        ////
        // Version Conversion for free floating grouping to persistent ownership
        // Kind of janky check to see if we don't have anything persisted in our save, then do a spot check to see if we need to persist somethign.
        if (!m_saveData.m_isNewGroup)
        {
            m_saveData.m_isNewGroup = true;

            m_groupedElements.clear();

            // Adjust the size before calling the interior information.
            m_frameWidget->adjustSize();

            FindInteriorElements(m_groupedElements);
            UpdateSavedElements();
        }
        ////

        RemapGroupedPersistentIds();        
    }

    void NodeGroupFrameComponent::PostOnGraphLoadComplete()
    {
        RestoreCollapsedState();
    }

    void NodeGroupFrameComponent::OnPersistentIdsRemapped(const AZStd::unordered_map<PersistentGraphMemberId, PersistentGraphMemberId>& persistentIdRemapping)
    {
        AZStd::vector< PersistentGraphMemberId > oldPersistentIds = AZStd::move(m_saveData.m_persistentGroupedIds);

        m_saveData.m_persistentGroupedIds.clear();
        m_saveData.m_persistentGroupedIds.reserve(oldPersistentIds.size());

        for (PersistentGraphMemberId oldPersistentId : oldPersistentIds)
        {
            auto remappedIter = persistentIdRemapping.find(oldPersistentId);

            if (remappedIter != persistentIdRemapping.end())
            {
                m_saveData.m_persistentGroupedIds.emplace_back(remappedIter->second);
            }
        }

        RemapGroupedPersistentIds();

        PersistentIdNotificationBus::Handler::BusDisconnect();
    }

    void NodeGroupFrameComponent::OnSystemTick()
    {
        // 1 or 0 indicates whether to align to that direction or not.
        const int adjustVertical = 1;
        const int adjustHorizontal = 1;

        // If we have something animating in us, we can adjust ourselves down.
        // Otherwise, we want to only grow in response to elements.
        m_frameWidget->ResizeToGroup(adjustHorizontal, adjustVertical, m_animatingElements.empty());

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void NodeGroupFrameComponent::OnPositionAnimateBegin()
    {
        const AZ::EntityId* sourceId = VisualNotificationBus::GetCurrentBusId();

        if (sourceId)
        {
            if ((*sourceId) == GetEntityId())
            {
                m_isGroupAnimating = true;

                m_movingElements.clear();
                SetupElementsForMove();

                for (const auto& element : m_movingElements)
                {
                    RootGraphicsItemRequestBus::Event(element, &RootGraphicsItemRequests::SignalGroupAnimationStart, GetEntityId());
                }
            }

            m_animatingElements.insert((*sourceId));
            m_frameWidget->SetSnapToGridEnabled(false);
        }
    }

    void NodeGroupFrameComponent::OnPositionAnimateEnd()
    {
        const AZ::EntityId* sourceId = VisualNotificationBus::GetCurrentBusId();

        if (sourceId)
        {
            if ((*sourceId) == GetEntityId())
            {
                for (const auto& element : m_movingElements)
                {
                    RootGraphicsItemRequestBus::Event(element, &RootGraphicsItemRequests::SignalGroupAnimationEnd, GetEntityId());
                }

                m_isGroupAnimating = false;
                m_movingElements.clear();
            }

            m_animatingElements.erase((*sourceId));

            if (m_animatingElements.empty())
            {
                m_frameWidget->SetSnapToGridEnabled(true);
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }
    }

    void NodeGroupFrameComponent::OnDisplayStateChanged(RootGraphicsItemDisplayState /*oldState*/, RootGraphicsItemDisplayState newState)
    {
        if (!m_ignoreDisplayStateChanges)
        {
            if (newState == RootGraphicsItemDisplayState::GroupHighlight
                || newState == RootGraphicsItemDisplayState::Inspection)
            {
                m_needsDisplayStateHighlight = true;
            }
            else
            {
                m_needsDisplayStateHighlight = false;
            }

            UpdateHighlightState();
        }
    }

    void NodeGroupFrameComponent::OnExpansionComplete()
    {
        CollapsedNodeGroupNotificationBus::Handler::BusDisconnect();

        m_collapsedNodeId.SetInvalid();
        SignalExpanded();
    }

    void NodeGroupFrameComponent::OnFrameResizeStart()
    {
        m_ignoreDisplayStateChanges = true;
        SetupHighlightElementsStateSetters();
        m_highlightDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);

        m_ignoreElementsOnResize.clear();

        AZ::EntityId groupId;
        GroupableSceneMemberRequestBus::EventResult(groupId, GetEntityId(), &GroupableSceneMemberRequests::GetGroupId);

        while (groupId.IsValid())
        {
            m_ignoreElementsOnResize.insert(groupId);

            AZ::EntityId parentGroup;
            GroupableSceneMemberRequestBus::EventResult(parentGroup, groupId, &GroupableSceneMemberRequests::GetGroupId);

            if (parentGroup == groupId)
            {
                break;
            }

            groupId = parentGroup;
        }
    }

    void NodeGroupFrameComponent::OnFrameResized()
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        QRectF blockArea = GetGroupBoundary();
        
        AZStd::unordered_set< AZ::EntityId > removedElements;
        AZStd::unordered_set< AZ::EntityId > resizedGroupElements;

        FindInteriorElements(resizedGroupElements, Qt::ItemSelectionMode::IntersectsItemShape);

        for (auto currentElement : resizedGroupElements)
        {
            if (m_ignoreElementsOnResize.find(currentElement) != m_ignoreElementsOnResize.end())
            {
                continue;
            }

            bool isVisible = false;
            VisualRequestBus::EventResult(isVisible, currentElement, &VisualRequests::IsVisible);

            if (!isVisible)
            {
                continue;
            }

            if (GraphUtils::IsGroupableElement(currentElement))
            {
                AZ::EntityId groupId;
                GroupableSceneMemberRequestBus::EventResult(groupId, currentElement, &GroupableSceneMemberRequests::GetGroupId);

                // Anything in a group will be added into this when the group is absorbed
                if (!groupId.IsValid()
                    || groupId == GetEntityId())
                {
                    QGraphicsItem* graphicsItem = nullptr;
                    SceneMemberUIRequestBus::EventResult(graphicsItem, currentElement, &SceneMemberUIRequests::GetRootGraphicsItem);

                    if (graphicsItem)
                    {
                        QRectF boundingRect = graphicsItem->sceneBoundingRect();

                        QRectF intersected = boundingRect.intersected(blockArea);

                        if (intersected.height() > boundingRect.height() * 0.5f
                            && intersected.width() > boundingRect.width() * 0.5f)
                        {
                            if (groupId != GetEntityId())
                            {
                                StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
                                RootGraphicsItemRequestBus::EventResult(stateController, currentElement, &RootGraphicsItemRequests::GetDisplayStateStateController);

                                m_highlightDisplayStateStateSetter.AddStateController(stateController);

                                m_groupedElements.insert(currentElement);
                                OnElementGrouped(currentElement);
                            }
                        }
                        else if (m_groupedElements.find(currentElement) != m_groupedElements.end())
                        {
                            removedElements.insert(currentElement);
                        }
                    }
                }
            }
        }

        // Go over everything that might have been completely out of sized.
        for (auto groupedElement : m_groupedElements)
        {
            if (resizedGroupElements.find(groupedElement) == resizedGroupElements.end())
            {
                removedElements.insert(groupedElement);
            }
        }

        for (auto removedElement : removedElements)
        {
            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
            RootGraphicsItemRequestBus::EventResult(stateController, removedElement, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_highlightDisplayStateStateSetter.RemoveStateController(stateController);

            m_groupedElements.erase(removedElement);

            OnElementUngrouped(removedElement);
        }
    }

    void NodeGroupFrameComponent::OnFrameResizeEnd()
    {
        // Sanitize our group elements from our display.
        UpdateSavedElements();

        const bool growGroupOnly = true;
        ResizeGroupToElements(growGroupOnly);

        m_ignoreElementsOnResize.clear();

        m_ignoreDisplayStateChanges = false;
    }

    EditorId NodeGroupFrameComponent::GetEditorId() const
    {
        return m_editorId;
    }

    void NodeGroupFrameComponent::RestoreCollapsedState()
    {
        if (m_saveData.m_isCollapsed)
        {
            m_frameWidget->adjustSize();

            if (m_groupedElements.empty())
            {
                RemapGroupedPersistentIds();
            }

            bool canCollapseNode = true;

            // Need to restore our collapsed states inward out(so any group that is contained within another group needs to collapse before I can collapse the owning group).
            for (const AZ::EntityId& groupedElement : m_groupedElements)
            {
                if (GraphUtils::IsNodeGroup(groupedElement))
                {
                    bool isCollapsed = false;
                    NodeGroupRequestBus::EventResult(isCollapsed, groupedElement, &NodeGroupRequests::IsCollapsed);

                    if (isCollapsed)
                    {
                        NodeId collapsedNodeId;
                        NodeGroupRequestBus::EventResult(collapsedNodeId, groupedElement, &NodeGroupRequests::GetCollapsedNodeId);

                        if (!collapsedNodeId.IsValid())
                        {
                            m_initializingGroups.insert(groupedElement);
                            canCollapseNode = false;
                            break;
                        }
                        else
                        {
                            m_groupedElements.insert(collapsedNodeId);
                            OnElementGrouped(collapsedNodeId);
                        }
                    }
                }
            }

            if (canCollapseNode)
            {
                QScopedValueRollback<bool> manipulationBlocker(m_enableSelectionManipulation, false);
                CollapseGroup();
            }
        }
    }

    void NodeGroupFrameComponent::TryAndRestoreCollapsedState()
    {
        if (m_saveData.m_isCollapsed)
        {
            bool canCollapse = true;

            for (const AZ::EntityId& internalGroup : m_groupedGrouped)
            {
                NodeGroupRequests* nodeGroupRequests = NodeGroupRequestBus::FindFirstHandler(internalGroup);

                if (nodeGroupRequests)
                {
                    if (nodeGroupRequests->IsCollapsed())
                    {
                        if (!nodeGroupRequests->GetCollapsedNodeId().IsValid())
                        {
                            canCollapse = false;
                            break;
                        }
                    }
                }
            }

            if (canCollapse)
            {
                RestoreCollapsedState();
            }
        }
    }

    void NodeGroupFrameComponent::FindInteriorElements(AZStd::unordered_set< AZ::EntityId >& interiorElements, Qt::ItemSelectionMode selectionMode)
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        QRectF blockArea = GetGroupBoundary();

        AZStd::vector< AZ::EntityId > elementList;
        SceneRequestBus::EventResult(elementList, sceneId, &SceneRequests::GetEntitiesInRect, blockArea, selectionMode);

        interiorElements.clear();

        for (const AZ::EntityId& testElement : elementList)
        {
            if (GraphUtils::IsConnection(testElement) || testElement == GetEntityId())
            {
                continue;
            }

            GroupableSceneMemberRequests* groupableRequests = GroupableSceneMemberRequestBus::FindFirstHandler(testElement);

            if (groupableRequests)
            {
                bool isVisible = true;

                VisualRequestBus::EventResult(isVisible, testElement, &VisualRequests::IsVisible);

                if (isVisible)
                {
                    interiorElements.insert(testElement);
                }
            }
        }
    }

    float NodeGroupFrameComponent::SetDisplayHeight(float height)
    {
        if (m_frameWidget && m_frameWidget->m_minimumSize.height() > height)
        {
            height = aznumeric_cast<float>(m_frameWidget->m_minimumSize.height());
        }

        m_saveData.m_displayHeight = height;
        SignalDirty();

        return height;
    }

    float NodeGroupFrameComponent::SetDisplayWidth(float width)
    {
        if (m_frameWidget && m_frameWidget->m_minimumSize.width() > width)
        {
            width = aznumeric_cast<float>(m_frameWidget->m_minimumSize.width());
        }

        m_saveData.m_displayWidth = width;
        SignalDirty();

        return width;
    }

    void NodeGroupFrameComponent::EnableInteriorHighlight(bool highlight)
    {
        m_needsManualHighlight = highlight;

        UpdateHighlightState();
    }

    void NodeGroupFrameComponent::EnableGroupedDisplayState(bool enabled)
    {
        m_forcedGroupDisplayStateStateSetter.ResetStateSetter();

        if (enabled)
        {
            SetupGroupedElementsStateSetters();

            m_forcedGroupDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
        }
    }

    void NodeGroupFrameComponent::OnBookmarkStatusChanged()
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        if (m_saveData.m_enableAsBookmark)
        {
            BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::RegisterBookmark, GetEntityId());
            SceneBookmarkRequestBus::Handler::BusConnect(sceneId);
        }
        else
        {
            bool isRegistered = false;
            BookmarkManagerRequestBus::EventResult(isRegistered, sceneId, &BookmarkManagerRequests::IsBookmarkRegistered, GetEntityId());

            if (isRegistered)
            {
                BookmarkManagerRequestBus::Event(sceneId, &BookmarkManagerRequests::UnregisterBookmark, GetEntityId());
            }

            m_saveData.m_shortcut = k_findShortcut;

            SceneBookmarkRequestBus::Handler::BusDisconnect();
        }
    }

    void NodeGroupFrameComponent::UpdateSavedElements()
    {
        if (!m_saveData.m_isCollapsed)
        {
            m_saveData.m_persistentGroupedIds.clear();

            for (AZ::EntityId groupedMemberId : m_groupedElements)
            {
                if (GraphUtils::IsCollapsedNodeGroup(groupedMemberId))
                {
                    continue;
                }

                PersistentGraphMemberId graphMemberId = PersistentGraphMemberId::CreateNull();
                PersistentMemberRequestBus::EventResult(graphMemberId, groupedMemberId, &PersistentMemberRequests::GetPersistentGraphMemberId);

                if (!graphMemberId.IsNull())
                {
                    m_saveData.m_persistentGroupedIds.emplace_back(graphMemberId);
                }
            }

            SignalDirty();
        }
    }

    void NodeGroupFrameComponent::RemapGroupedPersistentIds()
    {
        for (const AZ::EntityId& groupedElement : m_groupedElements)
        {
            OnElementUngrouped(groupedElement);
        }

        m_groupedElements.clear();

        for (const PersistentGraphMemberId& persistentMemberId : m_saveData.m_persistentGroupedIds)
        {
            AZ::EntityId graphMemberId;
            PersistentIdRequestBus::EventResult(graphMemberId, persistentMemberId, &PersistentIdRequests::MapToEntityId);

            if (graphMemberId.IsValid())
            {
                m_groupedElements.insert(graphMemberId);
                OnElementGrouped(graphMemberId);

                if (GraphUtils::IsNodeGroup(graphMemberId))
                {
                    bool isCollapsed = false;
                    NodeGroupRequestBus::EventResult(isCollapsed, graphMemberId, &NodeGroupRequests::IsCollapsed);

                    if (isCollapsed)
                    {
                        NodeId collapsedNodeId;
                        NodeGroupRequestBus::EventResult(collapsedNodeId, graphMemberId, &NodeGroupRequests::GetCollapsedNodeId);

                        if (collapsedNodeId.IsValid())
                        {
                            m_groupedElements.insert(collapsedNodeId);
                            OnElementGrouped(collapsedNodeId);
                        }
                    }
                }
            }
        }
    }

    bool NodeGroupFrameComponent::AddToGroupInternal(const AZ::EntityId& groupableElement)
    {
        if (GraphUtils::IsGroupableElement(groupableElement) && groupableElement != GetEntityId())
        {
            auto insertResult = m_groupedElements.insert(groupableElement);

            if (insertResult.second)
            {
                OnElementGrouped(groupableElement);
                return true;
            }
        }

        return false;
    }

    void NodeGroupFrameComponent::UpdateHighlightState()
    {
        bool isHighlighted = m_highlightDisplayStateStateSetter.HasState();
        bool shouldHighlight = m_needsDisplayStateHighlight || m_needsManualHighlight;

        if (isHighlighted != shouldHighlight)
        {
            if (shouldHighlight)
            {
                SetupHighlightElementsStateSetters();
                m_highlightDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
            }
            else
            {
                m_highlightDisplayStateStateSetter.ResetStateSetter();
            }
        }
        
    }

    void NodeGroupFrameComponent::SetupHighlightElementsStateSetters()
    {
        AZStd::vector< AZ::EntityId > highlightEntities;
        FindGroupedElements(highlightEntities);

        for (const AZ::EntityId& entityId : highlightEntities)
        {
            StateController<RootGraphicsItemDisplayState>* stateController = nullptr;
            RootGraphicsItemRequestBus::EventResult(stateController, entityId, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_highlightDisplayStateStateSetter.AddStateController(stateController);
        }
    }

    void NodeGroupFrameComponent::SetupGroupedElementsStateSetters()
    {
        AZStd::vector< AZ::EntityId > groupedElements;
        FindGroupedElements(groupedElements);

        groupedElements.push_back(GetEntityId());

        SubGraphParsingConfig config;
        config.m_createNonConnectableSubGraph = true;

        SubGraphParsingResult subGraphResult = GraphUtils::ParseSceneMembersIntoSubGraphs(groupedElements, config);

        SetupSubGraphGroupedElementsStateSetters(subGraphResult.m_nonConnectableGraph);

        for (const GraphSubGraph& subGraph : subGraphResult.m_subGraphs)
        {
            SetupSubGraphGroupedElementsStateSetters(subGraph);
        }
    }

    void NodeGroupFrameComponent::SetupSubGraphGroupedElementsStateSetters(const GraphSubGraph& subGraph)
    {
        for (const AZ::EntityId& elementId : subGraph.m_containedNodes)
        {
            if (elementId == m_collapsedNodeId)
            {
                continue;
            }

            StateController<RootGraphicsItemDisplayState>* displayStateController = nullptr;
            RootGraphicsItemRequestBus::EventResult(displayStateController, elementId, &RootGraphicsItemRequests::GetDisplayStateStateController);

            m_forcedGroupDisplayStateStateSetter.AddStateController(displayStateController);
        }
    }

    void NodeGroupFrameComponent::OnElementGrouped(const AZ::EntityId& groupableElement)
    {
        GroupableSceneMemberRequestBus::Event(groupableElement, &GroupableSceneMemberRequests::RegisterToGroup, GetEntityId());

        GeometryNotificationBus::MultiHandler::BusConnect(groupableElement);
        VisualNotificationBus::MultiHandler::BusConnect(groupableElement);

        if (GraphUtils::IsNodeGroup(groupableElement))
        {
            m_groupedGrouped.insert(groupableElement);
            NodeGroupNotificationBus::MultiHandler::BusConnect(groupableElement);
        }

        if (GraphUtils::IsCollapsedNodeGroup(groupableElement))
        {
            AZ::EntityId groupId;
            CollapsedNodeGroupRequestBus::EventResult(groupId, groupableElement, &CollapsedNodeGroupRequests::GetSourceGroup);

            if (groupId.IsValid())
            {
                m_collapsedGroupMapping[groupId] = groupableElement;

                if (m_groupedElements.find(groupId) == m_groupedElements.end())
                {
                    m_groupedElements.insert(groupId);
                    OnElementGrouped(groupId);
                }
            }
        }
    }

    void NodeGroupFrameComponent::OnElementUngrouped(const AZ::EntityId& groupableElement)
    {
        GroupableSceneMemberRequestBus::Event(groupableElement, &GroupableSceneMemberRequests::UnregisterFromGroup, GetEntityId());

        GeometryNotificationBus::MultiHandler::BusDisconnect(groupableElement);
        VisualNotificationBus::MultiHandler::BusDisconnect(groupableElement);

        size_t eraseCount = m_groupedGrouped.erase(groupableElement);

        if (eraseCount > 0)
        {
            NodeGroupNotificationBus::MultiHandler::BusDisconnect(groupableElement);
        }

        if (GraphUtils::IsCollapsedNodeGroup(groupableElement))
        {
            AZ::EntityId groupId;
            CollapsedNodeGroupRequestBus::EventResult(groupId, groupableElement, &CollapsedNodeGroupRequests::GetSourceGroup);

            if (groupId.IsValid())
            {
                // If we don't erase anything from this map, that means we are coming from the ExpandedSignal 
                size_t eraseCount2 = m_collapsedGroupMapping.erase(groupId);

                if (eraseCount2 > 0)
                {
                    m_groupedElements.erase(groupId);
                    OnElementUngrouped(groupId);
                }
            }
        }
    }

    void NodeGroupFrameComponent::SignalExpanded()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        m_saveData.m_isCollapsed = false;
        SignalDirty();

        NodeGroupNotificationBus::Event(GetEntityId(), &NodeGroupNotifications::OnExpanded);

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

        if (m_enableSelectionManipulation)
        {
            m_frameWidget->SetSelected(true);
        }
    }

    void NodeGroupFrameComponent::SetupElementsForMove()
    {
        if (m_movingElements.empty())
        {
            AZ_Warning("Graph Canvas", m_movingElements.empty(), "Moving elements should be empty when scraping for new elements.");

            m_movingElements = m_groupedElements;

            if (!m_isGroupAnimating)
            {
                for (const AZ::EntityId& currentElement : m_groupedElements)
                {
                    // We don't want to move anything that is selected, since in the drag move
                    // Qt will handle moving it already, so we don't want to double move it.
                    bool isSelected = false;
                    SceneMemberUIRequestBus::EventResult(isSelected, currentElement, &SceneMemberUIRequests::IsSelected);

                    if (isSelected)
                    {
                        m_movingElements.erase(currentElement);
                    }
                }
            }

            // Go through and erase any group ids that are subsumed by a collapsed node id.
            for (const auto& mappingPair : m_collapsedGroupMapping)
            {
                m_movingElements.erase(mappingPair.first);
            }
        }
    }

    void NodeGroupFrameComponent::SignalDirty()
    {
        if (!m_isGroupAnimating)
        {
            m_saveData.SignalDirty();
        }
    }

    QRectF NodeGroupFrameComponent::GetGroupBoundary() const
    {
        QRectF blockArea = m_frameWidget->sceneBoundingRect();

        // Need to convert this to our previous position
        // just incase we are moving.
        //
        // If we aren't moving, this will be the same, so just a little slower then necessary.
        blockArea.setX(m_previousPosition.GetX());
        blockArea.setY(m_previousPosition.GetY());

        blockArea.setWidth(m_frameWidget->RoundToClosestStep(aznumeric_cast<int>(blockArea.width()), m_frameWidget->GetGridXStep()));
        blockArea.setHeight(m_frameWidget->RoundToClosestStep(aznumeric_cast<int>(blockArea.height()), m_frameWidget->GetGridYStep()));

        // Want to adjust everything by half a step in each direction to get the elements that are directly on the edge of the frame widget
        // without grabbing the elements that are a single step off the edge.
        qreal adjustStepX = m_frameWidget->GetGridXStep() * 0.5f;
        qreal adjustStepY = m_frameWidget->GetGridYStep() * 0.5f;

        blockArea.adjust(-adjustStepX, -adjustStepY, adjustStepX, adjustStepY);

        return blockArea;
    }

    //////////////////////////////
    // NodeGroupFrameTitleWidget
    //////////////////////////////

    NodeGroupFrameTitleWidget::NodeGroupFrameTitleWidget()
        : m_frameWidget(nullptr)
    {
        setAcceptHoverEvents(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void NodeGroupFrameTitleWidget::RefreshStyle(const AZ::EntityId& parentId)
    {
        m_styleHelper.SetStyle(parentId, Styling::Elements::NodeGroup::Title);
        update();
    }

    void NodeGroupFrameTitleWidget::SetColor(const AZ::Color& color)
    {
        m_color = ConversionUtils::AZToQColor(color);
        update();
    }

    void NodeGroupFrameTitleWidget::RegisterFrame(NodeGroupFrameGraphicsWidget* frameWidget)
    {
        m_frameWidget = frameWidget;
    }

    void NodeGroupFrameTitleWidget::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_frameWidget)
        {
            if (m_frameWidget->m_adjustVertical != 0
                || m_frameWidget->m_adjustHorizontal != 0)
            {
                mouseEvent->setAccepted(false);
                return;
            }
        }

        QGraphicsWidget::mousePressEvent(mouseEvent);
    }

    void NodeGroupFrameTitleWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        QPen border = m_styleHelper.GetBorder();
        border.setColor(m_color);

        QBrush alphaBackground = m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor);

        QColor backgroundColor = m_color;
        backgroundColor.setAlpha(alphaBackground.color().alpha());

        QBrush background = QBrush(backgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 5.0);

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);
            
            QRectF bounds = boundingRect();

            // Ensure the bounds are large enough to draw the full radius
            // Even in our smaller section
            if (bounds.height() < 2.0 * cornerRadius)
            {
                bounds.setHeight(2.0 * cornerRadius);
            }

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjustedBounds = bounds.marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            painter->save();
            painter->setClipRect(bounds);

            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            // Moving the bottom bounds off the bottom, so we can't see them(mostly to avoid double drawing over the same region)
            adjustedBounds.setHeight(adjustedBounds.height() + border.widthF() + cornerRadius);

            // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
            path.addRoundedRect(adjustedBounds, cornerRadius - 1.0, cornerRadius - 1.0);

            painter->fillPath(path, background);
            painter->drawPath(path.simplified());

            painter->restore();
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    QVariant NodeGroupFrameTitleWidget::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        if (m_frameWidget)
        {
            VisualNotificationBus::Event(m_frameWidget->GetEntityId(), &VisualNotifications::OnItemChange, m_frameWidget->GetEntityId(), change, value);
        }

        return QGraphicsWidget::itemChange(change, value);
    }

    //////////////////////////////////
    // NodeGroupFrameBlockAreaWidget
    //////////////////////////////////

    NodeGroupFrameBlockAreaWidget::NodeGroupFrameBlockAreaWidget()
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void NodeGroupFrameBlockAreaWidget::RegisterFrame(NodeGroupFrameGraphicsWidget* frame)
    {
        m_frameWidget = frame;
    }

    void NodeGroupFrameBlockAreaWidget::RefreshStyle(const AZ::EntityId& parentId)
    {
        m_styleHelper.SetStyle(parentId, Styling::Elements::NodeGroup::BlockArea);
        update();
    }

    void NodeGroupFrameBlockAreaWidget::SetColor(const AZ::Color& color)
    {
        m_color = ConversionUtils::AZToQColor(color);
        update();
    }

    void NodeGroupFrameBlockAreaWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QPen border = m_styleHelper.GetBorder();

        border.setColor(m_color);

        QBrush alphaBackground = m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor);

        QColor backgroundColor = m_color;
        backgroundColor.setAlpha(alphaBackground.color().alpha());

        QBrush background = QBrush(backgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = m_styleHelper.GetAttribute(Styling::Attribute::BorderRadius, 5.0);

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);

            QRectF bounds = boundingRect();

            // Ensure the bounds are large enough to draw the full radius
            // Even in our smaller section
            if (bounds.height() < 2.0 * cornerRadius)
            {
                bounds.setHeight(2.0 * cornerRadius);
            }

            painter->save();
            painter->setClipRect(bounds);

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjustedBounds = bounds.marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            // Moving the tops bounds off the top, so we can't see them(mostly to avoid double drawing over the same region)
            adjustedBounds.setY(adjustedBounds.y() - AZ::GetMax(border.widthF(), cornerRadius));

            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
            path.addRoundedRect(adjustedBounds, cornerRadius - 1.0, cornerRadius - 1.0);

            painter->fillPath(path, background);
            painter->drawPath(path.simplified());

            int numLines = 3;

            border.setWidth(1);
            painter->setPen(border);

            qreal halfPenWidth = border.width() * 0.5f;
            qreal spacing = 3;
            qreal initialSpacing = 0;

            QPointF bottomPoint = bounds.bottomRight();

            QPointF offsetPointHorizontal = bottomPoint;
            offsetPointHorizontal.setX(offsetPointHorizontal.x() - (initialSpacing));

            QPointF offsetPointVertical = bottomPoint;
            offsetPointVertical.setY(offsetPointVertical.y() - (initialSpacing));

            for (int i = 0; i < numLines; ++i)
            {
                offsetPointHorizontal.setX(offsetPointHorizontal.x() - (spacing + halfPenWidth));
                offsetPointVertical.setY(offsetPointVertical.y() - (spacing + halfPenWidth));

                painter->drawLine(offsetPointHorizontal, offsetPointVertical);

                offsetPointHorizontal.setX(offsetPointHorizontal.x() - (halfPenWidth));
                offsetPointVertical.setY(offsetPointVertical.y() - (halfPenWidth));
            }

            painter->restore();
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    /////////////////////////////////
    // NodeGroupFrameGraphicsWidget
    /////////////////////////////////

    NodeGroupFrameGraphicsWidget::NodeGroupFrameGraphicsWidget(const AZ::EntityId& entityKey, NodeGroupFrameComponent& nodeFrameComponent)
        : NodeFrameGraphicsWidget(entityKey)        
        , m_nodeFrameComponent(nodeFrameComponent)
        , m_useTitleShape(false)
        , m_allowCommentReaction(false)
        , m_allowMovement(false)
        , m_resizeComment(false)
        , m_allowDraw(true)
        , m_adjustVertical(0)
        , m_adjustHorizontal(0)
        , m_overTitleWidget(false)
        , m_isSelected(false)
        , m_enableHighlight(false)
    {
        setAcceptHoverEvents(true);
        setCacheMode(QGraphicsItem::CacheMode::NoCache);
    }

    void NodeGroupFrameGraphicsWidget::RefreshStyle(const AZ::EntityId& styleEntity)
    {
        m_borderStyle.SetStyle(styleEntity, Styling::Elements::NodeGroup::Border);
    }

    void NodeGroupFrameGraphicsWidget::SetResizableMinimum(const QSizeF& minimumSize)
    {
        m_resizableMinimum = minimumSize;

        UpdateMinimumSize();

        // Weird case. The maximum size of this needs to be set.
        // Otherwise the text widget will force it to grow a bit.
        // This gets set naturally when you resize the element, but
        // not when one gets newly created. To catch this,
        // we'll just check if we don't have a reasonable maximum width set
        // and then just set ourselves to the minimum size that is passed in.
        if (maximumWidth() == QWIDGETSIZE_MAX)
        {
            ResizeTo(aznumeric_cast<float>(minimumSize.height()), aznumeric_cast<float>(minimumSize.width()));
        }
    }

    void NodeGroupFrameGraphicsWidget::SetUseTitleShape(bool enable)
    {
        m_useTitleShape = enable;
    }

    void NodeGroupFrameGraphicsWidget::OnActivated()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    QPainterPath NodeGroupFrameGraphicsWidget::GetOutline() const
    {
        QPainterPath path;
        path.addRect(sceneBoundingRect());
        return path;
    }

    void NodeGroupFrameGraphicsWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverEnterEvent(hoverEvent);

        QPointF point = hoverEvent->scenePos();

        UpdateCursor(point);
        m_allowDraw = m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(point); 
        m_overTitleWidget = m_allowDraw;

        UpdateHighlightState();        
    }

    void NodeGroupFrameGraphicsWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverMoveEvent(hoverEvent);

        QPointF point = hoverEvent->scenePos();

        UpdateCursor(point);

        bool allowDraw = m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(point);

        if (allowDraw != m_allowDraw)
        {
            m_overTitleWidget = allowDraw;
            m_allowDraw = allowDraw;
            update();
        }

        UpdateHighlightState();
    }

    void NodeGroupFrameGraphicsWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent)
    {
        NodeFrameGraphicsWidget::hoverLeaveEvent(hoverEvent);
        ResetCursor();

        m_adjustHorizontal = 0;
        m_adjustVertical = 0;

        m_allowDraw = true;
        m_overTitleWidget = false;

        UpdateHighlightState();
    }

    void NodeGroupFrameGraphicsWidget::mousePressEvent(QGraphicsSceneMouseEvent* pressEvent)
    {
        if (m_adjustHorizontal != 0 || m_adjustVertical != 0)
        {
            pressEvent->accept();

            m_allowCommentReaction = true;
            m_resizeComment = true;

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            SceneRequestBus::Event(sceneId, &SceneRequests::ClearSelection);
            SetSelected(true);

            m_nodeFrameComponent.OnFrameResizeStart();
        }
        else if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(pressEvent->scenePos()))
        {
            pressEvent->accept();
            NodeFrameGraphicsWidget::mousePressEvent(pressEvent);
        }
        else
        {
            pressEvent->setAccepted(false);
        }
    }

    void NodeGroupFrameGraphicsWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_resizeComment)
        {
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            mouseEvent->accept();

            QPointF point = mouseEvent->scenePos();
            QPointF anchorPoint = scenePos();

            double halfBorder = (m_style.GetAttribute(Styling::Attribute::BorderWidth, 1.0) * 0.5f);

            QSizeF originalSize = boundingRect().size();

            qreal newWidth = originalSize.width();
            qreal newHeight = originalSize.height();

            if (m_adjustVertical < 0)
            {
                newHeight += anchorPoint.y() - point.y();
            }
            else if (m_adjustVertical > 0)
            {
                newHeight += point.y() - (anchorPoint.y() + boundingRect().height() - halfBorder);
            }

            if (m_adjustHorizontal < 0)
            {
                newWidth += anchorPoint.x() - point.x();
            }
            else if (m_adjustHorizontal > 0)
            {
                newWidth += point.x() - (anchorPoint.x() + boundingRect().width() - halfBorder);
            }

            QSizeF minimumSize = m_style.GetMinimumSize();

            if (newWidth < m_minimumSize.width())
            {
                newWidth = minimumSize.width();
            }

            if (newHeight < m_minimumSize.height())
            {
                newHeight = minimumSize.height();
            }

            if (IsResizedToGrid())
            {
                int width = static_cast<int>(newWidth);
                newWidth = RoundToClosestStep(width, GetGridXStep());

                int height = static_cast<int>(newHeight);
                newHeight = RoundToClosestStep(height, GetGridYStep());
            }

            newWidth = m_nodeFrameComponent.SetDisplayWidth(aznumeric_cast<float>(newWidth));
            newHeight = m_nodeFrameComponent.SetDisplayHeight(aznumeric_cast<float>(newHeight));

            qreal widthDelta = newWidth - originalSize.width();
            qreal heightDelta = newHeight - originalSize.height();

            prepareGeometryChange();

            QPointF reposition(0, 0);

            if (m_adjustHorizontal < 0)
            {
                reposition.setX(-widthDelta);
            }

            if (m_adjustVertical < 0)
            {
                reposition.setY(-heightDelta);
            }

            prepareGeometryChange();
            setPos(scenePos() + reposition);

            setMinimumSize(newWidth, newHeight);
            setPreferredSize(newWidth, newHeight);
            setMaximumSize(newWidth, newHeight);

            adjustSize();
            updateGeometry();
            
            update();

            m_nodeFrameComponent.OnFrameResized();
        }
        else
        {
            NodeFrameGraphicsWidget::mouseMoveEvent(mouseEvent);
        }
    }

    void NodeGroupFrameGraphicsWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* releaseEvent)
    {
        if (m_resizeComment)
        {
            releaseEvent->accept();

            m_resizeComment = false;
            m_allowCommentReaction = false;

            m_nodeFrameComponent.OnFrameResizeEnd();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);            

            GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
        }
        else
        {
            NodeFrameGraphicsWidget::mouseReleaseEvent(releaseEvent);
        }
    }

    bool NodeGroupFrameGraphicsWidget::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::GraphicsSceneResize:
        {
            QGraphicsSceneResizeEvent* resizeEvent = static_cast<QGraphicsSceneResizeEvent*>(event);
            OnCommentSizeChanged(resizeEvent->oldSize(), resizeEvent->newSize());
            break;
        }        
        default:
            break;
        }

        return false;
    }

    void NodeGroupFrameGraphicsWidget::OnEditBegin()
    {
        m_allowCommentReaction = true;
    }

    void NodeGroupFrameGraphicsWidget::OnEditEnd()
    {
        m_allowCommentReaction = false;
    }

    void NodeGroupFrameGraphicsWidget::OnCommentSizeChanged(const QSizeF& oldSize, const QSizeF& newSize)
    {
        if (m_allowCommentReaction)
        {
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            qreal originalHeight = boundingRect().height();
            qreal newHeight = boundingRect().height() + (newSize.height() - oldSize.height());

            if (newHeight < m_minimumSize.height())
            {
                newHeight = m_minimumSize.height();
            }

            qreal heightDelta = newHeight - originalHeight;

            if (IsResizedToGrid())
            {
                // Check if we have enough space to grow down into the block widget without eating into a full square
                // basically use the bit of a fuzzy space where both the header and the block merge.
                //
                // If we can, just expand down, otherwise then we want to grow up a tick.
                qreal frameHeight = m_nodeFrameComponent.m_blockWidget->boundingRect().height();

                if (heightDelta >= 0 && GrowToNextStep(aznumeric_cast<int>(frameHeight - heightDelta), GetGridYStep()) > frameHeight)
                {
                    heightDelta = 0;
                    newHeight = originalHeight;
                }
                else
                {
                    int height = static_cast<int>(newHeight);
                    newHeight = GrowToNextStep(height, GetGridYStep());

                    heightDelta = newHeight - originalHeight;
                }
            }

            QPointF reposition(0, -heightDelta);

            prepareGeometryChange();
            setPos(scenePos() + reposition);
            updateGeometry();

            setMinimumHeight(newHeight);
            setPreferredHeight(newHeight);
            setMaximumHeight(newHeight);

            m_nodeFrameComponent.SetDisplayHeight(aznumeric_cast<float>(newHeight));

            adjustSize();
        }
    }

    void NodeGroupFrameGraphicsWidget::OnCommentFontReloadBegin()
    {
        m_allowCommentReaction = true;
    }

    void NodeGroupFrameGraphicsWidget::OnCommentFontReloadEnd()
    {
        m_allowCommentReaction = false;
    }

    void NodeGroupFrameGraphicsWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent)
    {
        if (m_adjustHorizontal != 0
            || m_adjustVertical != 0)
        {
            mouseEvent->accept();
            ResizeToGroup(m_adjustHorizontal, m_adjustVertical);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
        else
        {
            bool collapseOnDoubleClick = false;
            AssetEditorSettingsRequestBus::EventResult(collapseOnDoubleClick, m_nodeFrameComponent.GetEditorId(), &AssetEditorSettingsRequests::IsGroupDoubleClickCollapseEnabled);

            if (collapseOnDoubleClick)
            {
                if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(mouseEvent->scenePos()))
                {
                    NodeGroupRequestBus::Event(GetEntityId(), &NodeGroupRequests::CollapseGroup);
                }
                else
                {
                    NodeFrameGraphicsWidget::mouseDoubleClickEvent(mouseEvent);

                    mouseEvent->accept();
                    NodeGroupRequestBus::Event(GetEntityId(), &NodeGroupRequests::CollapseGroup);
                }
            }
        }
    }

    void NodeGroupFrameGraphicsWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        if (isSelected() || m_allowDraw)
        {
            QPen border = m_borderStyle.GetBorder();
            painter->setPen(border);
            painter->drawRect(boundingRect());
        }

        QGraphicsWidget::paint(painter, option, widget);
    }

    QPainterPath NodeGroupFrameGraphicsWidget::shape() const
    {
        // We want to use the title shape for determinig things like selection range with a drag select.
        // But we need to use the full shape for things like mouse events.
        if (m_useTitleShape)
        {
            QPainterPath path;
            path.addRect(m_nodeFrameComponent.m_titleWidget->boundingRect());
            return path;
        }
        else
        {
            return NodeFrameGraphicsWidget::shape();
        }
    }

    QVariant NodeGroupFrameGraphicsWidget::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        QVariant retVal = NodeFrameGraphicsWidget::itemChange(change, value);

        if (change == QGraphicsItem::ItemSelectedChange)
        {
            m_isSelected = value.toBool();
            UpdateHighlightState();
        }

        return retVal;
    }

    void NodeGroupFrameGraphicsWidget::OnMemberSetupComplete()
    {
        m_allowMovement = true;
        CommentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NodeGroupFrameGraphicsWidget::ResizeToGroup(int adjustHorizontal, int adjustVertical, bool growOnly)
    {
        QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

        QRectF blockBoundingRect = m_nodeFrameComponent.m_blockWidget->sceneBoundingRect();
        QRectF calculatedBounds;

        // Default grid step to something non-zero so we have some gap

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        AZ::Vector2 gridStep = GraphUtils::FindMinorStep(sceneId);

        QRectF searchBoundingRect = sceneBoundingRect();
        searchBoundingRect.adjust(-gridStep.GetX() * 0.5f, -gridStep.GetY() * 0.5f, gridStep.GetX() * 0.5f, gridStep.GetY() * 0.5f);

        for (AZ::EntityId groupedElement : m_nodeFrameComponent.m_groupedElements)
        {
            // Don't want to resize to connections.
            // And don't want to include ourselves in this calculation
            if (ConnectionRequestBus::FindFirstHandler(groupedElement) != nullptr || groupedElement == GetEntityId())
            {
                continue;
            }

            QGraphicsItem* graphicsItem = nullptr;
            SceneMemberUIRequestBus::EventResult(graphicsItem, groupedElement, &SceneMemberUIRequests::GetRootGraphicsItem);

            if (graphicsItem == nullptr || !graphicsItem->isVisible())
            {
                continue;
            }

            if (calculatedBounds.isEmpty())
            {
                calculatedBounds = graphicsItem->sceneBoundingRect();
            }
            else
            {
                calculatedBounds = calculatedBounds.united(graphicsItem->sceneBoundingRect());
            }
        }

        if (!calculatedBounds.isEmpty())
        {
            // When we are in 'grow' only mode we don't want to add in extra padding, since that will be compounded. So we'll apply that padding once we decide which bound to use.
            if (!growOnly)
            {
                calculatedBounds.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());
            }

            if (adjustHorizontal != 0)
            {
                if (growOnly)
                {
                    int left = static_cast<int>(blockBoundingRect.left());

                    if (left >= calculatedBounds.left())
                    {
                        left = static_cast<int>(calculatedBounds.left() - gridStep.GetX());
                    }

                    int right = static_cast<int>(blockBoundingRect.right());

                    if (right <= calculatedBounds.right())
                    {
                        right = static_cast<int>(calculatedBounds.right() + gridStep.GetX());
                    }
                    
                    blockBoundingRect.setX(left);
                    blockBoundingRect.setWidth(right - left);
                }
                else
                {
                    blockBoundingRect.setX(calculatedBounds.x());
                    blockBoundingRect.setWidth(calculatedBounds.width());
                }
            }

            if (adjustVertical != 0)
            {
                if (growOnly)
                {
                    int top = static_cast<int>(blockBoundingRect.top());

                    if (top >= calculatedBounds.top())
                    {
                        top = static_cast<int>(calculatedBounds.top() - gridStep.GetY());
                    }

                    int bottom = static_cast<int>(blockBoundingRect.bottom());

                    if (bottom <= calculatedBounds.bottom())
                    {
                        bottom = static_cast<int>(calculatedBounds.bottom() + gridStep.GetY());
                    }

                    blockBoundingRect.setY(top);
                    blockBoundingRect.setHeight(bottom - top);
                }
                else
                {
                    blockBoundingRect.setY(calculatedBounds.y());
                    blockBoundingRect.setHeight(calculatedBounds.height());
                }
            }

            m_nodeFrameComponent.SetGroupSize(blockBoundingRect);
        }
    }

    void NodeGroupFrameGraphicsWidget::UpdateHighlightState()
    {
        SetHighlightState(m_overTitleWidget || m_isSelected);
    }

    void NodeGroupFrameGraphicsWidget::SetHighlightState(bool highlightState)
    {
        if (highlightState != m_enableHighlight)
        {
            m_enableHighlight = highlightState;
            m_nodeFrameComponent.EnableInteriorHighlight(m_enableHighlight);
        }
    }

    void NodeGroupFrameGraphicsWidget::ResizeTo(float height, float width)
    {
        prepareGeometryChange();

        if (height >= 0)
        {
            setMinimumHeight(height);
            setPreferredHeight(height);
            setMaximumHeight(height);
        }
        
        if (width >= 0)
        {
            setMinimumWidth(width);
            setPreferredWidth(width);
            setMaximumWidth(width);
        }

        updateGeometry();
    }

    void NodeGroupFrameGraphicsWidget::OnDeactivated()
    {
        CommentNotificationBus::Handler::BusDisconnect();
    }

    void NodeGroupFrameGraphicsWidget::UpdateMinimumSize()
    {
        QSizeF styleMinimum = m_style.GetMinimumSize();

        m_minimumSize.setWidth(AZ::GetMax(styleMinimum.width(), m_resizableMinimum.width()));
        m_minimumSize.setHeight(AZ::GetMax(styleMinimum.height(), m_resizableMinimum.height()));

        if (IsResizedToGrid())
        {
            m_minimumSize.setWidth(GrowToNextStep(static_cast<int>(m_minimumSize.width()), GetGridXStep()));
            m_minimumSize.setHeight(GrowToNextStep(static_cast<int>(m_minimumSize.height()), GetGridYStep()));
        }

        prepareGeometryChange();

        if (minimumHeight() < m_minimumSize.height())
        {
            setMinimumHeight(m_minimumSize.height());
            setPreferredHeight(m_minimumSize.height());
            setMaximumHeight(m_minimumSize.height());
            
            // Fix for a timing hole in the start-up process.
            //
            // Save size is set, but not used. But then the style refreshed, which causes 
            // this to be recalculated which stomps on the save data.
            if (m_nodeFrameComponent.m_saveData.m_displayHeight < m_minimumSize.height())
            {
                m_nodeFrameComponent.SetDisplayHeight(aznumeric_cast<float>(m_minimumSize.height()));
            }
        }

        if (minimumWidth() < m_minimumSize.width())
        {
            setMinimumWidth(m_minimumSize.width());
            setPreferredWidth(m_minimumSize.width());
            setMaximumWidth(m_minimumSize.width());

            // Fix for a timing hole in the start-up process.
            //
            // Save size is set, but not used. But then the style refreshed, which causes 
            // this to be recalculated which stomps on the save data.
            if (m_nodeFrameComponent.m_saveData.m_displayWidth < m_minimumSize.width())
            {
                m_nodeFrameComponent.SetDisplayWidth(aznumeric_cast<float>(m_minimumSize.width()));
            }
        }

        prepareGeometryChange();
        updateGeometry();

        update();
    }

    void NodeGroupFrameGraphicsWidget::ResetCursor()
    {
        setCursor(Qt::ArrowCursor);
    }

    void NodeGroupFrameGraphicsWidget::UpdateCursor(QPointF cursorPoint)
    {
        qreal border = m_style.GetAttribute(Styling::Attribute::BorderWidth, 1.0);
        border = AZStd::GetMax(10.0, border);

        QPointF topLeft = scenePos();
        topLeft.setX(topLeft.x() + border);
        topLeft.setY(topLeft.y() + border);

        QPointF bottomRight = scenePos() + QPointF(boundingRect().width(), boundingRect().height());
        bottomRight.setX(bottomRight.x() - border);
        bottomRight.setY(bottomRight.y() - border);

        m_adjustVertical = 0;
        m_adjustHorizontal = 0;

        if (cursorPoint.x() < topLeft.x())
        {
            m_adjustHorizontal = -1;
        }
        else if (cursorPoint.x() >= bottomRight.x())
        {
            m_adjustHorizontal = 1;
        }

        if (cursorPoint.y() < topLeft.y())
        {
            m_adjustVertical = -1;
        }
        else if (cursorPoint.y() >= bottomRight.y())
        {
            m_adjustVertical = 1;
        }

        if (m_adjustHorizontal == 0 && m_adjustVertical == 0)
        {
            ResetCursor();
        }
        else if (m_adjustHorizontal == m_adjustVertical)
        {
            setCursor(Qt::SizeFDiagCursor);
        }
        else if (m_adjustVertical != 0 && m_adjustHorizontal != 0)
        {
            setCursor(Qt::SizeBDiagCursor);
        }
        else if (m_adjustVertical != 0)
        {
            setCursor(Qt::SizeVerCursor);
        }
        else
        {
            setCursor(Qt::SizeHorCursor);
        }
    }
}
