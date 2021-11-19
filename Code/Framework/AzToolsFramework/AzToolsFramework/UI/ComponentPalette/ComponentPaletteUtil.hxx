/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
