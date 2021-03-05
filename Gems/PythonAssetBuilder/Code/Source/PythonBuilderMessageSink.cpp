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
