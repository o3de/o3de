/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeUtils.h"

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>

#include <ScriptCanvas/Core/NodeBus.h>

namespace ScriptCanvasEditor::Nodes
{
    void UpdateSlotDatumLabel(const AZ::EntityId& graphCanvasNodeId, ScriptCanvas::SlotId scSlotId, const AZStd::string& name)
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scNodeEntityId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (scNodeEntityId.IsValid())
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ScriptCanvas::NodeRequestBus::Event(scNodeEntityId, &ScriptCanvas::NodeRequests::FindModifiableDatumView, scSlotId, datumView);

            datumView.RelabelDatum(name);
        }
    }

    void UpdateSlotDatumLabels(AZ::EntityId graphCanvasNodeId)
    {
        AZStd::vector<AZ::EntityId> graphCanvasSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetSlotIds);
        for (AZ::EntityId graphCanvasSlotId : graphCanvasSlotIds)
        {
            AZStd::any* slotUserData{};
            GraphCanvas::SlotRequestBus::EventResult(slotUserData, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetUserData);

            if (auto scriptCanvasSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData))
            {
                AZStd::string slotName;
                GraphCanvas::SlotRequestBus::EventResult(slotName, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetName);
                UpdateSlotDatumLabel(graphCanvasNodeId, *scriptCanvasSlotId, slotName);
            }
        }
    }


    AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData)
    {
        if (auto editorDataElement = classData.m_editData ? classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr)
        {
            if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
            {
                if (auto data = azrtti_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                {
                    AZStd::string fullCategoryName = data->Get(nullptr);
                    AZStd::string delimiter = "/";
                    AZStd::vector<AZStd::string> results;
                    AZStd::tokenize(fullCategoryName, delimiter, results);
                    return results.back();
                }
            }
        }

        return {};
    }
}
