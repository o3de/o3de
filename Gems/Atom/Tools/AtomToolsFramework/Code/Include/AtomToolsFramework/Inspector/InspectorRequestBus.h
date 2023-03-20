/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace AtomToolsFramework
{
    class InspectorGroupWidget;

    class InspectorRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Add heading widget above scroll area
        virtual void AddHeading(QWidget* headingWidget) = 0;

        //! Clear heading widgets
        virtual void ClearHeading() = 0;

        //! Clear all inspector groups and content
        virtual void Reset() = 0;

        //! Called before all groups are added
        virtual void AddGroupsBegin() = 0;

        //! Called after all groups are added
        virtual void AddGroupsEnd() = 0;

        //! Add a group consisting of a collapsable header and widget
        virtual void AddGroup(
            const AZStd::string& groupName,
            const AZStd::string& groupDisplayName,
            const AZStd::string& groupDescription,
            QWidget* groupWidget) = 0;

        //! Sets the visibility of a specific property group. This impacts both the header and the widget.
        virtual void SetGroupVisible(const AZStd::string& groupName, bool visible) = 0;
        
        //! Returns whether a specific property is visible.
        //! Note this follows the same rules as QWidget::isVisible(), meaning a group could be not visible due to the widget's parents being not visible.
        virtual bool IsGroupVisible(const AZStd::string& groupName) const = 0;
        
        //! Returns whether a specific property is explicitly hidden.
        //! Note this follows the same rules as QWidget::isHidden(), meaning a group that is hidden will not become visible automatically when the parent becomes visible.
        virtual bool IsGroupHidden(const AZStd::string& groupName) const = 0;

        //! Calls Refresh for a specific InspectorGroupWidget, allowing for non-destructive UI changes
        virtual void RefreshGroup(const AZStd::string& groupName) = 0;

        //! Calls Rebuild for a specific InspectorGroupWidget, allowing for destructive UI changes
        virtual void RebuildGroup(const AZStd::string& groupName) = 0;

        //! Calls Refresh for all InspectorGroupWidget, allowing for non-destructive UI changes
        virtual void RefreshAll() = 0;

        //! Calls Rebuild for all InspectorGroupWidget, allowing for destructive UI changes
        virtual void RebuildAll() = 0;

        //! Expands a specific group
        virtual void ExpandGroup(const AZStd::string& groupName) = 0;

        //! Collapses a specific group
        virtual void CollapseGroup(const AZStd::string& groupName) = 0;

        //! Checks the expansion state of a specific group
        virtual bool IsGroupExpanded(const AZStd::string& groupName) const = 0;

        //! Expands all groups and headers
        virtual void ExpandAll() = 0;

        //! Collapses all groups and headers
        virtual void CollapseAll() = 0;
    };

    using InspectorRequestBus = AZ::EBus<InspectorRequests>;
} // namespace AtomToolsFramework
