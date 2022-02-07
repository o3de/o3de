/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        , public GeometryNotificationBus::MultiHandler
        , public NodeGroupRequestBus::Handler
        , public NodeGroupNotificationBus::MultiHandler
        , public PersistentIdNotificationBus::Handler
        , public VisualNotificationBus::MultiHandler
        , public AZ::SystemTickBus::Handler
        , public RootGraphicsItemNotificationBus::Handler
        , public CollapsedNodeGroupNotificationBus::Handler
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

            // Signals wether or not this group was created before or after the group refactor so we can update the initial state.
            bool      m_isNewGroup = true;

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
        void SetGroupSize(QRectF blockDimension) override;
        QRectF GetGroupBoundingBox() const override;
        AZ::Color GetGroupColor() const override;
        
        void CollapseGroup() override;
        void ExpandGroup() override;
        void UngroupGroup() override;        

        bool IsCollapsed() const override;
        AZ::EntityId GetCollapsedNodeId() const override;

        void AddElementToGroup(const AZ::EntityId& groupableElement) override;
        void AddElementsToGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements) override;
        void AddElementsVectorToGroup(const AZStd::vector<AZ::EntityId>& groupableElements) override;

        void RemoveElementFromGroup(const AZ::EntityId& groupableElement) override;
        void RemoveElementsFromGroup(const AZStd::unordered_set<AZ::EntityId>& groupableElements) override;
        void RemoveElementsVectorFromGroup(const AZStd::vector<AZ::EntityId>& groupableElements) override;
        
        void FindGroupedElements(AZStd::vector< NodeId >& groupedElements) override;

        void ResizeGroupToElements(bool growGroupOnly) override;

        bool IsInTitle(const QPointF& scenePos) const override;

        void AdjustTitleSize() override;
        ////

        // NodeGroupNotifications
        void OnCollapsed(const NodeId& collapsedNodeId) override;
        void OnExpanded() override;
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
        void OnBoundsChanged() override;
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

        void OnNodeRemoved(const AZ::EntityId& sceneMemberId) override;
        void OnSceneMemberRemoved(const AZ::EntityId& sceneMemberId) override;

        void OnEntitiesDeserializationComplete(const GraphSerialization&) override;

        void OnGraphLoadComplete() override;
        void PostOnGraphLoadComplete() override;
        ////

        // PersistentIdNotifications
        void OnPersistentIdsRemapped(const AZStd::unordered_map<PersistentGraphMemberId, PersistentGraphMemberId>& persistentIdRemapping) override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        // VisualNotificationBus
        void OnPositionAnimateBegin() override;
        void OnPositionAnimateEnd() override;
        ////

        // RootGraphicsNotificationBus
        void OnDisplayStateChanged(RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState) override;
        ////

        // CollapsedNodeGroupNotificationBus
        void OnExpansionComplete() override;
        ////

        void OnFrameResizeStart();
        void OnFrameResized();
        void OnFrameResizeEnd();

        EditorId GetEditorId() const;

    protected:

        void RestoreCollapsedState();
        void TryAndRestoreCollapsedState();
        
        void FindInteriorElements(AZStd::unordered_set< AZ::EntityId >& interiorElements, Qt::ItemSelectionMode = Qt::ItemSelectionMode::ContainsItemShape);

        float SetDisplayHeight(float height);
        float SetDisplayWidth(float width);

        void EnableInteriorHighlight(bool highlight);
        void EnableGroupedDisplayState(bool enabled);

        void OnBookmarkStatusChanged();

        void UpdateSavedElements();

        void RemapGroupedPersistentIds();

    private:

        bool AddToGroupInternal(const AZ::EntityId& groupableElement);

        void UpdateHighlightState();
        void SetupHighlightElementsStateSetters();

        void SetupGroupedElementsStateSetters();
        void SetupSubGraphGroupedElementsStateSetters(const GraphSubGraph& subGraph);

        void OnElementGrouped(const AZ::EntityId& groupedElement);
        void OnElementUngrouped(const AZ::EntityId& ungroupedElement);

        NodeGroupFrameComponent(const NodeGroupFrameComponent&) = delete;
        const NodeGroupFrameComponent& operator=(const NodeGroupFrameComponent&) = delete;
        void SignalExpanded();

        void SetupElementsForMove();
        void SignalDirty();

        QRectF GetGroupBoundary() const;

        QGraphicsLinearLayout*               m_displayLayout;
        
        AZStd::unique_ptr<NodeGroupFrameGraphicsWidget> m_frameWidget;

        NodeGroupFrameTitleWidget*     m_titleWidget;
        NodeGroupFrameBlockAreaWidget* m_blockWidget;

        NodeGroupFrameComponentSaveData m_saveData;

        AZ::Vector2                             m_previousPosition;

        EditorId                                m_editorId;

        AZ::EntityId                            m_collapsedNodeId;

        bool                                    m_needsDisplayStateHighlight;
        bool                                    m_needsManualHighlight;

        bool                                    m_enableSelectionManipulation;
        bool                                    m_ignoreDisplayStateChanges;
        bool                                    m_ignoreSubSlementPostionChanged;
        bool                                    m_isGroupAnimating;

        AZStd::unordered_set< AZ::EntityId >    m_initializingGroups;
        AZStd::unordered_set< AZ::EntityId >    m_groupedGrouped;
        AZStd::unordered_map< AZ::EntityId, AZ::EntityId > m_collapsedGroupMapping;

        AZStd::unordered_set< AZ::EntityId >    m_movingElements;

        AZStd::unordered_set< AZ::EntityId >        m_ignoreElementsOnResize;
        AZStd::unordered_set< AZ::EntityId >        m_groupedElements;

        AZStd::unordered_set< AZ::EntityId >        m_animatingElements;

        // List of redirections available on the collapsed group. Used to persist these
        // slots on a collapsed node post duplicate/copy and paste. Will not persist between saves.
        AZStd::vector< Endpoint >   m_collapsedRedirectionEndpoints;

        StateSetter< RootGraphicsItemDisplayState > m_highlightDisplayStateStateSetter;

        // Grouped Element StateControllers
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
        void OnEditBegin() override;
        void OnEditEnd() override;

        void OnCommentSizeChanged(const QSizeF& oldSize, const QSizeF& newSize) override;

        void OnCommentFontReloadBegin() override;
        void OnCommentFontReloadEnd() override;
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

        void ResizeToGroup(int adjustHorizontal, int adjustVertical, bool growOnly = false);

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

        Styling::StyleHelper m_borderStyle;

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
