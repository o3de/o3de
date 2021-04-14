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

#include "CrySystem_precompiled.h"
#include <AZCrySystemInitLogSink.h>

#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/std/string/osstring.h>

#include <ISystem.h>

namespace AZ
{
    namespace Debug
    {
        void CrySystemInitLogSink::SetFatalMessageBox(bool enable)
        {
            m_isMessageBoxFatal = enable;
        }

        void CrySystemInitLogSink::DisplayCollectedErrorStrings() const
        {
            if (m_errorStringsCollected.empty())
            {
                return;
            }

            AZ::OSString msgBoxMessage;
            msgBoxMessage.append("O3DE could not initialize correctly for the following reason(s):");

            for (const AZ::OSString& errMsg : m_errorStringsCollected)
            {
                msgBoxMessage.append("\n");
                msgBoxMessage.append(errMsg.c_str());
            }

            Trace::Output(nullptr, "\n==================================================================\n");
            Trace::Output(nullptr, msgBoxMessage.c_str());
            Trace::Output(nullptr, "\n==================================================================\n");

            EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "O3DE Initialization Failed", msgBoxMessage.c_str(), false);
        }
    } // namespace Debug
} // namespace AZ
