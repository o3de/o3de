/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

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

        //! Clear all inspector groups and content
        virtual void Reset() = 0;

        //! Called before all groups are added
        virtual void AddGroupsBegin() = 0;

        //! Called after all groups are added
        virtual void AddGroupsEnd() = 0;

        //! Add a group consisting of a collapsable header and widget
        virtual void AddGroup(
            const AZStd::string& groupNameId,
            const AZStd::string& groupDisplayName,
            const AZStd::string& groupDescription,
            InspectorGroupWidget* groupWidget) = 0;

        //! Calls Refresh for a specific InspectorGroupWidget, allowing for non-destructive UI changes
        virtual void RefreshGroup(const AZStd::string& groupNameId) = 0;

        //! Calls Rebuild for a specific InspectorGroupWidget, allowing for destructive UI changes
        virtual void RebuildGroup(const AZStd::string& groupNameId) = 0;

        //! Calls Refresh for all InspectorGroupWidget, allowing for non-destructive UI changes
        virtual void RefreshAll() = 0;

        //! Calls Rebuild for all InspectorGroupWidget, allowing for destructive UI changes
        virtual void RebuildAll() = 0;
    };

    using InspectorRequestBus = AZ::EBus<InspectorRequests>;
} // namespace AtomToolsFramework
