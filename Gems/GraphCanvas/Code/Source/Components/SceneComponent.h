/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsScene>
#include <QRect>
#include <QTimer>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Vector2.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/GraphicsItems/GlowOutlineGraphicsItem.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>

class QAction;
class QMimeData;

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QGraphicsScene, "{C27E4829-BA47-4BAA-B9FD-C549ACF316B7}");
}

class QGraphicsItem;

namespace GraphCanvas
{
    class GraphCanvasGraphicsScene;

    class SceneHelper        
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneHelper, AZ::SystemAllocator);
        SceneHelper() = default;
        virtual ~SceneHelper() = default;

        void SetSceneId(const AZ::EntityId& sceneId);
        const AZ::EntityId& GetSceneId() const;

        void SetEditorId(const EditorId& editorId);
        const EditorId& GetEditorId() const;

    protected:
        virtual void OnEditorIdSet();

    private:        
        AZ::EntityId m_sceneId;
        EditorId     m_editorId;
    };

    //! Separate class just to avoid over-cluttering the scene.
    //
    // Handles the creation process for nodes
    class MimeDelegateSceneHelper
        : public SceneHelper
        , public SceneMimeDelegateHandlerRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MimeDelegateSceneHelper, AZ::SystemAllocator);
        MimeDelegateSceneHelper() = default;
        ~MimeDelegateSceneHelper() = default;

        void Activate();
        void Deactivate();

        void SetMimeType(const char* mimeType);
        const QString& GetMimeType() const;
        bool HasMimeType() const;

        // SceneMimeDelegateHandlerRequestBus
        bool IsInterestedInMimeData(const AZ::EntityId& sceneId, const QMimeData* dragEnterEvent) override;
        void HandleMove(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        void HandleDrop(const AZ::EntityId& sceneId, const QPointF& dropPoint, const QMimeData* mimeData) override;
        void HandleLeave(const AZ::EntityId& sceneId, const QMimeData* mimeData) override;
        ////

        void SignalNodeCreated(const NodeId& nodeId);

    private:
        
        void OnTrySplice();
        void CancelSplice();

        void PushUndoBlock();
        void PopUndoBlock();

        void AssignLastCreationToGroup();

        QString m_mimeType;

        NodeNudgingController m_nudgingController;

        QTimer m_spliceTimer;

        AZ::EntityId m_targetConnection;

        bool m_enableConnectionSplicing;
        QByteArray m_splicingData;

        AZ::EntityId m_splicingNode;
        QPainterPath m_splicingPath;
        AZ::Vector2  m_positionOffset;

        QPointF m_targetPosition;

        Endpoint m_spliceSource;
        Endpoint m_spliceTarget;
        AZStd::vector< ConnectionEndpoints > m_opportunisticSpliceRemovals;

        bool m_pushedUndoBlock;
        
        StateSetter<RootGraphicsItemDisplayState> m_displayStateStateSetter;

        AZ::EntityId m_groupTarget;
        StateSetter<RootGraphicsItemDisplayState> m_groupTargetStateSetter;

        AZStd::unordered_set< NodeId > m_lastCreationGroup;
    };

    // Handles identifying Gestures for the Scene.
    // Helper class just to avoid overcomplicating the actual scene class
    class GestureSceneHelper
        : public SceneHelper
        , public GeometryNotificationBus::Handler
        , public SceneNotificationBus::Handler        
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GestureSceneHelper, AZ::SystemAllocator);
        GestureSceneHelper() = default;
        ~GestureSceneHelper() = default;

        void Activate();
        void Deactivate();

        void TrackElement(const AZ::EntityId& elementId);
        void ResetTracker();
        void StopTrack();

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position) override;
        ////

        void OnSettingsChanged();

        // SystemTickBus
        void OnSystemTick() override;
        ////

    protected:

        void OnEditorIdSet() override;

    private:

        void HandleDesplice();

        QTimer m_timer;

        bool m_handleShakeAction = false;
        bool m_trackShake = false;
        AZ::EntityId m_trackingTarget;

        float m_movementTolerance = 0.0f;
        float m_minimumDistance = 0.0f;

        float m_straightnessPercent = 0.0f;
        
        int m_shakeThreshold = 0;
        int m_shakeCounter = 0;

        QPointF m_currentAnchor;
        QPointF m_lastPoint;

        AZ::Vector2 m_lastDirection;

        bool m_hasDirection = false;
    };

    struct SceneMemberBuckets
    {
        AZStd::unordered_set< AZ::EntityId > m_nodes;
        AZStd::unordered_set< AZ::EntityId > m_connections;
        AZStd::unordered_set< AZ::EntityId > m_bookmarkAnchors;
    };

    class SceneComponent
        : public GraphCanvasPropertyComponent
        , public AZ::EntityBus::Handler
        , public SceneRequestBus::Handler
        , public GeometryNotificationBus::MultiHandler
        , public VisualNotificationBus::MultiHandler
        , public ViewNotificationBus::Handler
        , public SceneMimeDelegateRequestBus::Handler
        , public EntitySaveDataRequestBus::Handler
        , public SceneBookmarkActionBus::Handler
        , public StyleManagerNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
        , public AssetEditorSettingsNotificationBus::Handler
    {
    private:
        friend class GraphCanvasGraphicsScene;
        friend class SceneComponentSaveData;

    public:
        AZ_COMPONENT(SceneComponent, "{3F71486C-3D51-431F-B904-DA070C7A0238}", GraphCanvasPropertyComponent);
        static void Reflect(AZ::ReflectContext* context);

        struct GraphCanvasConstructSaveData
        {
            AZ_CLASS_ALLOCATOR(GraphCanvasConstructSaveData, AZ::SystemAllocator);
            AZ_RTTI(GraphCanvasConstructSaveData, "{C074944F-8218-4753-94EE-1C5CC02DE8E4}");

            static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement);

            // Put the invalid thing at the end of the list, instead of the start.
            // Cannot actually add new elements to this list.
            //
            // Going to make a new enum rather then try to salvage this.
            enum class GraphCanvasConstructType
            {
                CommentNode,
                BlockCommentNode,
                BookmarkAnchor,

                Unknown,
            };            

            virtual ~GraphCanvasConstructSaveData() = default;

            ConstructType m_constructType = ConstructType::Unknown;
            EntitySaveDataContainer m_saveDataContainer;
            AZ::EntityId m_persistentId;
        };

        class SceneComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(SceneComponentSaveData, "{5F84B500-8C45-40D1-8EFC-A5306B241444}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(SceneComponentSaveData, AZ::SystemAllocator);

            SceneComponentSaveData()
                : m_bookmarkCounter(0)
            {
            }

            ~SceneComponentSaveData()
            {
                ClearConstructData();
            }

            void ClearConstructData()
            {
                for (GraphCanvasConstructSaveData* saveData : m_constructs)
                {
                    delete saveData;
                }

                m_constructs.clear();
            }

            AZStd::vector< GraphCanvasConstructSaveData* > m_constructs;
            ViewParams                                     m_viewParams;
            AZ::u32                                        m_bookmarkCounter;
        };

        SceneComponent();
        ~SceneComponent() override;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_SceneService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_MimeDataHandlerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incombatible)
        {
            incombatible.push_back(AZ_CRC_CE("GraphCanvas_SceneService"));
            incombatible.push_back(AZ_CRC_CE("GraphCanvas_MimeDataHandlerService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        // EntityBus
        void OnEntityExists(const AZ::EntityId& entityId) override;
        ////

        // EntitySaveDataRequest
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override;
        ////

        SceneRequests* GetSceneRequests() { return this; }
        const SceneRequests* GetSceneRequestsConst() const { return this; }

        GraphData* GetGraphData() override { return &m_graphData; };
        const GraphData* GetGraphDataConst() const override { return &m_graphData; };

        // SceneRequestBus
        AZStd::any* GetUserData() override;
        const AZStd::any* GetUserDataConst() const override;
        AZ::Entity* GetSceneEntity() const override { return GetEntity(); }

        void SetEditorId(const EditorId& editorId) override;
        EditorId GetEditorId() const override;

        AZ::EntityId GetGrid() const override;

        GraphicsEffectId CreatePulse(const AnimatedPulseConfiguration& pulseConfiguration) override;
        GraphicsEffectId CreatePulseAroundArea(const QRectF& area, int gridSteps, AnimatedPulseConfiguration& pulseConfiguration) override;
        GraphicsEffectId CreatePulseAroundSceneMember(const AZ::EntityId& memberId, int gridSteps, AnimatedPulseConfiguration configuration) override;
        GraphicsEffectId CreateCircularPulse(const AZ::Vector2& centerPoint, float initialRadius, float finalRadius, AnimatedPulseConfiguration configuration) override;

        GraphicsEffectId CreateOccluder(const OccluderConfiguration& occluderConfiguration) override;

        GraphicsEffectId CreateGlow(const FixedGlowOutlineConfiguration& configuration) override;
        GraphicsEffectId CreateGlowOnSceneMember(const SceneMemberGlowOutlineConfiguration& configuration) override;

        GraphicsEffectId CreateParticle(const ParticleConfiguration& configuration) override;
        AZStd::vector< GraphicsEffectId > ExplodeSceneMember(const AZ::EntityId& memberId, float fillPercent) override;

        void CancelGraphicsEffect(const GraphicsEffectId& effectId) override;

        bool AddNode(const AZ::EntityId& nodeId, const AZ::Vector2& position, bool isPaste) override;
        void AddNodes(const AZStd::vector<AZ::EntityId>& nodeIds) override;
        bool RemoveNode(const AZ::EntityId& nodeId) override;

        AZStd::vector<AZ::EntityId> GetNodes() const override;

        AZStd::vector<AZ::EntityId> GetSelectedNodes() const override;

        void DeleteNodeAndStitchConnections(const AZ::EntityId& node) override;

        AZ::EntityId CreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool AddConnection(const AZ::EntityId& connectionId) override;
        void AddConnections(const AZStd::vector<AZ::EntityId>& connectionIds) override;
        bool RemoveConnection(const AZ::EntityId& connectionId) override;

        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<AZ::EntityId> GetSelectedConnections() const override;
        AZStd::vector<AZ::EntityId> GetConnectionsForEndpoint(const Endpoint& firstEndpoint) const override;

        bool IsEndpointConnected(const Endpoint& endpoint) const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;

        bool CreateConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool DisplayConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool Disconnect(const Endpoint&, const Endpoint&) override;
        bool DisconnectById(const AZ::EntityId& connectionId) override;
        bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const override;

        bool AddBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId, const AZ::Vector2& position) override;
        bool RemoveBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId) override;

        bool Add(const AZ::EntityId& entity, bool isPaste = false) override;
        bool Remove(const AZ::EntityId& entity) override;

        bool Show(const AZ::EntityId& graphMemeber) override;
        bool Hide(const AZ::EntityId& graphMemeber) override;

        bool IsHidden(const AZ::EntityId& graphMember) const override;

        bool Enable(const NodeId& nodeId) override;
        void EnableVisualState(const NodeId& nodeId) override;
        void EnableSelection() override;

        bool Disable(const NodeId& nodeId) override;
        void DisableVisualState(const NodeId& nodeId) override;
        void DisableSelection() override;

        void ProcessEnableDisableQueue() override;

        void ClearSelection() override;

        void SetSelectedArea(const AZ::Vector2& topLeft, const AZ::Vector2& topRight) override;
        void SelectAll() override;
        void SelectAllRelative(ConnectionType connectionType) override;
        void SelectConnectedNodes() override;

        bool HasSelectedItems() const override;
        bool HasMultipleSelection() const override;
        bool HasCopiableSelection() const override;

        bool HasEntitiesAt(const AZ::Vector2& scenePoint) const override;

        AZStd::vector<AZ::EntityId> GetSelectedItems() const override;

        QGraphicsScene* AsQGraphicsScene() override;

        void CopySelection() const override;
        void Copy(const AZStd::vector<AZ::EntityId>& itemIds) const override;

        void CutSelection() override;
        void Cut(const AZStd::vector<AZ::EntityId>& itemIds) override;

        void Paste() override;
        void PasteAt(const QPointF& scenePos) override;

        void SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, GraphSerialization& serializationTarget) const override;
        void DeserializeEntities(const QPointF& scenePos, const GraphSerialization& serializationTarget) override;

        void DuplicateSelection() override;
        void DuplicateSelectionAt(const QPointF& scenePos) override;
        void Duplicate(const AZStd::vector<AZ::EntityId>& itemIds) override;
        void DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos) override;

        void DeleteSelection() override;
        void Delete(const AZStd::unordered_set<AZ::EntityId>& itemIds) override;
        void DeleteGraphData(const GraphData& graphData) override;
        
        void ClearScene() override;

        void SuppressNextContextMenu() override;

        const AZStd::string& GetCopyMimeType() const override;
        void SetMimeType(const char* mimeType) override;

        AZStd::vector<AZ::EntityId> GetEntitiesAt(const AZ::Vector2& position) const override;
        AZStd::vector<AZ::EntityId> GetEntitiesInRect(const QRectF& rect, Qt::ItemSelectionMode mode) const override;

        AZStd::vector<Endpoint> GetEndpointsInRect(const QRectF& rect) const override;

        void RegisterView(const AZ::EntityId& viewId) override;
        void RemoveView(const AZ::EntityId& viewId) override;
        ViewId GetViewId() const override;

        void DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData) override;

        bool AddGraphData(const GraphData& graphData) override;
        void RemoveGraphData(const GraphData& graphData) override;

        void SetDragSelectionType(DragSelectionType dragSelectionType) override;

        void SignalDragSelectStart() override;
        void SignalDragSelectEnd() override;

        bool IsDragSelecting() const override;

        void SignalConnectionDragBegin() override;
        void SignalConnectionDragEnd() override;

        bool IsDraggingConnection() const override;

        void SignalDesplice() override;

        QRectF GetSelectedSceneBoundingArea() const override;
        QRectF GetSceneBoundingArea() const override;

        void SignalLoadStart() override;
        void SignalLoadEnd() override;
        bool IsLoading() const override;
        bool IsPasting() const override;

        void RemoveUnusedNodes() override;
        void RemoveUnusedElements() override;

        void HandleProposalDaisyChainWithGroup(const NodeId& startNode, SlotType slotType, ConnectionType connectionType, const QPoint& screenPoint, const QPointF& focusPoint, AZ::EntityId targetId) override;
        
        void StartNudging(const AZStd::unordered_set<AZ::EntityId>& fixedNodes) override;
        void FinalizeNudging() override;
        void CancelNudging() override;

        AZ::EntityId FindTopmostGroupAtPoint(QPointF scenePoint) override;

        QPointF SignalGenericAddPositionUseBegin() override;
        void SignalGenericAddPositionUseEnd() override;
        ////

        bool AllowContextMenu() const;

        // VisualNotificationBus
        bool OnMousePress(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* mouseEvent) override;
        bool OnMouseRelease(const AZ::EntityId& sourceId, const QGraphicsSceneMouseEvent* mouseEvent) override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& itemId, const AZ::Vector2& position) override;
        ////

        // ViewNotificationBus
        void OnEscape() override;
        void OnViewParamsChanged(const ViewParams& viewParams) override;
        ////

        // MimeDelegateRequestBus
        void AddDelegate(const AZ::EntityId& delegateId) override;
        void RemoveDelegate(const AZ::EntityId& delegateId) override;
        ////

        // SceneBookmarkActionBus
        AZ::u32 GetNewBookmarkCounter() override;
        ////

        // StyleManagerNotificationBus
        void OnStylesLoaded() override;
        ////

        // AssetEditorSettingsNotificationBus
        void OnSettingsChanged() override;
        ////

    protected:

        void ConfigureAndAddGraphicsEffect(GraphicsEffectInterface* graphicsEffect);

        void OnSceneDragEnter(const QMimeData* mimeData);
        void OnSceneDragMoveEvent(const QPointF& scenePoint, const QMimeData* mimeData);
        void OnSceneDropEvent(const QPointF& scenePoint, const QMimeData* mimeData);
        void OnSceneDragExit(const QMimeData* mimeData);

        bool HasActiveMimeDelegates() const;

    private:
        SceneComponent(const SceneComponent&) = delete;

        template<typename Container>
        void InitItems(const Container& entities) const;

        template<typename Container>
        void ActivateItems(const Container& entities);

        template<typename Container>
        void DeactivateItems(const Container& entities);

        template<typename Container>
        void DestroyItems(const Container& entities) const;

        void DestroyGraphicsItem(const GraphicsEffectId& effectId, QGraphicsItem* graphicsItem);

        void InitConnections();
        void NotifyConnectedSlots();
        void OnSelectionChanged();

        void RegisterSelectionItem(const AZ::EntityId& itemId);
        void UnregisterSelectionItem(const AZ::EntityId& itemId);

        void AddSceneMember(const AZ::EntityId& item, bool positionItem = false, const AZ::Vector2& position = AZ::Vector2());
        void RemoveItemFromScene(QGraphicsItem* item);

        //! Seives a set of entity id's into a node, connection and group entityId set based on if they are in the scene
        void SieveSceneMembers(const AZStd::unordered_set<AZ::EntityId>& itemIds, SceneMemberBuckets& buckets) const;

        QPointF GetViewCenterScenePoint() const;

        void OnDragCursorMove(const QPointF& mousePosition);
        void DetermineDragGroupTarget(const QPointF& cursorPoint);
        AZ::EntityId FindGroupTarget(const QPointF& scenePoint, const AZStd::unordered_set<AZ::EntityId>& ignoreElement = {});

        void OnTrySplice();

        void InitiateSpliceToNode(const NodeId& nodeId);
        void InitiateSpliceToConnection(const AZStd::vector<ConnectionId>& connectionIds);

        bool m_allowReset;
        QPointF m_genericAddOffset;

        int m_deleteCount;
        AZStd::string m_copyMimeType;

        AZ::EntityId m_grid;
        AZStd::unordered_map<QGraphicsItem*, AZ::EntityId> m_itemLookup;

        ViewId m_viewId;
        ViewParams m_viewParams;

        MimeDelegateSceneHelper m_mimeDelegateSceneHelper;
        GestureSceneHelper m_gestureSceneHelper;        

        AZStd::unordered_set< QGraphicsItem* > m_hiddenElements;
        GraphData m_graphData;
        
        AZStd::vector< GraphicsEffectId > m_activeParticles;

        AZStd::unordered_set<NodeId> m_queuedEnable;
        AZStd::unordered_set<NodeId> m_queuedDisable;

        AZStd::unordered_set<NodeId> m_queuedVisualEnable;
        AZStd::unordered_set<NodeId> m_queuedVisualDisable;

        AZStd::unordered_set<AZ::EntityId> m_delegates;
        AZStd::unordered_set<AZ::EntityId> m_activeDelegates;
        AZStd::unordered_set<AZ::EntityId> m_interestedDelegates;

        AZStd::unordered_set<AZ::EntityId> m_ignoredDragTargets;
        AZStd::unordered_set<AZ::EntityId> m_draggedGroupableElements;
        AZ::EntityId                       m_dragTargetGroup;
        StateSetter< RootGraphicsItemDisplayState > m_forcedGroupDisplayStateStateSetter;
        StateSetter< AZStd::string > m_forcedLayerStateSetter;

        bool m_isLoading;
        bool m_isPasting;

        AZStd::unique_ptr<GraphCanvasGraphicsScene> m_graphicsSceneUi;

        DragSelectionType m_dragSelectionType;

        bool m_activateScene;
        bool m_isDragSelecting;

        AZ::EntityId m_inputCouplingTarget;
        AZ::EntityId m_outputCouplingTarget;

        AZ::EntityId m_couplingTarget;
        
        AZ::EntityId m_pressedEntity;
        AZ::Vector2  m_originalPosition;

        bool m_forceDragReleaseUndo;
        bool m_isDraggingEntity;

        bool m_isDraggingConnection;

        // Elements for handling with the drag onto objects
        QTimer m_spliceTimer;
        bool m_enableSpliceTracking;
        
        bool m_enableNodeDragConnectionSpliceTracking;
        bool m_enableNodeDragCouplingTracking;

        bool m_enableNodeChainDragConnectionSpliceTracking;

        bool m_enableNudging;
        NodeNudgingController m_nudgingController;
        
        AZ::EntityId m_spliceTarget;

        GraphSubGraph m_selectedSubGraph;

        StateSetter<RootGraphicsItemDisplayState> m_spliceTargetDisplayStateStateSetter;
        StateSetter<RootGraphicsItemDisplayState> m_pressedEntityDisplayStateStateSetter;
        StateSetter<RootGraphicsItemDisplayState> m_couplingEntityDisplayStateStateSetter;
        ////

        AZ::u32 m_bookmarkCounter;

        EditorId m_editorId;
    };

    //! This is the is Qt Ui QGraphicsScene elements that is stored in the GraphCanvas SceneComponent
    //! This class should NOT be serialized
    class GraphCanvasGraphicsScene
        : public QGraphicsScene
    {
    public:
        AZ_TYPE_INFO(GraphCanvasGraphicsScene, "{48C47083-2CF2-4BB5-8058-FF25084FC2AA}");
        AZ_CLASS_ALLOCATOR(GraphCanvasGraphicsScene, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext*) = delete;

        GraphCanvasGraphicsScene(SceneComponent& scene);
        ~GraphCanvasGraphicsScene() = default;

        // Do not allow serialization
        AZ::EntityId GetEntityId() const;
        void SuppressNextContextMenu();

    protected:
        // QGraphicsScene
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
        QList<QGraphicsItem*> itemsAtPosition(const QPoint& screenPos, const QPointF& scenePos, QWidget* widget) const;
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

        void dragEnterEvent(QGraphicsSceneDragDropEvent * event) override;
        void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
        void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
        void dropEvent(QGraphicsSceneDragDropEvent* event) override;
        ////

    private:
        GraphCanvasGraphicsScene(const GraphCanvasGraphicsScene&) = delete;

        void SignalGroupHighlight();
        void CleanupHighlight();

        SceneComponent& m_scene;
        bool m_suppressContextMenu;

        // Elements to make the group highlighting correct.
        AZ::EntityId                       m_contextMenuGroupTarget;
        StateSetter< RootGraphicsItemDisplayState > m_forcedGroupDisplayStateStateSetter;
        StateSetter< AZStd::string > m_forcedLayerStateSetter;
    };
}
