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

#pragma once

#include <AzFramework/Logging/StartupLogSinkReporter.h>

namespace AZ
{
    namespace Debug
    {
        /**
        * A handler for the TraceMessageBus which is meant to collect errors and asserts during CrySystem::Init to display them to the user.
        * It will also elevate all output to CryLogAlways while it is in scope.
        * As such, it assumes that it is being used within the CrySystem library and gEnv and gEnv->pSystem are valid.
        */
        class CrySystemInitLogSink
            : public StartupLogSink
        {
        public:
            CrySystemInitLogSink() = default;

            /**
            * Enables or disables the fatal flags to send to the platform specific message box
            */
            void SetFatalMessageBox(bool enable = true);

            /**
            * Formats the collected error messages into a platform specific message box to display to the user.
            * This expects that a valid gEnv->pSystem exists, and the IOSPlatform has been initialzied.
            * Will also log output messages to whatever medium is possible for OnOutput messages on TraceMessageBus (ex. debug output, logs).
            */
            void DisplayCollectedErrorStrings() const override;

        protected:
            bool m_isMessageBoxFatal = false;
        };
    } // namespace Debug
} // namespace AZ

