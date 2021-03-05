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

#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/SearchWidget/SearchWidgetTypes.hxx>
#include <QString>

namespace AZ
{
    class SerializeContext;
};

namespace AzToolsFramework
{
    namespace ComponentPaletteUtil
    {
        using ComponentDataTable = AZStd::map <QString, AZStd::map <QString, const AZ::SerializeContext::ClassData* > >;

        using ComponentIconTable = AZStd::map<const AZ::SerializeContext::ClassData*, QString>;

        // Returns true if the given component provides at least one of the services specified or no services are provided
        bool OffersRequiredServices(
            const AZ::SerializeContext::ClassData* componentClass,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
            const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter
        );

        bool OffersRequiredServices(
            const AZ::SerializeContext::ClassData* componentClass,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter
        );

        // Returns true if the given component is addable by the user
        bool IsAddableByUser(const AZ::SerializeContext::ClassData* componentClass);

        void BuildComponentTables(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::ComponentFilter& componentFilter,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
            const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter,
            ComponentDataTable &componentDataTable,
            ComponentIconTable &componentIconTable
        );

        void BuildComponentTables(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::ComponentFilter& componentFilter,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
            ComponentDataTable &componentDataTable,
            ComponentIconTable &componentIconTable
        );

        // Returns true if any components in the given filter provide any of the services
        // specified and are addable/editable by the user
        bool ContainsEditableComponents(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::ComponentFilter& componentFilter,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
            const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter
        );

        bool ContainsEditableComponents(
            AZ::SerializeContext* serializeContext,
            const AzToolsFramework::ComponentFilter& componentFilter,
            const AZStd::vector<AZ::ComponentServiceType>& serviceFilter
        );

        QRegExp BuildFilterRegExp(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator);
    }
}
