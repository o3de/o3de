/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/EBus/EBus.h>

class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
#endif

namespace AzQtComponents
{
    /**
    * DragAndDropContext is used to communicate to listener(s) about drag and drop operations.
    * Every time something happens with drag and drop, the context will be filled with the appropriate
    * information before calls are made.
    * every listener should check the current drag result before overriding it.  
    * only the first may actually answer - the rest are still told about the event occurring so that they can respond appropriately
    */
    class DragAndDropContextBase
    {
    public:
        AZ_CLASS_ALLOCATOR(DragAndDropContextBase, AZ::SystemAllocator);
        // this class is a base class for specific types of drag and drop contexts (such as viewport drag and drop context or others)
        // that have more information about the context of the drag and drop.

        AZ_RTTI(DragAndDropContextBase, "{F9F9CC31-1D1D-4FFE-B2F1-F8104D38E632}");
        virtual ~DragAndDropContextBase() {}
    };

    /**
     * The DragAndDropEvents bus is intended to act as a proxy for objects handling QT Drag and Drop Events on Widgets.
     * Multiple handlers can register with a single Widget name to handle drag and drop events from that name. 
     * It is the handler authors' responsibility to not conflict with others' drag and drop handlers.
     *
     * See http://doc.qt.io/qt-5.8/dnd.html for more complete documentation on QT Drag and Drop.
     *
     * In general, to use this system, listen on the bus at the address of one of the contexts (see who derives from the above
     * context base class to see the available contexts).
     * Each time you get any of the events, always check if the event has already been consumed by some other system
     * by examining the event's "isAccepted()" bool property or the accepted bool passed in.
     * If its already accepted by someone, do not do anything yourself.
     * otherwise you may examine the mimeData attached in event->mimeData() and decide whether its relevant or not to your handler.
     * if it is, accept the event yourself.
     * The context is intentionally not const, allowing higher priority handlers to fill the context so that later ones can use it.
     *
     * Note that this API covers both general dragging (over main windows and other windows of the application such as viewports) as
     * well as list views, tree views, and other item views.
     * Note that item views are a special case and the control generally eats the Begin/End/etc events itself.
     * In that case, the CanDrop and DoDrop handlers are exercised instead of the above.
     */

    class CommonDragAndDropBusTraits : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////
        using BusIdType = AZ::u32;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        // When connecting to a Drag and Drop bus you can set a priority for your connection:
        static constexpr const int s_LowPriority = -10; //! Run this handler after the default handler.
        static constexpr const int s_NormalPriority = 0; //! The default handler.
        static constexpr const int s_HighPriority = 10; //! Run this handler before the default handler.
    };

    class  DragAndDropEvents : public CommonDragAndDropBusTraits
    {
    public:
        //! Override this with one of the above priority values (or a number between them)
        //! to control what order your handler gets invoked relative to the other handlers.
        virtual int GetDragAndDropEventsPriority() const
        {
            return CommonDragAndDropBusTraits::s_NormalPriority;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        inline bool Compare(const DragAndDropEvents* other) const
        {
            return GetDragAndDropEventsPriority() > other->GetDragAndDropEventsPriority();
        }

        //! Sent when a drag and drop action enters a widget.
        //! The context is intentionally non const, so that higher level listeners can add additional
        //! contextual information such as decoding the data and caching it, or partially consuming the data.
        virtual void DragEnter(QDragEnterEvent* /*event*/, DragAndDropContextBase& /*context*/) {}
        
        //! Sent when a drag and drop action is in progress.
        virtual void DragMove(QDragMoveEvent* /*event*/, DragAndDropContextBase& /*context*/) {}
        
        //! Sent when a drag and drop action leaves a widget.
        //! note that when drag leaves a widget, the context is not transmitted since there is no longer
        //! any context.
        virtual void DragLeave(QDragLeaveEvent* /*event*/) {}
        
        //! Sent when a drag and drop action completes.
        virtual void Drop(QDropEvent* /*event*/, DragAndDropContextBase& /*context*/) {}

        //! Sent when a drag and drop action completes. Contains a suggested destination path that can be overridden in the dialog.
        virtual void DropAtLocation(QDropEvent* /*event*/, DragAndDropContextBase& /*context*/, QString& /* path*/)
        {
        }
    };

    using DragAndDropEventsBus = AZ::EBus<DragAndDropEvents>;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // In the case of an Item View (Tree View, List View, tile view, etc),
    // the Qt widget control will absorb the above DragEnter, DragLeave, DragMove, and Drop events, in order
    // highlight the hover item and do other processing.  In this case, after doing so, it emits
    // these below events instead.  If you want to handle drop events that are happening in item views, you
    // should listen to this bus.
    class DragAndDropItemViewEvents : public CommonDragAndDropBusTraits
    {
    public:
        //! Override this with one of the above priority values (or a number between them)
        //! to control what order your handler gets invoked relative to the other handlers.
        virtual int GetDragAndDropItemViewEventsPriority() const
        {
            return CommonDragAndDropBusTraits::s_NormalPriority;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        inline bool Compare(const DragAndDropItemViewEvents* other) const
        {
            return GetDragAndDropItemViewEventsPriority() > other->GetDragAndDropItemViewEventsPriority();
        }

        //! CanDropItemView is Sent on every mouse move to query whether the data in the context is acceptable to drop on a view item.
        //! Note: Set accepted to true if you wish to swallow this event and prevent lower priority listeners from responding.
        //! Conversely, always check accepted and return early if its already true.
        //! The context depends on the sender of the event, but should contain enough information to answer the CanDrop
        //! question.
        //! The context is intentionally non const, so that higher level listeners can add additional
        //! contextual information such as decoding the data and caching it, or partially consuming the data.
        virtual void CanDropItemView(bool& /*accepted*/, DragAndDropContextBase& /*context*/) = 0;

        //! DoDropItemView is sent when a drag and drop action completes in an item view.
        //! See notes for CanDrop above.
        virtual void DoDropItemView(bool& /*accepted*/, DragAndDropContextBase& /*context*/) = 0;
    };

    using DragAndDropItemViewEventsBus = AZ::EBus<DragAndDropItemViewEvents>;

} 
