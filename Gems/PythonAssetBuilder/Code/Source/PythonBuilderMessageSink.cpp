/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PythonBuilderMessageSink.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace PythonAssetBuilder
{
    PythonBuilderMessageSink::PythonBuilderMessageSink()
    {
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();
    }

    PythonBuilderMessageSink::~PythonBuilderMessageSink()
    {
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void PythonBuilderMessageSink::OnTraceMessage(AZStd::string_view message)
    {
        if (message.empty() == false)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "%.*s", static_cast<int>(message.size()), message.data());
        }
    }

    void PythonBuilderMessageSink::OnErrorMessage(AZStd::string_view message)
    {
        if (message.empty() == false)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "ERROR: %.*s", static_cast<int>(message.size()), message.data());
        }
    }

    void PythonBuilderMessageSink::OnExceptionMessage(AZStd::string_view message)
    {
        if (message.empty() == false)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "EXCEPTION: %.*s", static_cast<int>(message.size()), message.data());
        }
    }
}
