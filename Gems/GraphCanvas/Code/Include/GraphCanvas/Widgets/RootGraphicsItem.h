/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <type_traits>
#include <AzCore/Platform.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsSceneEvent>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QDebug>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/unordered_set.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Utils/StateControllers/PrioritizedStateController.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QGraphicsItem, "{054358C3-B3D7-4035-9A74-2D7B2741271A}");
}

namespace GraphCanvas
{
    // Number just to cap the movement at a reasonable speed to avoid slow jittery movement
    constexpr float minimumAnimationPixelsPerSecond = 50.0f;

    //! Generates EBus notifications for some QGraphicsItem events.
    template<typename GraphicsItem>
    class RootGraphicsItem
        : public GraphicsItem
        , public ViewSceneNotificationBus::Handler
        , public RootGraphicsItemRequestBus::Handler
        , public StateController<RootGraphicsItemDisplayState>::Notifications::Handler
        , public AZ::TickBus::Handler
    {
        static_assert(std::is_base_of<QGraphicsItem, GraphicsItem>::value, "GraphicsItem must be a descendant of QGraphicsItem");        

    public:

        using GraphicsItem::setAcceptHoverEvents;
        using GraphicsItem::setPos;
        using GraphicsItem::pos;

        RootGraphicsItem(AZ::EntityId itemId)
            : m_resizeToGrid(false)
            , m_snapToGrid(false)
            , m_gridX(1)
            , m_gridY(1)
            , m_animationDuration(0.0f)
            , m_currentAnimationTime(0.0f)
            , m_allowQuickDeletion(true)
            , m_itemId(itemId)
            , m_anchorPoint(0,0)
            , m_enabledState(RootGraphicsItemEnabledState::ES_Enabled)
            , m_forcedStateDisplayState(RootGraphicsItemDisplayState::Neutral)
            , m_internalDisplayState(RootGraphicsItemDisplayState::Neutral)
            , m_actualDisplayState(RootGraphicsItemDisplayState::Neutral)
        {
            setAcceptHoverEvents(true);
            RootGraphicsItemRequestBus::Handler::BusConnect(GetEntityId());
            StateController<RootGraphicsItemDisplayState>::Notifications::Handler::BusConnect(&m_forcedStateDisplayState);

            m_gridSize = AZ::Vector2(aznumeric_cast<float>(m_gridX), aznumeric_cast<float>(m_gridY));
        }

        ~RootGraphicsItem() override = default;

        // QGraphicsItem
        enum
        {
            Type = QGraphicsItem::UserType + 1
        };

        AZ::EntityId GetEntityId() const 
        {
            return m_itemId;
        }

        bool IsSnappedToGrid() const
        {
            return m_snapToGrid;
        }

        bool IsResizedToGrid() const
        {
            return m_resizeToGrid;
        }

        int GetGridXStep() const
        {
            return m_gridX;
        }

        int GetGridYStep() const
        {
            return m_gridY;
        }

        void SetSnapToGridEnabled(bool enabled)
        {
            if (m_snapToGrid != enabled)
            {
                m_snapToGrid = enabled;

                if (m_snapToGrid)
                {
                    GraphicsItem* thisItem = static_cast<GraphicsItem*>(this);
                    thisItem->setPos(CalculatePosition(thisItem->pos()));
                }
            }
        }

        void SetResizeToGridEnabled(bool enabled)
        {
            m_resizeToGrid = enabled;
        }

        void SetGridSize(const AZ::Vector2& gridSize)
        {
            if (gridSize.GetX() >= 0)
            {
                m_gridX = static_cast<unsigned int>(gridSize.GetX());
            }
            else
            {
                m_gridX = 1;
                AZ_Error("VisualNotificationsHelper", false, "Invalid X-Step to snap grid to.");
            }

            if (gridSize.GetY() >= 0)
            {
                m_gridY = static_cast<unsigned int>(gridSize.GetY());
            }
            else
            {
                m_gridY = 1;
                AZ_Error("VisualNotificationsHelper", false, "Invalid Y-Step to snap grid to.");
            }

            m_gridSize = AZ::Vector2(aznumeric_cast<float>(m_gridX), aznumeric_cast<float>(m_gridY));
        }

        void SetAnchorPoint(const AZ::Vector2& anchorPoint)
        {
            m_anchorPoint = anchorPoint;
        }

        // StateController<RootGraphicsItemDisplayState>
        void OnStateChanged([[maybe_unused]] const RootGraphicsItemDisplayState& displayState) override
        {
            UpdateActualDisplayState();
        }
        ////

        // TickBus
        void OnTick(float delta, AZ::ScriptTimePoint) override
        {
            m_currentAnimationTime += delta;

            if (m_currentAnimationTime >= m_animationDuration)
            {
                m_currentAnimationTime = m_animationDuration;
                AZ::TickBus::Handler::BusDisconnect();
                CleanUpAnimation();
            }
            else
            {
                float percentage = m_currentAnimationTime / m_animationDuration;

                AZ::Vector2 position = m_startPoint.Lerp(m_targetPoint, percentage);
                GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, position);
            }
        }
        ////

