/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsItem>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/RootGraphicsItem.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    class GridGraphicsItem;

    class GridVisualComponent
        : public AZ::Component
        , public VisualRequestBus::Handler
        , public SceneMemberUIRequestBus::Handler
        , public GridNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GridVisualComponent, "{9BAEAE14-A2D3-4D60-AEA8-A8BA3E4D5EC9}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        GridVisualComponent();
        ~GridVisualComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_GridVisualService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_VisualService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_GridVisualService"));
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_VisualService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_GridService"));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // VisualRequestBus
        QGraphicsItem* AsGraphicsItem() override;

        bool Contains(const AZ::Vector2&) const override;

        void SetVisible(bool visible) override;
        bool IsVisible() const override;
        ////

        // SceneMemberUIRequestBus
        QGraphicsItem* GetRootGraphicsItem() override;
        QGraphicsLayoutItem* GetRootGraphicsLayoutItem() override;

        void SetSelected(bool selected) override;
        bool IsSelected() const override;

        QPainterPath GetOutline() const override;

        void SetZValue(qreal zValue) override;
        qreal GetZValue() const override;
        ////

        // GridNotificationBus
        void OnMajorPitchChanged(const AZ::Vector2& pitch) override;
        void OnMinorPitchChanged(const AZ::Vector2& pitch) override;
        ////

    private:
        GridVisualComponent(const GridVisualComponent&) = delete;

        AZ::Vector2 m_majorPitch;
        AZ::Vector2 m_minorPitch;

        friend class GridGraphicsItem;
        AZStd::unique_ptr<GridGraphicsItem> m_gridVisualUi;
    };
    
    class GridGraphicsItem
        : public RootGraphicsItem<QGraphicsItem>
        , protected StyleNotificationBus::Handler
    {
        static const int k_stencilScaleFactor;
    public:
        AZ_TYPE_INFO(GridGraphicsItem, "{D483E19C-8046-472B-801D-A6B1A9F2FF33}");
        AZ_CLASS_ALLOCATOR(GridGraphicsItem, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context) = delete;

        GridGraphicsItem(GridVisualComponent& gridVisual);
        ~GridGraphicsItem() override = default;

        void Init();
        void Activate();
        void Deactivate();

        // RootVisualNotificationsHelper
        QRectF GetBoundingRect() const override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // QGraphicsItem
        QRectF boundingRect(void) const override;
        ////

    protected:

        // QGraphicsItem
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////

    private:
        GridGraphicsItem(const GridGraphicsItem&) = delete;

        void CacheStencils();

        Styling::StyleHelper m_style;

        AZStd::fixed_vector< QPixmap*, 4> m_levelOfDetails;

        GridVisualComponent& m_gridVisual;
    };
}
