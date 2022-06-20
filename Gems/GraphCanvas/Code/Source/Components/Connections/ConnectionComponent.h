/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <qgraphicswidget.h>
#include <qgraphicssceneevent.h>
#include <QTimer>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/Notifications/ToastBus.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphicsItems/GraphCanvasSceneEventFilter.h>
#include <GraphCanvas/Utils/StateControllers/StateController.h>

namespace GraphCanvas
{
    class ConnectionEventFilter;

    class ConnectionComponent
        : public AZ::Component
        , public ConnectionRequestBus::Handler
        , public SceneMemberRequestBus::Handler
        , public AZ::TickBus::Handler
        , public ViewNotificationBus::Handler
        , public NodeNotificationBus::Handler
        , public SceneNotificationBus::Handler
    {
    private:
        friend class ConnectionEventFilter;

    protected:

        class ConnectionEndpointAnimator
        {
        public:
            ConnectionEndpointAnimator();

            bool IsAnimating() const;
            void AnimateToEndpoint(const QPointF& startPoint, const Endpoint& endPoint, float timeFrame);

            QPointF GetAnimatedPosition() const;

            bool Tick(float deltaTime);

        private:
            bool m_isAnimating;
            float m_maxTime;
            float m_timer;
            QPointF m_currentPosition;

            QPointF m_startPosition;
            Endpoint m_targetEndpoint;
        };

        enum class DragContext
        {
            Unknown,
            TryConnection,
            MoveSource,
            MoveTarget,
            Connected
        };

        enum class ConnectionMoveResult
        {
            DeleteConnection,
            ConnectionMove,
            NodeCreation
        };

        struct ConnectionCandidate
        {
            Endpoint m_connectableTarget;
            Endpoint m_testedTarget;
        };

    public:
        AZ_COMPONENT(ConnectionComponent, "{14BB1535-3B30-4B1C-8324-D864963FBC76}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateBaseConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& selectorClass);
        static AZ::Entity* CreateGeneralConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& substyle = "");

        ConnectionComponent();
        ConnectionComponent(const Endpoint& sourceEndpoint,const Endpoint& targetEndpoint, bool createModelConnection = true);
        ~ConnectionComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("GraphCanvas_ConnectionService", 0x7ef98865));
            provided.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_ConnectionService", 0x7ef98865));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void Activate() override;
        void Deactivate() override;
        ////

        // NodeNotificationBus
        void OnSlotRemovedFromNode(const AZ::EntityId& slotId) override;
        ////

        // ConnectionRequestBus
        AZ::EntityId GetSourceSlotId() const override;
        AZ::EntityId GetSourceNodeId() const override;
        Endpoint GetSourceEndpoint() const override;
        QPointF GetSourcePosition() const override;
        void StartSourceMove() override;
        void SnapSourceDisplayTo(const Endpoint& sourceEndpoint) override;
        void AnimateSourceDisplayTo(const Endpoint& sourceEndpoint, float connectionTime) override;

        AZ::EntityId GetTargetSlotId() const override;
        AZ::EntityId GetTargetNodeId() const override;
        Endpoint GetTargetEndpoint() const override;
        QPointF GetTargetPosition() const override;
        void StartTargetMove() override;
        void SnapTargetDisplayTo(const Endpoint& targetEndpoint) override;
        void AnimateTargetDisplayTo(const Endpoint& targetEndpoint, float connectionTime) override;

        bool ContainsEndpoint(const Endpoint& endpoint) const override;

        void ChainProposalCreation(const QPointF& scenePos, const QPoint& screenPos, AZ::EntityId groupTarget) override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& oldSceneId) override;

        void SignalMemberSetupComplete() override;

        AZ::EntityId GetScene() const override;
        ////

        // ConnectionRequestBus
        AZStd::string GetTooltip() const override;
        void SetTooltip(const AZStd::string& tooltip) override;

        AZStd::any* GetUserData() override;
        ////

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;
        ////

        // ViewNotificationBus
        void OnEscape() override;
        void OnFocusLost() override;
        ////

        // SceneNotificationBus
        void OnNodeIsBeingEdited(bool isBeingEditeed) override;
        ////

    protected:
        ConnectionComponent(const ConnectionComponent&) = delete;
        const ConnectionComponent& operator=(const ConnectionComponent&) = delete;

        void SetGroupTarget(AZ::EntityId groupTarget);
        
        void FinalizeMove();

        virtual void OnConnectionMoveStart();
        virtual bool OnConnectionMoveCancelled();
        virtual ConnectionMoveResult OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos, AZ::EntityId groupTarget);

        virtual bool AllowNodeCreation() const;

        void CleanupToast();

        void StartMove();
        void StopMove();

        bool UpdateProposal(Endpoint& activePoint, const Endpoint& proposedEndpoint, AZStd::function< void(const AZ::EntityId&, const AZ::EntityId&)> endpointChangedFunctor);

        ConnectionCandidate FindConnectionCandidateAt(const QPointF& scenePos) const;

        void UpdateMovePosition(const QPointF& scenePos);
        void FinalizeMove(const QPointF& scenePos, const QPoint& screenPos, bool chainAddition);

        void DisplayConnectionToolTip(const QPointF& scenePos, const Endpoint& connectionTarget);

        ConnectionValidationTooltip m_validationResult;
        Endpoint m_endpointTooltip;
        AzToolsFramework::ToastId  m_toastId;

        //! The Id of the graph this connection belongs to.
        GraphId m_graphId;

        //! The source endpoint that this connection is from.
        Endpoint m_sourceEndpoint;
        ConnectionEndpointAnimator m_sourceAnimator;

        //! The target endpoint that this connection is to.
        Endpoint m_targetEndpoint;
        ConnectionEndpointAnimator m_targetAnimator;

        //! Information needed to handle the dragging aspect of the connections
        QPointF m_mousePoint;

        DragContext m_dragContext;
        Endpoint m_previousEndPoint;

        AZStd::string m_tooltip;

        ConnectionEventFilter* m_eventFilter = nullptr;

        //! Store custom data for this connection
        AZStd::any m_userData;

        StateSetter<RootGraphicsItemDisplayState> m_nodeDisplayStateStateSetter;
        StateSetter<RootGraphicsItemDisplayState> m_connectionStateStateSetter;

        // Group Interactions
        AZ::EntityId m_groupTarget;
        StateSetter< RootGraphicsItemDisplayState > m_forcedGroupDisplayStateStateSetter;
        StateSetter< AZStd::string > m_forcedLayerStateSetter;
    }; 

    class ConnectionEventFilter
        : public SceneEventFilter
    {
    public:
        AZ_CLASS_ALLOCATOR(ConnectionEventFilter, AZ::SystemAllocator, 0);
        ConnectionEventFilter(ConnectionComponent& connection)
            : SceneEventFilter(nullptr)
            , m_connection(connection)
        {
        }

        bool sceneEventFilter(QGraphicsItem*, QEvent* event) override
        {
            switch (event->type())
            {
            case QEvent::GraphicsSceneMouseMove:
            {
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
                m_connection.UpdateMovePosition(mouseEvent->scenePos());
                return true;
            }
            case QEvent::GraphicsSceneMouseRelease:
            {
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);

                bool chainAddition = (mouseEvent->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

                m_connection.FinalizeMove(mouseEvent->scenePos(), mouseEvent->screenPos(), chainAddition);
                return true;
            }
            default:
                break;
            }

            return false;
        }

    private:
        ConnectionComponent& m_connection;
    };
}
