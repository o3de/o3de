/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QGraphicsItem>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Utils/StateControllers/StateController.h>
#include <GraphCanvas/Types/Types.h>

namespace AZ
{
    class Vector2;
}

class QGraphicsLayoutItem;
class QGraphicsLayout;

namespace GraphCanvas
{
    class SceneMemberUIRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Returns the root graphics item that represents the element that should be added to the QGraphicsScene
        virtual QGraphicsItem* GetRootGraphicsItem() = 0;

        //! Returns the QGraphicsItem that represents the element that is selectectable for the visual item.
        virtual QGraphicsItem* GetSelectionItem() { return GetRootGraphicsItem(); }

        // Returns the root graphics item that represents the element as a QGraphicsLayoutItem
        virtual QGraphicsLayoutItem* GetRootGraphicsLayoutItem() = 0;

        //! Returns whether or not the Visual Entity should allow itself to be selected via drag selection.
        virtual bool AllowDragSelection() const { return true; }
        
        virtual void SetSelected(bool selected) = 0;
        virtual bool IsSelected() const = 0;

        virtual QPainterPath GetOutline() const = 0;

        virtual void SetZValue(qreal zValue) = 0;
        virtual qreal GetZValue() const = 0;
    };

    using SceneMemberUIRequestBus = AZ::EBus<SceneMemberUIRequests>;

    //! VisualRequests
    //! Similar to the root visual, which is just the top-level one that will be parented by an owning entity (such as
    //! in the node/slot relationship), every other visual needs to be referenced by means of its Qt interface.
    class VisualRequests : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;


        //! If the visual is a QGraphicsItem, this method will return a pointer to it.
        virtual QGraphicsItem* AsGraphicsItem() = 0;
        //! If the visual is a QGraphicsLayoutItem, this method will return a pointer to that interface.
        //! The default is to return \code nullptr.
        virtual QGraphicsLayoutItem* AsGraphicsLayoutItem() { return nullptr; }

        //! Does the visual contain a given scene coordinate?
        virtual bool Contains(const AZ::Vector2&) const = 0;

        //! Show or hide this element.
        virtual void SetVisible(bool) = 0;
        //! Get the visibility of this element.
        virtual bool IsVisible() const = 0;        
    };

    using VisualRequestBus = AZ::EBus<VisualRequests>;


    //! VisualNotifications
    //! Notifications that provide access to various QGraphicsItem events that are of interest
    class VisualNotifications : public AZ::EBusTraits
    {
    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool OnMousePress(const AZ::EntityId&, const QGraphicsSceneMouseEvent*) { return false; };
        virtual bool OnMouseRelease(const AZ::EntityId&, const QGraphicsSceneMouseEvent*) { return false; };

        virtual bool OnMouseDoubleClick(const QGraphicsSceneMouseEvent*) { return false; };

        virtual void OnItemResized() {};

        //! Forwards QGraphicsItem::onItemChange events to the event bus system.
        //! QGraphicsItems can produce a wide variety of informational events, relating to all sorts of changes in their
        //! state. See QGraphicsItem::itemChange and QGraphicsItem::GraphicsItemChange.
        //!
        //! # Parameters
        //! 1. The entity that has changed.
        //! 2. The type of change.
        //! 3. The value (if any) associated with the change.
        virtual void OnItemChange(const AZ::EntityId&, QGraphicsItem::GraphicsItemChange, const QVariant&) {}

        virtual void OnPositionAnimateBegin() {}
        virtual void OnPositionAnimateEnd() {};
    };

    using VisualNotificationBus = AZ::EBus<VisualNotifications>;

    class RootGraphicsItemRequests : public AZ::EBusTraits
    {
        // There are a few classes which need to manipulate the enabled state. But I don't want it public editable.
        friend class GraphUtils;
        friend class NodeComponent;
        friend class WrapperNodeLayoutComponent;        

    public:
        // Allow any number of handlers per address.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void AnimatePositionTo(const QPointF& scenePoint, const AZStd::chrono::milliseconds& duration) = 0;
        virtual void CancelAnimation() = 0;

        virtual void OffsetBy(const AZ::Vector2& delta) = 0;

        virtual void SignalGroupAnimationStart(AZ::EntityId groupId) = 0;
        virtual void SignalGroupAnimationEnd(AZ::EntityId groupId) = 0;

        virtual StateController<RootGraphicsItemDisplayState>* GetDisplayStateStateController() = 0;
        virtual RootGraphicsItemDisplayState GetDisplayState() const = 0;
        
        virtual RootGraphicsItemEnabledState GetEnabledState() const = 0;
        
        bool IsEnabled() const
        {
            return GetEnabledState() == RootGraphicsItemEnabledState::ES_Enabled;
        }

    private:
        virtual void SetEnabledState(RootGraphicsItemEnabledState enabledState) = 0;
    };

    using RootGraphicsItemRequestBus = AZ::EBus<RootGraphicsItemRequests>;

    class RootGraphicsItemNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnEnabledChanged([[maybe_unused]] RootGraphicsItemEnabledState enabledState) {}
        virtual void OnDisplayStateChanged([[maybe_unused]] RootGraphicsItemDisplayState oldState, [[maybe_unused]] RootGraphicsItemDisplayState newState) {}
    };

    using RootGraphicsItemNotificationBus = AZ::EBus<RootGraphicsItemNotifications>;

}
