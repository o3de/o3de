/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorNavigationUtil.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace LmbrCentral
{
    AZStd::vector<AZStd::string> PopulateAgentTypeList()
    {
        AZStd::vector<AZStd::string> agentTypes;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(
            agentTypes, &AzToolsFramework::EditorRequests::Bus::Events::GetAgentTypes);
        agentTypes.insert(agentTypes.begin(), ""); // insert blank element
        return agentTypes;
    }
} // namespace LmbrCentral
