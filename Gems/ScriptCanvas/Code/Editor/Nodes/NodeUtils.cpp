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
    void CopyTranslationKeyedNameToDatumLabel(const AZ::EntityId& graphCanvasNodeId,
        ScriptCanvas::SlotId scSlotId,
        const AZ::EntityId& graphCanvasSlotId)
    {
        GraphCanvas::TranslationKeyedString name;
        GraphCanvas::SlotRequestBus::EventResult(name, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetTranslationKeyedName);
        if (name.GetDisplayString().empty())
        {
            return;
        }

        // GC node -> SC node.
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scNodeEntityId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (scNodeEntityId.IsValid())
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ScriptCanvas::NodeRequestBus::Event(scNodeEntityId, &ScriptCanvas::NodeRequests::FindModifiableDatumView, scSlotId, datumView);

            datumView.RelabelDatum(name.GetDisplayString());
        }
    }

    void CopySlotTranslationKeyedNamesToDatums(AZ::EntityId graphCanvasNodeId)
    {
        AZStd::vector<AZ::EntityId> graphCanvasSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasSlotIds, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetSlotIds);
        for (AZ::EntityId graphCanvasSlotId : graphCanvasSlotIds)
        {
            AZStd::any* slotUserData{};
            GraphCanvas::SlotRequestBus::EventResult(slotUserData, graphCanvasSlotId, &GraphCanvas::SlotRequests::GetUserData);

            if (auto scriptCanvasSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData))
            {
                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, *scriptCanvasSlotId, graphCanvasSlotId);
            }
        }
    }

    //////////////////////
    // NodeConfiguration
    //////////////////////
    AZStd::string GetCategoryName(const AZ::SerializeContext::ClassData& classData)
    {
        if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
            {
                if (auto data = azrtti_cast<AZ::Edit::AttributeData<const char*>*>(attribute))
                {
                    return data->Get(nullptr);
                }
            }
        }

        return {};
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
