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

// AZ
#include <AzFramework/StringFunc/StringFunc.h>

// Graph Model
#include <GraphModel/Integration/StringDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    StringDataInterface::StringDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    AZStd::string StringDataInterface::GetString() const
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            return slot->GetValue<AZStd::string>();
        }
        else
        {
            return "";
        }
    }
    void StringDataInterface::SetString(const AZStd::string& value)
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            AZStd::string trimValue = value;
            AzFramework::StringFunc::TrimWhiteSpace(trimValue, true, true);

            if (trimValue != slot->GetValue<AZStd::string>())
            {
                slot->SetValue(trimValue);

                GraphCanvas::GraphId graphCanvasSceneId;
                IntegrationBus::BroadcastResult(graphCanvasSceneId, &IntegrationBusInterface::GetActiveGraphCanvasSceneId);
                IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, graphCanvasSceneId);
            }
        }
    }
}