        // RootGraphicsItemRequestBus
        void AnimatePositionTo(const QPointF& scenePoint, const AZStd::chrono::milliseconds& duration) override
        {
            if (!IsAnimating())
            {
                StartAnimating();
            }

            if (!AZ::TickBus::Handler::BusIsConnected())
            {
                GeometryRequestBus::EventResult(m_startPoint, GetEntityId(), &GeometryRequests::GetPosition);
                AZ::TickBus::Handler::BusConnect();
            }
            else
            {
                float percentage = m_currentAnimationTime / m_animationDuration;

                m_startPoint = m_startPoint.Lerp(m_targetPoint, percentage);
            }
            
            m_rawAnimationTarget = ConversionUtils::QPointToVector(scenePoint);

            if (m_snapToGrid)
            {
                m_targetPoint = ConversionUtils::QPointToVector(CalculatePosition(ConversionUtils::AZToQPoint(m_rawAnimationTarget)));
            }
            else
            {
                m_targetPoint = m_rawAnimationTarget;
            }

            // Maintain a certain 'velocity' for the nodes so they don't like slowly dribble around.
            float minimumDuration = (m_targetPoint - m_startPoint).GetLength();
            minimumDuration /= minimumAnimationPixelsPerSecond;
            
            m_animationDuration = AZStd::min(minimumDuration, duration.count() * 0.001f);
            m_currentAnimationTime = 0.0f;

            GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetAnimationTarget, m_targetPoint);
        }

        void CancelAnimation() override
        {
            m_currentAnimationTime = m_animationDuration;
            CleanUpAnimation();
        }

        void OffsetBy(const AZ::Vector2& delta) override
        {
            if (IsAnimating())
            {
                m_rawAnimationTarget += delta;

                AZ::Vector2 newTarget = m_rawAnimationTarget;

                if (m_snapToGrid)
                {
                    newTarget = ConversionUtils::QPointToVector(CalculatePosition(ConversionUtils::AZToQPoint(m_rawAnimationTarget)));
                }

                if (!newTarget.IsClose(m_targetPoint))
                {
                    m_targetPoint = newTarget;
                    GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetAnimationTarget, m_targetPoint);
                }

                if (!AZ::TickBus::Handler::BusIsConnected())
                {
                    GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetPosition, m_rawAnimationTarget);
                }
            }
            else
            {
                GeometryRequests* geometryRequests = GeometryRequestBus::FindFirstHandler(GetEntityId());

                if (geometryRequests)
                {
                    geometryRequests->SetPosition(geometryRequests->GetPosition() + delta);
                }
            }
        }

        void SignalGroupAnimationStart(AZ::EntityId groupId) override
        {
            if (!IsAnimating())
            {
                StartAnimating();
            }

            m_groupAnimators.insert(groupId);
        }

        void SignalGroupAnimationEnd(AZ::EntityId groupId) override
        {
            size_t eraseCount = m_groupAnimators.erase(groupId);

            if (eraseCount > 0)
            {
                CleanUpAnimation();
            }
        }

        StateController<RootGraphicsItemDisplayState>* GetDisplayStateStateController() override
        {
            return &m_forcedStateDisplayState;
        }

        RootGraphicsItemDisplayState GetDisplayState() const override
        {
            return m_actualDisplayState;
        }

        void SetEnabledState(RootGraphicsItemEnabledState state) override
        {
            if (m_enabledState != state)
            {
                m_enabledState = state;
                OnEnabledStateChanged(state);

                UpdateActualDisplayState();

                RootGraphicsItemNotificationBus::Event(GetEntityId(), &RootGraphicsItemNotifications::OnEnabledChanged, m_enabledState);
            }
        }

        RootGraphicsItemEnabledState GetEnabledState() const override
        {
            return m_enabledState;
        }
        ////

    protected:
        RootGraphicsItem(const RootGraphicsItem&) = delete;

        void SetDisplayState(RootGraphicsItemDisplayState displayState)
        {
            if (m_internalDisplayState != displayState)
            {
                m_internalDisplayState = displayState;
                UpdateActualDisplayState();
            }
        }

        // ViewSceneNotifications
        void OnAltModifier(bool enabled) override
        {
            if (m_allowQuickDeletion)
            {
                if (enabled)
                {
                    SetDisplayState(RootGraphicsItemDisplayState::Deletion);
                }
                else
                {
                    SetDisplayState(RootGraphicsItemDisplayState::Inspection);
                }
            }
        }
        ////
        
        // QGraphicsItem
        void hoverEnterEvent(QGraphicsSceneHoverEvent* hoverEvent) override
        {
            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            ViewSceneNotificationBus::Handler::BusConnect(sceneId);

            if ((hoverEvent->modifiers() & Qt::KeyboardModifier::AltModifier) && IsSelectable())
            {
                SetDisplayState(RootGraphicsItemDisplayState::Deletion);
            }
            else
            {
                SetDisplayState(RootGraphicsItemDisplayState::Inspection);
            }

            GraphicsItem::hoverEnterEvent(hoverEvent);
        }

        void hoverLeaveEvent(QGraphicsSceneHoverEvent* hoverEvent) override
        {
            ViewSceneNotificationBus::Handler::BusDisconnect();

            SetDisplayState(RootGraphicsItemDisplayState::Neutral);

            GraphicsItem::hoverLeaveEvent(hoverEvent);
        }

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override
        {
            if ((event->modifiers() & Qt::KeyboardModifier::AltModifier) && IsSelectable())
            {
                OnDeleteItem();
            }
            else
            {
                bool result = false;
                VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMousePress, GetEntityId(), event);
                if (!result)
                {
                    GraphicsItem::mousePressEvent(event);
                }
            }
        }

        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseRelease, GetEntityId(), event);
            if (!result)
            {
                GraphicsItem::mouseReleaseEvent(event);
            }
        }

        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent) override
        {
            bool result = false;
            VisualNotificationBus::EventResult(result, GetEntityId(), &VisualNotifications::OnMouseDoubleClick, mouseEvent);

            if (!result)
            {
                GraphicsItem::mouseDoubleClickEvent(mouseEvent);
            }
        }

        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override
        {
            if (change == QAbstractGraphicsShapeItem::ItemPositionChange)
            {
                QVariant snappedValue(CalculatePosition(value.toPointF()));

                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, snappedValue);

                return snappedValue;
            }
            else
            {
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnItemChange, GetEntityId(), change, value);
            }

            return GraphicsItem::itemChange(change, value);
        }

        virtual QRectF GetBoundingRect() const = 0;

        int type() const override
        {
            return Type;
        }
        ////

        virtual void OnDeleteItem()
        {
            AZ::EntityId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &SceneMemberRequests::GetScene);

            AZStd::unordered_set<AZ::EntityId> deleteIds = { GetEntityId() };
            SceneRequestBus::Event(graphId, &SceneRequests::Delete, deleteIds);
        }

        virtual void OnEnabledStateChanged(RootGraphicsItemEnabledState enabledState)
        {
            AZ_UNUSED(enabledState);
        }

        void SetAllowQuickDeletion(bool enabled)
        {
            m_allowQuickDeletion = enabled;
        }

    private:

        void UpdateActualDisplayState()
        {
            RootGraphicsItemDisplayState desiredDisplayState = m_internalDisplayState;

            if (m_forcedStateDisplayState.HasState())
            {
                desiredDisplayState = m_forcedStateDisplayState.GetState();
            }
            else if (m_enabledState != RootGraphicsItemEnabledState::ES_Enabled)
            {                
                if (desiredDisplayState <= RootGraphicsItemDisplayState::Disabled)
                {
                    if (m_enabledState == RootGraphicsItemEnabledState::ES_Disabled)
                    {
                        desiredDisplayState = RootGraphicsItemDisplayState::Disabled;
                    }
                    else
                    {
                        desiredDisplayState = RootGraphicsItemDisplayState::PartialDisabled;
                    }
                }
            }

            if (desiredDisplayState != m_actualDisplayState)
            {
                RootGraphicsItemDisplayState oldDisplayState = m_actualDisplayState;

                switch (m_actualDisplayState)
                {
                case RootGraphicsItemDisplayState::Deletion:
                    LeaveDeletionState();
                    break;
                case RootGraphicsItemDisplayState::Disabled:
                    LeaveDisabledState();
                    break;
                case RootGraphicsItemDisplayState::PartialDisabled:
                    LeavePartialDisabledState();
                    break;
                case RootGraphicsItemDisplayState::InspectionTransparent:
                    LeaveInspectionTransparentState();
                    break;
                case RootGraphicsItemDisplayState::Inspection:
                    LeaveInspectionState();
                    break;
                case RootGraphicsItemDisplayState::GroupHighlight:
                    LeaveGroupHighlightState();
                    break;
                case RootGraphicsItemDisplayState::Preview:
                    LeavePreviewState();
                    break;
                case RootGraphicsItemDisplayState::Neutral:
                    LeaveNeutralState();
                    break;
                default:
                    break;
                }

                m_actualDisplayState = desiredDisplayState;

                switch (m_actualDisplayState)
                {
                case RootGraphicsItemDisplayState::Deletion:
                    EnterDeletionState();
                    break;
                case RootGraphicsItemDisplayState::Disabled:
                    EnterDisabledState();
                    break;
                case RootGraphicsItemDisplayState::PartialDisabled:
                    EnterPartialDisabledState();
                    break;
                case RootGraphicsItemDisplayState::InspectionTransparent:
                    EnterInspectionTransparentState();
                    break;
                case RootGraphicsItemDisplayState::Inspection:
                    EnterInspectionState();
                    break;
                case RootGraphicsItemDisplayState::GroupHighlight:
                    EnterGroupHighlightState();
                    break;
                case RootGraphicsItemDisplayState::Preview:
                    EnterPreviewState();
                    break;
                case RootGraphicsItemDisplayState::Neutral:
                    EnterNeutralState();
                    break;
                default:
                    break;
                }

                RootGraphicsItemNotificationBus::Event(GetEntityId(), &RootGraphicsItemNotifications::OnDisplayStateChanged, oldDisplayState, m_actualDisplayState);
            }
        }

        void EnterPreviewState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Preview);
        }

        void LeavePreviewState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Preview);
        }

        void EnterDeletionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Deletion);
        }

        void LeaveDeletionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Deletion);
        }

        void EnterPartialDisabledState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::PartialDisabled);
        }

        void LeavePartialDisabledState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::PartialDisabled);
        }

        void EnterDisabledState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Disabled);
        }

        void LeaveDisabledState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Disabled);
        }

        void EnterInspectionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Hovered);
        }

        void LeaveInspectionState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Hovered);
        }

        void EnterInspectionTransparentState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::InspectionTransparent);
        }

        void LeaveInspectionTransparentState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::InspectionTransparent);
        }

        void EnterGroupHighlightState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::AddSelectorState, Styling::States::Hovered);
        }

        void LeaveGroupHighlightState()
        {
            StyledEntityRequestBus::Event(GetEntityId(), &StyledEntityRequests::RemoveSelectorState, Styling::States::Hovered);
        }

        void EnterNeutralState()
        {
        }

        void LeaveNeutralState()
        {
        }

        QPointF CalculatePosition(QPointF position) const
        {
            if (m_snapToGrid && !AZ::TickBus::Handler::BusIsConnected())
            {
                return GraphUtils::CalculateGridSnapPosition(position, m_anchorPoint, GetBoundingRect(), m_gridSize);
            }
            else
            {
                return GraphUtils::CalculateAnchorPoint(position, m_anchorPoint, GetBoundingRect());
            }
        }

        bool IsAnimating() const
        {
            return !m_groupAnimators.empty() || !AZ::IsClose(m_animationDuration, m_currentAnimationTime, AZ::Constants::FloatEpsilon);
        }

        void StartAnimating()
        {
            VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnPositionAnimateBegin);
            GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetIsPositionAnimating, true);

            GeometryRequestBus::EventResult(m_rawAnimationTarget, GetEntityId(), &GeometryRequests::GetPosition);
            m_startPoint = m_rawAnimationTarget;
        }

        void CleanUpAnimation()
        {
            if (!IsAnimating())
            {
                AZ::TickBus::Handler::BusDisconnect();

                GeometryRequestBus::Event(GetEntityId(), &GeometryRequests::SetIsPositionAnimating, false);
                VisualNotificationBus::Event(GetEntityId(), &VisualNotifications::OnPositionAnimateEnd);

                setPos(CalculatePosition(ConversionUtils::AZToQPoint(m_rawAnimationTarget)));
            }
        }

        bool IsSelectable() const
        {
            return GraphicsItem::flags().testFlag(GraphicsItem::GraphicsItemFlag::ItemIsSelectable);
        }

        bool m_resizeToGrid;
        bool m_snapToGrid;

        unsigned int m_gridX;
        unsigned int m_gridY;
        AZ::Vector2  m_gridSize;
        
        float m_animationDuration;
        float m_currentAnimationTime;

        AZ::Vector2 m_rawAnimationTarget;
        AZ::Vector2 m_targetPoint;
        AZ::Vector2 m_startPoint;

        AZStd::unordered_set<AZ::EntityId> m_groupAnimators;

        bool m_allowQuickDeletion;

        RootGraphicsItemEnabledState    m_enabledState;

        PrioritizedStateController<RootGraphicsItemDisplayState>  m_forcedStateDisplayState;
        RootGraphicsItemDisplayState                           m_internalDisplayState;

        RootGraphicsItemDisplayState                           m_actualDisplayState;

        AZ::EntityId m_itemId;

        AZ::Vector2 m_anchorPoint;
    };
}
