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

#include "LmbrCentral_precompiled.h"
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