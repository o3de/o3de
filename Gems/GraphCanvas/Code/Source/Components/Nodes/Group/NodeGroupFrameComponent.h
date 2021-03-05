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

#include <QGraphicsWidget>
#include <qgraphicslinearlayout.h>

#include <AzCore/Math/Color.h>

#include <Components/Nodes/NodeFrameGraphicsWidget.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/PersistentIdBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/StateControllers/StackStateController.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    class NodeGroupFrameGraphicsWidget;
    class NodeGroupFrameTitleWidget;
    class NodeGroupFrameBlockAreaWidget;

    class NodeGroupFrameComponent
        : public GraphCanvasPropertyComponent
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public BookmarkRequestBus::Handler
        , public SceneBookmarkRequestBus::Handler
        , public BookmarkNotificationBus::Handler
        , public CommentNotificationBus::Handler
        , public SceneNotificationBus::Handler
        , public SceneMemberNotificationBus::MultiHandler
        , public GeometryNotificationBus::Handler
        , public NodeGroupRequestBus::Handler
        , public NodeGroupNotificationBus::MultiHandler
        , public PersistentIdNotificationBus::Handler
        , public VisualNotificationBus::Handler
    {
        friend class NodeGroupFrameGraphicsWidget;

    public:
        AZ_COMPONENT(NodeGroupFrameComponent, "{71CF9059-C439-4536-BB4B-6A1872A5ED94}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext*);

        class NodeGroupFrameComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(NodeGroupFrameComponentSaveData, "{6F4811ED-BD83-4A2A-8831-58EEA4020D57}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(NodeGroupFrameComponentSaveData, AZ::SystemAllocator, 0);

            NodeGroupFrameComponentSaveData();
            NodeGroupFrameComponentSaveData(NodeGroupFrameComponent* nodeFrameComponent);
            ~NodeGroupFrameComponentSaveData() = default;

            void operator=(const NodeGroupFrameComponentSaveData& other);
            
            void OnBookmarkStatusChanged();
            void OnCollapsedStatusChanged();

            AZ::Color m_color;
            float     m_displayHeight;
            float     m_displayWidth;

            bool      m_enableAsBookmark;
            int       m_shortcut;

            bool                                     m_isCollapsed;
            AZStd::vector< PersistentGraphMemberId > m_persistentGroupedIds;

        private:
            NodeGroupFrameComponent* m_callback;
        };

        NodeGroupFrameComponent();
        ~NodeGroupFrameComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            provided.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            provided.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_NodeVisualService", 0x39c4e7f3));
            incompatible.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
            incompatible.push_back(AZ_CRC("GraphCanvas_VisualService", 0xfbb2c871));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
        }
        ////

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeGroupRequestBus
        StateController<bool>* GetExternallyControlledStateController() override;

        void SetGroupSize(QRectF blockDimension) override;
        QRectF GetGroupBoundingBox() const override;
        AZ::Color GetGroupColor() const override;
        
        void CollapseGroup() override;
        void ExpandGroup() override;
        void UngroupGroup() override;

        bool IsCollapsed() const override;
        AZ::EntityId GetCollapsedNodeId() const override;
        
        void FindGroupedElements(AZStd::vector< NodeId >& groupedElements) override;

        bool IsInTitle(const QPointF& scenePos) const override;
        ////

        // NodeGroupNotifications
        void OnCollapsed(const NodeId& collapsedNodeId) override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;
        void OnAddedToScene(const AZ::EntityId& sceneId) override;
        ////

        // SceneMemberNotificationBus
        void PreOnRemovedFromScene(const AZ::EntityId& sceneId) override;
        void OnRemovedFromScene(const AZ::EntityId& sceneId) override;

        void OnSceneMemberAboutToSerialize(GraphSerialization& serialziationTarget) override;
        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphSerialization& serializationTarget) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position) override;
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

        // SceneBookmarkRequests
        AZ::EntityId GetBookmarkId() const override;
        ////

        // BookmarkBus
        void RemoveBookmark() override;
        int GetShortcut() const override;
        void SetShortcut(int shortcut) override;

        AZStd::string GetBookmarkName() const override;
        void SetBookmarkName(const AZStd::string& bookmarkName) override;
        QRectF GetBookmarkTarget() const override;
        QColor GetBookmarkColor() const override;
        ////

        // BookmarkNotifications
        void OnBookmarkTriggered() override;
        ////

        // CommentNotificationBus
        void OnCommentChanged(const AZStd::string&) override;
        void OnBackgroundColorChanged(const AZ::Color& color) override;
        ////

        // SceneNotificationBus
        void OnSceneMemberDragBegin() override;
        void OnSceneMemberDragComplete() override;

        void OnDragSelectStart() override;
        void OnDragSelectEnd() override;

        void OnEntitiesDeserializationComplete(const GraphSerialization&) override;

        void OnGraphLoadComplete() override;

        void OnSceneMemberAdded(const AZ::EntityId& sceneMemberId) override;
        void OnSceneMemberRemoved(const AZ::EntityId& sceneMemberId) override;
        ////

        // PersistentIdNotifications
        void OnPersistentIdsRemapped(const AZStd::unordered_map<PersistentGraphMemberId, PersistentGraphMemberId>& persistentIdRemapping) override;
        ////

    protected:

        void RestoreCollapsedState();

        void FindInteriorElements(AZStd::vector< NodeId >& interiorElements);

        float SetDisplayHeight(float height);
        float SetDisplayWidth(float width);

        void EnableInteriorHighlight(bool highlight);
        void EnableGroupedDisplayState(bool enabled);

        void OnBookmarkStatusChanged();

        void CacheGroupedElements();
        void DirtyGroupedElements();

    private:

        void SetupHighlightElementsStateSetters();

        void SetupGroupedElementsStateSetters();
        void SetupSubGraphGroupedElementsStateSetters(const GraphSubGraph& subGraph);

        NodeGroupFrameComponent(const NodeGroupFrameComponent&) = delete;
        const NodeGroupFrameComponent& operator=(const NodeGroupFrameComponent&) = delete;
        void OnExpanded();

        void FindElementsForDrag();

        QGraphicsLinearLayout*               m_displayLayout;
        
        AZStd::unique_ptr<NodeGroupFrameGraphicsWidget> m_frameWidget;

        NodeGroupFrameTitleWidget*     m_titleWidget;
        NodeGroupFrameBlockAreaWidget* m_blockWidget;

        NodeGroupFrameComponentSaveData m_saveData;

        AZ::Vector2                             m_previousPosition;

        AZ::EntityId                            m_collapsedNodeId;

        bool                                    m_enableSelectionManipulation;

        bool                                    m_needsElements;
        AZStd::vector< AZ::EntityId >           m_movingElements;
        
        bool                                    m_groupedElementsDirty;
        AZStd::vector< AZ::EntityId >           m_groupedElements;

        StackStateController<bool>                  m_isExternallyControlled;

        StateSetter< RootGraphicsItemDisplayState > m_highlightDisplayStateStateSetter;
        StateSetter< bool >                         m_externallyControlledStateSetter;

        // Grouped Element StateControllers
        StateSetter< AZStd::string >                m_forcedGroupLayerStateSetter;
        StateSetter< RootGraphicsItemDisplayState > m_forcedGroupDisplayStateStateSetter;
    };

    //! The QGraphicsItem for the Node Group title area
    class NodeGroupFrameTitleWidget
        : public QGraphicsWidget
    {
    public:
        AZ_TYPE_INFO(NodeGroupFrameTitleWidget, "{FC062E52-CA81-4DA5-B9BF-48FD7BE6E374}");
        AZ_CLASS_ALLOCATOR(NodeGroupFrameTitleWidget, AZ::SystemAllocator, 0);

        NodeGroupFrameTitleWidget();

        void RefreshStyle(const AZ::EntityId& parentId);
        void SetColor(const AZ::Color& color);

        void RegisterFrame(NodeGroupFrameGraphicsWidget* frameWidget);

        // QGraphicsWidget
        void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
        ////

    private:

        Styling::StyleHelper m_styleHelper;
        QColor m_color;

        NodeGroupFrameGraphicsWidget* m_frameWidget;
    };

    //! The QGraphicsItem for the Node Group resiable area
    class NodeGroupFrameBlockAreaWidget 
        : public QGraphicsWidget
    {
    public:
        AZ_TYPE_INFO(NodeGroupFrameBlockAreaWidget, "{9278BBBC-5872-4CA0-9F09-10BAE77ECA7E}");
        AZ_CLASS_ALLOCATOR(NodeGroupFrameBlockAreaWidget, AZ::SystemAllocator, 0);

        NodeGroupFrameBlockAreaWidget();

        void RegisterFrame(NodeGroupFrameGraphicsWidget* frame);

        void RefreshStyle(const AZ::EntityId& parentId);
        void SetColor(const AZ::Color& color);

        // QGraphicsWidget
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////

    private:

        Styling::StyleHelper m_styleHelper;
        QColor m_color;

        NodeGroupFrameGraphicsWidget* m_frameWidget;
    };

    //! The QGraphicsItem for the Node Group frame.
    class NodeGroupFrameGraphicsWidget
        : public NodeFrameGraphicsWidget
        , public CommentNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
        friend class NodeGroupFrameComponent;
        friend class NodeGroupFrameTitleWidget;

    public:
        AZ_TYPE_INFO(NodeGroupFrameGraphicsWidget, "{708C3817-C668-47B7-A4CB-0896425E634A}");
        AZ_CLASS_ALLOCATOR(NodeGroupFrameGraphicsWidget, AZ::SystemAllocator, 0);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        NodeGroupFrameGraphicsWidget(const AZ::EntityId& nodeVisual, NodeGroupFrameComponent& frameComponent);
        ~NodeGroupFrameGraphicsWidget() override = default;

        void RefreshStyle(const AZ::EntityId& styleEntity);
        void SetResizableMinimum(const QSizeF& minimumSize);

        void SetUseTitleShape(bool enable);

        // NodeFrameGraphicsWidget
        void OnActivated() override;

        QPainterPath GetOutline() const override;
        ////

        // QGraphicsWidget
        void hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent) override;
        void hoverMoveEvent(QGraphicsSceneHoverEvent* hoverEvent) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent) override;

        void mousePressEvent(QGraphicsSceneMouseEvent* pressEvent) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* releaseEvent) override;

        bool sceneEventFilter(QGraphicsItem*, QEvent* event) override;
        ////

        // CommentNotificationBus
        void OnEditBegin();
        void OnEditEnd();

        void OnCommentSizeChanged(const QSizeF& oldSize, const QSizeF& newSize) override;

        void OnCommentFontReloadBegin();
        void OnCommentFontReloadEnd();
        ////

        // QGraphicsItem
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        QPainterPath shape() const override;

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
        ////

        // SceneMemberNotificationBus
        void OnMemberSetupComplete() override;
        ////

    protected:

        void UpdateHighlightState();
        void SetHighlightState(bool highlightState);

        void ResizeTo(float height, float width);

        // NodeFrameGraphicsWidget
        void OnDeactivated() override;
        ////

    private:

        void UpdateMinimumSize();

        void ResetCursor();
        void UpdateCursor(QPointF cursorPoint);

        NodeGroupFrameComponent& m_nodeFrameComponent;

        bool m_useTitleShape;
        bool m_allowCommentReaction;

        bool m_allowMovement;
        bool m_resizeComment;

        bool m_allowDraw;

        int m_adjustVertical;
        int m_adjustHorizontal;

        bool m_overTitleWidget;
        bool m_isSelected;
        bool m_enableHighlight;

        QSizeF m_minimumSize;

        QSizeF m_resizableMinimum;
    };
}
