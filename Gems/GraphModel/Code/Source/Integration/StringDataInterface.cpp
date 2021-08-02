/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
