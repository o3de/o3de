/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QGraphicsWidget>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/RootGraphicsItem.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    //! Base class to handle a bunch of the weird quirky stuff that the NodeFrames need to manage.
    //! Will not paint anything.
    class NodeFrameGraphicsWidget
        : public RootGraphicsItem<QGraphicsWidget>
        , public SceneMemberUIRequestBus::Handler
        , public GeometryNotificationBus::Handler
        , public VisualRequestBus::Handler
        , public NodeNotificationBus::Handler
        , public NodeUIRequestBus::Handler
        , public StyleNotificationBus::Handler
    {
    private:
        enum NodeFrameDisplayState
        {
            None,
            Inspection,
            Deletion
        };

        enum StepAxis
        {
            Unknown,
            Height,
            Width
        };

    public:
        AZ_TYPE_INFO(NodeFrameGraphicsWidget, "{33B9DFFB-9E40-4D55-82A7-85468F7E7790}");
        AZ_CLASS_ALLOCATOR(NodeFrameGraphicsWidget, AZ::SystemAllocator);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        NodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual);
        ~NodeFrameGraphicsWidget() override = default;

        void Activate();
        void Deactivate();

        // RootVisualNotificationsHelper
        QRectF GetBoundingRect() const override;
        ////

        // SceneMemberUIRequestBus
        QGraphicsItem* GetRootGraphicsItem() override;
        QGraphicsLayoutItem* GetRootGraphicsLayoutItem() override;

        void SetSelected(bool selected) override;
        bool IsSelected() const override;

        void SetZValue(qreal zValue) override;
        qreal GetZValue() const override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // QGraphicsItem
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const override;
        void resizeEvent(QGraphicsSceneResizeEvent *) override;
        ////

        // RootGraphicsItem
        void OnDeleteItem() override;
        ////

        // VisualRequestBus
        QGraphicsItem* AsGraphicsItem() override;

        bool Contains(const AZ::Vector2& position) const override;

        void SetVisible(bool visible) override;
        bool IsVisible() const override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;
        void OnAddedToScene(const AZ::EntityId& graphId) override;

        void OnNodeWrapped(const AZ::EntityId& wrappingNode) override;
        ////

        // NodeUIRequestBus
        void AdjustSize() override;

        void SetSteppedSizingEnabled(bool sizing) override;
        void SetSnapToGrid(bool snapToGrid) override;
        void SetResizeToGrid(bool resizeToGrid) override;
        void SetGrid(AZ::EntityId gridId) override;

        qreal GetCornerRadius() const override;
        qreal GetBorderWidth() const override;
        ////        

    protected:

        void SetDisplayState(NodeFrameDisplayState displayState);
        void UpdateDisplayState(NodeFrameDisplayState displayState, bool enabled);

        int GrowToNextStep(int value, int step, StepAxis stepAxis  = StepAxis::Unknown) const;
        int RoundToClosestStep(int value, int step) const;
        int ShrinkToPreviousStep(int value, int step) const;

        virtual void OnActivated();
        virtual void OnDeactivated();
        virtual void OnRefreshStyle();

    protected:
        NodeFrameGraphicsWidget(const NodeFrameGraphicsWidget&) = delete;

        Styling::StyleHelper m_style;
        NodeFrameDisplayState m_displayState;

        bool m_enabledSteppedSizing = true;
        EditorId m_editorId;

        AZ::EntityId m_wrapperNode;
        bool m_isWrapped = false;
    };
}
