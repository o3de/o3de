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
#include "precompiled.h"

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
            AZ::Crc32 colorId = AZ_CRC("Color", 0x665648e9);
            AZ::Crc32 heightId = AZ_CRC("DisplayHeight", 0x7e251278);
            AZ::Crc32 widthId = AZ_CRC("DisplayWidth", 0x5a55d875);

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

    void NodeGroupFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<NodeGroupFrameComponentSaveData, ComponentSaveData>()
                ->Version(3)
                ->Field("Color", &NodeGroupFrameComponentSaveData::m_color)
                ->Field("DisplayHeight", &NodeGroupFrameComponentSaveData::m_displayHeight)
                ->Field("DisplayWidth", &NodeGroupFrameComponentSaveData::m_displayWidth)
                ->Field("EnableAsBookmark", &NodeGroupFrameComponentSaveData::m_enableAsBookmark)
                ->Field("QuickIndex", &NodeGroupFrameComponentSaveData::m_shortcut)
                ->Field("Collapsed", &NodeGroupFrameComponentSaveData::m_isCollapsed)
                ->Field("PersistentGroupedId", &NodeGroupFrameComponentSaveData::m_persistentGroupedIds)
            ;

            serializeContext->Class<NodeGroupFrameComponent, GraphCanvasPropertyComponent>()
                ->Version(2, &NodeGroupFrameComponentVersionConverter)
                ->Field("SaveData", &NodeGroupFrameComponent::m_saveData)
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
        , m_enableSelectionManipulation(true)
        , m_needsElements(true)
        , m_groupedElementsDirty(true)
        , m_isExternallyControlled(false)
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

        m_frameWidget->Activate();
    }

    void NodeGroupFrameComponent::Deactivate()
    {
        GraphCanvasPropertyComponent::Deactivate();

        m_frameWidget->Deactivate();

        NodeGroupRequestBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::MultiHandler::BusDisconnect();
        BookmarkNotificationBus::Handler::BusDisconnect();
        BookmarkRequestBus::Handler::BusDisconnect();
        NodeGroupRequestBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
    }

    StateController<bool>* NodeGroupFrameComponent::GetExternallyControlledStateController()
    {
        return &m_isExternallyControlled;
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
        m_saveData.SignalDirty();

        m_frameWidget->ResizeTo(m_saveData.m_displayHeight, m_saveData.m_displayWidth);
        m_frameWidget->adjustSize();

        QPointF position = blockRectangle.topLeft();

        position.setY(m_frameWidget->RoundToClosestStep(aznumeric_cast<int>(position.y() - titleSize.height()), m_frameWidget->GetGridYStep()));

        GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, AZ::Vector2(aznumeric_cast<float>(position.x()), aznumeric_cast<float>(position.y())));
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
            CommentUIRequestBus::Event(GetEntityId(), &CommentUIRequests::SetEditable, false);

            // Only want to search for new elements if the grouped elements is empty.
            // Otherwise we just want to use whatever we previously found.
            //
            // This will occur when we are loading from a save and are using the persistent Id's to figure out
            // what we should do.
            //
            // Will also help in situations where we expand and collapse a node group without manipulating anything
            // within the scene(or only manipulating the group)
            CacheGroupedElements();

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

                AZ::Vector2 position = ConversionUtils::QPointToVector(m_frameWidget->sceneBoundingRect().center());

                GeometryRequestBus::Event(nodeGroup->GetId(), &GeometryRequests::SetPosition, position);

                SceneRequestBus::Event(graphId, &SceneRequests::Add, nodeGroup->GetId());

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
                m_saveData.SignalDirty();
                GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

                if (m_enableSelectionManipulation)
                {
                    SceneMemberUIRequestBus::Event(m_collapsedNodeId, &SceneMemberUIRequests::SetSelected, true);
                }
            }
            else
            {
                m_saveData.m_isCollapsed = false;
                m_saveData.m_persistentGroupedIds.clear();
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
                OnExpanded();
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

            m_groupedElements.clear();
            m_groupedElementsDirty = false;

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

    void NodeGroupFrameComponent::FindGroupedElements(AZStd::vector< NodeId >& groupedElements)
    {
        if (m_groupedElementsDirty)
        {
            FindInteriorElements(groupedElements);
        }
        else
        {
            groupedElements = m_groupedElements;
        }
    }

    bool NodeGroupFrameComponent::IsInTitle(const QPointF& scenePos) const
    {
        return m_titleWidget->sceneBoundingRect().contains(scenePos);
    }

    void NodeGroupFrameComponent::OnCollapsed(const NodeId& collapsedNodeId)
    {
        const AZ::EntityId* busId = NodeGroupNotificationBus::GetCurrentBusId();

        if (busId)
        {
            NodeGroupNotificationBus::MultiHandler::BusDisconnect((*busId));

            m_groupedElements.push_back(collapsedNodeId);

            if (!NodeGroupNotificationBus::MultiHandler::BusIsConnected())
            {
                RestoreCollapsedState();
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
                GeometryNotificationBus::Handler::BusConnect(entityId);

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
            }
        }
    }

    void NodeGroupFrameComponent::PreOnRemovedFromScene([[maybe_unused]] const AZ::EntityId& sceneId)
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
            else if ((*busId) == m_collapsedNodeId)
            {
                m_collapsedNodeId.SetInvalid();

                OnExpanded();
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

                AZStd::unordered_set< AZ::EntityId > memberIds = { GetEntityId() };
                GraphUtils::ParseMembersForSerialization(serializationTarget, memberIds);
            }
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberDeserialized(const AZ::EntityId& graphId, [[maybe_unused]] const GraphSerialization& serializationTarget)
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
        m_titleWidget->RefreshStyle(GetEntityId());
        m_blockWidget->RefreshStyle(GetEntityId());

        QSizeF titleMinimumSize = m_titleWidget->minimumSize();
        QSizeF blockMinimumSize = m_blockWidget->minimumSize();

        QSizeF finalMin;
        finalMin.setWidth(AZ::GetMax(titleMinimumSize.width(), blockMinimumSize.width()));
        finalMin.setHeight(titleMinimumSize.height() + blockMinimumSize.height());

        m_frameWidget->SetResizableMinimum(finalMin);
    }

    void NodeGroupFrameComponent::OnPositionChanged([[maybe_unused]] const AZ::EntityId& entityId, const AZ::Vector2& position)
    {
        if (m_frameWidget->m_allowMovement && !m_isExternallyControlled.GetState())
        {
            FindElementsForDrag();

            if (!m_movingElements.empty())
            {
                AZ::Vector2 delta = position - m_previousPosition;

                if (!delta.IsZero())
                {
                    for (const AZ::EntityId& element : m_movingElements)
                    {
                        AZ::Vector2 position;
                        GeometryRequestBus::EventResult(position, element, &GeometryRequests::GetPosition);

                        position += delta;
                        GeometryRequestBus::Event(element, &GeometryRequests::SetPosition, position);
                    }
                }
            }
        }

        m_previousPosition = position;
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

        m_saveData.SignalDirty();
    }

    int NodeGroupFrameComponent::GetShortcut() const
    {
        return m_saveData.m_shortcut;
    }

    void NodeGroupFrameComponent::SetShortcut(int shortcut)
    {
        m_saveData.m_shortcut = shortcut;
        m_saveData.SignalDirty();
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
        if (m_frameWidget->IsSelected())
        {
            FindElementsForDrag();
            EnableGroupedDisplayState(true);
            CacheGroupedElements();
        }
        else
        {
            DirtyGroupedElements();
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberDragComplete()
    {
        m_needsElements = true;

        for (const AZ::EntityId& sceneMember : m_movingElements)
        {
            SceneMemberRequestBus::Event(sceneMember, &SceneMemberRequests::UnlockForExternalMovement, GetEntityId());
        }

        m_movingElements.clear();
        m_externallyControlledStateSetter.ResetStateSetter();
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

        RestoreCollapsedState();
    }

    void NodeGroupFrameComponent::OnSceneMemberAdded(const AZ::EntityId&)
    {
        if (!m_saveData.m_isCollapsed)
        {
            DirtyGroupedElements();
        }
    }

    void NodeGroupFrameComponent::OnSceneMemberRemoved(const AZ::EntityId&)
    {
        if (!m_saveData.m_isCollapsed)
        {
            DirtyGroupedElements();
        }
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

        PersistentIdNotificationBus::Handler::BusDisconnect();
    }

    void NodeGroupFrameComponent::RestoreCollapsedState()
    {
        if (m_saveData.m_isCollapsed)
        {
            m_frameWidget->adjustSize();

            m_groupedElements.clear();

            bool canCollapseNode = true;

            for (const PersistentGraphMemberId& persistentMemberId : m_saveData.m_persistentGroupedIds)
            {
                AZ::EntityId graphMemberId;
                PersistentIdRequestBus::EventResult(graphMemberId, persistentMemberId, &PersistentIdRequests::MapToEntityId);

                if (graphMemberId.IsValid())
                {
                    if (GraphUtils::IsNodeGroup(graphMemberId))
                    {
                        bool isCollapsed = false;
                        NodeGroupRequestBus::EventResult(isCollapsed, graphMemberId, &NodeGroupRequests::IsCollapsed);

                        if (isCollapsed)
                        {
                            NodeId collapsedNodeId;
                            NodeGroupRequestBus::EventResult(collapsedNodeId, graphMemberId, &NodeGroupRequests::GetCollapsedNodeId);

                            if (!collapsedNodeId.IsValid())
                            {
                                NodeGroupNotificationBus::MultiHandler::BusConnect(graphMemberId);
                            }
                            else
                            {
                                m_groupedElements.push_back(collapsedNodeId);
                            }
                        }

                        continue;
                    }

                    m_groupedElements.push_back(graphMemberId);
                }
            }

            m_groupedElementsDirty = false;

            if (canCollapseNode)
            {
                QScopedValueRollback<bool> manipulationBlocker(m_enableSelectionManipulation, false);
                CollapseGroup();
            }
        }
    }

    void NodeGroupFrameComponent::FindInteriorElements(AZStd::vector< NodeId >& interiorElements)
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

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

        AZStd::vector< AZ::EntityId > elementList;
        SceneRequestBus::EventResult(elementList, sceneId, &SceneRequests::GetEntitiesInRect, blockArea, Qt::ItemSelectionMode::ContainsItemShape);

        interiorElements.clear();
        interiorElements.reserve(elementList.size());

        for (const AZ::EntityId& testElement : elementList)
        {
            if (GraphUtils::IsConnection(testElement) || testElement == GetEntityId())
            {
                continue;
            }

            bool isVisible = true;

            VisualRequestBus::EventResult(isVisible, testElement, &VisualRequests::IsVisible);

            if (isVisible)
            {
                interiorElements.push_back(testElement);
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
        m_saveData.SignalDirty();

        DirtyGroupedElements();

        return height;
    }

    float NodeGroupFrameComponent::SetDisplayWidth(float width)
    {
        if (m_frameWidget && m_frameWidget->m_minimumSize.width() > width)
        {
            width = aznumeric_cast<float>(m_frameWidget->m_minimumSize.width());
        }

        m_saveData.m_displayWidth = width;
        m_saveData.SignalDirty();

        DirtyGroupedElements();

        return width;
    }

    void NodeGroupFrameComponent::EnableInteriorHighlight(bool highlight)
    {
        m_highlightDisplayStateStateSetter.ResetStateSetter();

        if (highlight)
        {
            SetupHighlightElementsStateSetters();
            m_highlightDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
        }        
    }

    void NodeGroupFrameComponent::EnableGroupedDisplayState(bool enabled)
    {
        m_forcedGroupDisplayStateStateSetter.ResetStateSetter();

        m_forcedGroupLayerStateSetter.ResetStateSetter();

        if (enabled)
        {
            SetupGroupedElementsStateSetters();

            m_forcedGroupDisplayStateStateSetter.SetState(RootGraphicsItemDisplayState::GroupHighlight);
            m_forcedGroupLayerStateSetter.SetState("grouped");
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

    void NodeGroupFrameComponent::CacheGroupedElements()
    {
        if (!m_saveData.m_isCollapsed && m_groupedElementsDirty)
        {
            m_groupedElements.clear();

            FindInteriorElements(m_groupedElements);

            m_saveData.m_persistentGroupedIds.clear();

            for (AZ::EntityId groupedMemberId : m_groupedElements)
            {
                if (GraphUtils::IsCollapsedNodeGroup(groupedMemberId))
                {
                    CollapsedNodeGroupRequestBus::EventResult(groupedMemberId, groupedMemberId, &CollapsedNodeGroupRequests::GetSourceGroup);
                }

                PersistentGraphMemberId graphMemberId = PersistentGraphMemberId::CreateNull();
                PersistentMemberRequestBus::EventResult(graphMemberId, groupedMemberId, &PersistentMemberRequests::GetPersistentGraphMemberId);

                if (!graphMemberId.IsNull())
                {
                    m_saveData.m_persistentGroupedIds.emplace_back(graphMemberId);
                }
            }

            m_groupedElementsDirty = false;
        }
    }

    void NodeGroupFrameComponent::DirtyGroupedElements()
    {

        // We only want to invalidate our grouped elements when we aren't collapsed.
        if (!m_saveData.m_isCollapsed && !m_groupedElementsDirty)
        {
            m_groupedElementsDirty = true;
            EnableGroupedDisplayState(false);

            m_saveData.m_persistentGroupedIds.clear();
            m_saveData.SignalDirty();

            m_groupedElements.clear();            
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

            StateController<AZStd::string>* layerStateController = nullptr;
            LayerControllerRequestBus::EventResult(layerStateController, elementId, &LayerControllerRequests::GetLayerModifierController);

            m_forcedGroupLayerStateSetter.AddStateController(layerStateController);
        }

        for (const AZ::EntityId& elementId : subGraph.m_innerConnections)
        {
            StateController<AZStd::string>* layerStateController = nullptr;
            LayerControllerRequestBus::EventResult(layerStateController, elementId, &LayerControllerRequests::GetLayerModifierController);

            m_forcedGroupLayerStateSetter.AddStateController(layerStateController);
        }
    }

    void NodeGroupFrameComponent::OnExpanded()
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

        m_saveData.m_isCollapsed = false;
        m_saveData.SignalDirty();

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);

        if (m_enableSelectionManipulation)
        {
            m_frameWidget->SetSelected(true);
        }
    }

    void NodeGroupFrameComponent::FindElementsForDrag()
    {
        if (m_needsElements)
        {
            AZ_Warning("Graph Canvas", m_movingElements.empty(), "Moving elements should be empty when scraping for new elements.");
            AZ_Warning("Graph Canvas", !m_externallyControlledStateSetter.HasState() && !m_externallyControlledStateSetter.HasTargets(), "Externally Controlled Objects should be empty when scraping for new elements.")

            m_externallyControlledStateSetter.SetState(true);

            m_needsElements = false;

            FindGroupedElements(m_movingElements);

            AZStd::vector< AZ::EntityId >::iterator elementIter = m_movingElements.begin();

            while (elementIter != m_movingElements.end())
            {
                AZ::EntityId currentElement = (*elementIter);
                if (GraphUtils::IsNodeGroup(currentElement))
                {
                    StateController<bool>* stateController = nullptr;
                    NodeGroupRequestBus::EventResult(stateController, currentElement, &NodeGroupRequests::GetExternallyControlledStateController);

                    if (stateController)
                    {
                        m_externallyControlledStateSetter.AddStateController(stateController);
                    }
                }

                // We don't want to move anything that is selected, since in the drag move
                // Qt will handle moving it already, so we don't want to double move it.
                bool isSelected = false;
                SceneMemberUIRequestBus::EventResult(isSelected, currentElement, &SceneMemberUIRequests::IsSelected);

                // Next, for anything that is inside the intersection of two Node Groups, we only
                // want one of them to be able to move them in the case of a double selection.
                // So we will let each one try to lock something for external movement, and then unset that
                // once the movement is complete
                bool lockedForExternalMovement = false;
                SceneMemberRequestBus::EventResult(lockedForExternalMovement, currentElement, &SceneMemberRequests::LockForExternalMovement, GetEntityId());

                if (isSelected || !lockedForExternalMovement)
                {
                    elementIter = m_movingElements.erase(elementIter);

                    if (lockedForExternalMovement)
                    {
                        SceneMemberRequestBus::Event(currentElement, &SceneMemberRequests::UnlockForExternalMovement, GetEntityId());
                    }
                }
                else
                {
                    ++elementIter;
                }
            }
        }
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
        m_styleHelper.SetStyle(parentId, Styling::Elements::BlockComment::Title);
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
        m_styleHelper.SetStyle(parentId, Styling::Elements::BlockComment::BlockArea);
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

            qreal penWidth = border.width();
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
        m_style.SetStyle(styleEntity, "");

        QColor backgroundColor = m_style.GetAttribute(Styling::Attribute::BackgroundColor, QColor(0,0,0));
        setOpacity(backgroundColor.alphaF() / 255.0);
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

            QRectF rect = boundingRect();

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
            QScopedValueRollback<bool> allowMovement(m_allowMovement, false);

            mouseEvent->accept();

            QRectF blockBoundingRect = m_nodeFrameComponent.m_blockWidget->sceneBoundingRect();
            QRectF calculatedBounds;

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            AZ::Vector2 gridStep;

            AZ::EntityId grid;
            GraphCanvas::SceneRequestBus::EventResult(grid, sceneId, &GraphCanvas::SceneRequests::GetGrid);
            GraphCanvas::GridRequestBus::EventResult(gridStep, grid, &GraphCanvas::GridRequests::GetMinorPitch);

            QRectF searchBoundingRect = sceneBoundingRect();
            searchBoundingRect.adjust(-gridStep.GetX() * 0.5f, -gridStep.GetY() * 0.5f, gridStep.GetX() * 0.5f, gridStep.GetY() * 0.5f);

            AZStd::vector< AZ::EntityId > entities;
            SceneRequestBus::EventResult(entities, sceneId, &SceneRequests::GetEntitiesInRect, searchBoundingRect, Qt::ItemSelectionMode::IntersectsItemBoundingRect);

            for (AZ::EntityId entity : entities)
            {
                // Don't want to resize to connections.
                // And don't want to include ourselves in this calculation
                if (ConnectionRequestBus::FindFirstHandler(entity) != nullptr || entity == GetEntityId())
                {
                    continue;
                }

                QGraphicsItem* graphicsItem = nullptr;
                SceneMemberUIRequestBus::EventResult(graphicsItem, entity, &SceneMemberUIRequests::GetRootGraphicsItem);

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
                calculatedBounds.adjust(-gridStep.GetX(), -gridStep.GetY(), gridStep.GetX(), gridStep.GetY());

                if (m_adjustHorizontal != 0)
                {
                    blockBoundingRect.setX(calculatedBounds.x());
                    blockBoundingRect.setWidth(calculatedBounds.width());
                }

                if (m_adjustVertical != 0)
                {
                    blockBoundingRect.setY(calculatedBounds.y());
                    blockBoundingRect.setHeight(calculatedBounds.height());
                }

                m_nodeFrameComponent.SetGroupSize(blockBoundingRect);
            }

            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RequestUndoPoint);
        }
        else if (m_nodeFrameComponent.m_titleWidget->sceneBoundingRect().contains(mouseEvent->scenePos()))
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

    void NodeGroupFrameGraphicsWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {   
        if (isSelected())
        {
            QPen border;
            border.setWidth(3);
            border.setColor("#17A3CD");
            border.setStyle(Qt::PenStyle::DashLine);
            painter->setPen(border);

            painter->drawRect(boundingRect());
        }
        else if (m_allowDraw)
        {            
            if (GetDisplayState() == RootGraphicsItemDisplayState::Deletion)
            {
                QPen border;
                border.setWidth(3);
                border.setColor("#EF1713");
                border.setStyle(Qt::PenStyle::DashLine);
                painter->setPen(border);

                painter->drawRect(boundingRect());
            }
            else if (GetDisplayState() == RootGraphicsItemDisplayState::Inspection)
            {
                QPen border;
                border.setWidth(3);
                border.setColor("#4285f4");
                border.setStyle(Qt::PenStyle::DashLine);
                painter->setPen(border);

                painter->drawRect(boundingRect());
            }
            else
            {
                QPen border;
                border.setWidth(3);
                border.setColor("#000000");
                border.setStyle(Qt::PenStyle::DashLine);
                painter->setPen(border);

                painter->drawRect(boundingRect());
            }
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
