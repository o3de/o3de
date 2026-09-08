/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RerouteNode.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Model/Slot.h>

namespace LandscapeCanvas
{
    const char* RerouteNode::TITLE = "Reroute";
    const GraphModel::SlotName RerouteNode::IN_SLOT_ID = "In";
    const GraphModel::SlotName RerouteNode::OUT_SLOT_ID = "Out";

    void RerouteNode::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RerouteNode, BaseNode>()->Version(0);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<RerouteNode>("Reroute", "Routes a connection without changing its data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(GraphModelIntegration::Attributes::TitlePaletteOverride, "UtilityNodeTitlePalette")
                    ->Attribute(GraphModelIntegration::Attributes::NodeStyleOverride, ".reroute")
                    ->Attribute(GraphModelIntegration::Attributes::DataTypePassthrough, "In|Out");
            }
        }
    }

    RerouteNode::RerouteNode(GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* RerouteNode::GetTitle() const
    {
        return TITLE;
    }

    bool RerouteNode::IsVisualOnly() const
    {
        return true;
    }

    void RerouteNode::RegisterSlots()
    {
        const GraphModel::DataTypeList& dataTypes = GetGraphContext()->GetAllDataTypes();
        AZ_Assert(!dataTypes.empty(), "Landscape Canvas reroute requires at least one registered data type.");
        if (dataTypes.empty())
        {
            return;
        }

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Input,
            GraphModel::SlotType::Data,
            IN_SLOT_ID,
            "",
            "Input connection",
            dataTypes,
            dataTypes.front()->GetDefaultValue()));

        RegisterSlot(AZStd::make_shared<GraphModel::SlotDefinition>(
            GraphModel::SlotDirection::Output,
            GraphModel::SlotType::Data,
            OUT_SLOT_ID,
            "",
            "Output connection",
            dataTypes));
    }
} // namespace LandscapeCanvas
