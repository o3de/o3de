/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
