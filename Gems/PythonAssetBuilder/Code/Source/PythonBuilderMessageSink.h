/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace PythonAssetBuilder
{
    class PythonBuilderMessageSink final
        : public AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PythonBuilderMessageSink, AZ::SystemAllocator, 0);

        PythonBuilderMessageSink();
        ~PythonBuilderMessageSink();

    protected:
        // AzToolsFramework::EditorPythonConsoleNotificationBus
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;
    };
}
