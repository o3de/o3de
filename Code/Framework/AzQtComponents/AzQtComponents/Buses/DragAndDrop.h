/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/EBus/EBus.h>

class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

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
        AZ_CLASS_ALLOCATOR(DragAndDropContextBase, AZ::SystemAllocator, 0);
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
     * In general, to use this system, listen on the bus at the address of one of the contexts (see files in AzQtComponents/DragAndDrop/ for contexts)
     * Each time you get any of the events, always check if the event has already been consumed by some other system
     * by examining the event's "isAccepted()" bool property.  If its already accepted by someone, do not do anything yourself.
     * otherwise you may examine the mimeData attached in event->mimeData() and decide whether its relevant or not to your handler.
     * if it is, accept the event yourself.
     * The context is intentionally not const, allowing higher priority handlers to fill the context so that later ones can use it.
     */
    class DragAndDropEvents
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////
        using BusIdType = AZ::u32;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        ///////////////////////////////////////////////////////////////////////

        virtual int GetPriority() const
        {
            // our default handlers will return 0 for this.  You can override default behavior
            // by making sure your handler gets invoked first (higher priority).
            return 0;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        inline bool Compare(const DragAndDropEvents* other) const
        {
            return GetPriority() > other->GetPriority();
        }

        /**
        * Sent when a drag and drop action enters a widget.
        */
        virtual void DragEnter(QDragEnterEvent* /*event*/, DragAndDropContextBase& /*context*/) {}
        
        /**
        * Sent when a drag and drop action is in progress.
        */
        virtual void DragMove(QDragMoveEvent* /*event*/, DragAndDropContextBase& /*context*/) {}
        
        /**
        * Sent when a drag and drop action leaves a widget.
        * note that when drag leaves a widget, the context is not transmitted since there is no longer
        * any context.
        */
        virtual void DragLeave(QDragLeaveEvent* /*event*/) {}
        
        /**
        * Sent when a drag and drop action completes.
        */
        virtual void Drop(QDropEvent* /*event*/, DragAndDropContextBase& /*context*/) {}
    };

    using DragAndDropEventsBus = AZ::EBus<DragAndDropEvents>;

} // namespace AzToolsFramework
