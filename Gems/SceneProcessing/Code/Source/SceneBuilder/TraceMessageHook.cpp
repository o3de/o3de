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

#include <SceneBuilder/TraceMessageHook.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace SceneBuilder
{
    TraceMessageHook::TraceMessageHook()
    {
        BusConnect();
    }

    TraceMessageHook::~TraceMessageHook()
    {
        BusDisconnect();
    }

    bool TraceMessageHook::OnPrintf(const char* window, [[maybe_unused]] const char* message)
    {
        if (AzFramework::StringFunc::Equal(window, AZ::SceneAPI::Utilities::ErrorWindow))
        {
            AZ_Error(window, false, "%s", message);
            AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::IgnoreNextPrintf, 1);
            return true;
        }
        else if (AzFramework::StringFunc::Equal(window, AZ::SceneAPI::Utilities::WarningWindow))
        {
            AZ_Warning(window, false, "%s", message);
            AssetBuilderSDK::AssetBuilderTraceBus::Broadcast(&AssetBuilderSDK::AssetBuilderTraceBus::Events::IgnoreNextPrintf, 1);
            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace SceneBuilder
