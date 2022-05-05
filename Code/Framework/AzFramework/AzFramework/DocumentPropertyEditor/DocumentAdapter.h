/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::DocumentPropertyEditor
{
    // Forward declarations
    class DocumentAdapter;
    class RoutingAdapter;

    using DocumentAdapterPtr = AZStd::shared_ptr<DocumentAdapter>;
    using ConstDocumentAdapterPtr = AZStd::shared_ptr<const DocumentAdapter>;

    //! A DocumentAdapter provides an interface for transforming data from an arbitrary
    //! source into a DOM hierarchy that can be viewed and edited by a DocumentPropertyView.
    //!
    //! A DocumentAdapter shall provide hierarchical contents in the form of a DOM structure in a
    //! node-based hierarchy comprised of:
    //! - A root Adapter element that contains any number of child rows. A mininmal correct adapter may be represented
    //!   as an empty Adapter element.
    //! - Row elements, that may contain any number of property editors or rows within. Nested rows will be displayed
    //!   as children within the hierarchy, while all other nodes will be turned into widget representations
    //!   and laid out horizontally within the row.
    //! - Label elements displayed as textual labels within a Row. Labels may have attributes, but no children.
    //! - PropertyEditor elements that display a property editor of an arbitrary type, specified by the mandatory
    //!   "type" attribute. The Document Property View will scan for a registered property editor of this type, and provide
    //!   this node to the property editor for rendering. The contents of a PropertyEditor are dictated by its type.
    class DocumentAdapter
    {
    public:
        using ResetEvent = Event<>;
        using ChangedEvent = Event<const Dom::Patch&>;

        virtual ~DocumentAdapter() = default;

        //! Retrieves the contents of this adapter. This must be an Adapter DOM node.
        //! \see AdapterBuilder for building out this DOM structure.
        virtual Dom::Value GetContents() const = 0;

        //! Connects a listener for the reset event, fired when the contents of this adapter have completely changed.
        //! Any views listening to this adapter will need to call GetContents to retrieve the new contents of the adapter.
        void ConnectResetHandler(ResetEvent::Handler& handler);
        //! Connects a listener for the changed event, fired when the contents of the adapter have changed.
        //! The provided patch contains all the changes provided (i.e. it shall apply cleanly on top of the last
        //! GetContents() result).
        void ConnectChangedHandler(ChangedEvent::Handler& handler);

        //! Sets a router responsible for chaining nested adapters, if supported.
        //! \see RoutingAdapter
        virtual void SetRouter(RoutingAdapter* router, const Dom::Path& route);

    protected:
        //! Subclasses may call this to trigger a ResetEvent and let the view know that GetContents should be requeried.
        //! Where possible, prefer to use NotifyContentsChanged instead.
        void NotifyResetDocument();
        //! Subclasses may call this to trigger a ChangedEvent to notify the view that this adapter's contents have changed.
        //! This patch should apply cleanly on the last result GetContents would have returned after any preceding changed
        //! or reset events.
        void NotifyContentsChanged(const Dom::Patch& patch);

    private:
        ResetEvent m_resetEvent;
        ChangedEvent m_changedEvent;
    };
} // namespace AZ::DocumentPropertyEditor
