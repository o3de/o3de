/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qgraphicswidget.h>
#include <qcolor.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Component/TickBus.h>

#include <GraphCanvas/Components/Bookmarks/BookmarkBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Widgets/RootGraphicsItem.h>

namespace GraphCanvas
{
    //! The actual visual graphics item
    class BookmarkAnchorVisualGraphicsWidget
        : public RootGraphicsItem<QGraphicsWidget>
        , public GeometryNotificationBus::Handler
        , public StyleNotificationBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public BookmarkNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(BookmarkAnchorVisualGraphicsWidget, "");
        AZ_CLASS_ALLOCATOR(BookmarkAnchorVisualGraphicsWidget, AZ::SystemAllocator, 0);

        BookmarkAnchorVisualGraphicsWidget(const AZ::EntityId& busId);
        virtual ~BookmarkAnchorVisualGraphicsWidget() = default;

        void SetColor(const QColor& drawColor);

        QPainterPath GetOutline() const;

        // RootGraphicsItem
        QRectF GetBoundingRect() const override;
        ////

        // BookmarkNotifications
        void OnBookmarkTriggered() override;
        void OnBookmarkNameChanged() override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position) override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId& sceneId) override;
        ////

        // QGraphicsWidget
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
        ////

    private:

        QPainterPath m_outline;
        QColor m_drawColor;

        Styling::StyleHelper m_style;

        float m_animationDuration;        
    };

    //! Some sort of visual indicator of a bookmark, just to help focus, and allow for easier changing of bookmark locations
    class BookmarkAnchorVisualComponent
        : public GraphCanvasPropertyComponent
        , public SceneMemberUIRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
        , public BookmarkNotificationBus::Handler
    {
    public:
        // Dummy class retained to avoid crashes on graph processing.
        class BookmarkAnchorVisualComponentSaveData
            : public ComponentSaveData
        {
        public:
            AZ_RTTI(BookmarkAnchorVisualComponentSaveData, "{1EDD7480-8CB5-4656-8B04-00E82ED0063A}", ComponentSaveData);
            AZ_CLASS_ALLOCATOR(BookmarkAnchorVisualComponentSaveData, AZ::SystemAllocator, 0);

            BookmarkAnchorVisualComponentSaveData() = default;
        };

    
        AZ_COMPONENT(BookmarkAnchorVisualComponent, "{AD921E77-962B-417F-88FB-500FA679DFDF}");

        static void Reflect(AZ::ReflectContext* reflectContext);

        BookmarkAnchorVisualComponent();
        ~BookmarkAnchorVisualComponent() = default;

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
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

        // SceneMemberNotifications
        void OnSceneSet(const AZ::EntityId& graphId) override;
        ////

        // BookmarkNotifications
        void OnBookmarkColorChanged() override;
        ////

    private:

        BookmarkAnchorVisualComponent(const BookmarkAnchorVisualComponent&) = delete;
        const BookmarkAnchorVisualComponent& operator=(const BookmarkAnchorVisualComponent&) = delete;
        AZStd::unique_ptr<BookmarkAnchorVisualGraphicsWidget>  m_graphicsWidget;        
    };
}
