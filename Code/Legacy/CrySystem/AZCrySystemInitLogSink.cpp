/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            Trace::Instance().Output(AZ::Debug::Trace::GetDefaultSystemWindow(), "\n==================================================================\n");
            Trace::Instance().Output(AZ::Debug::Trace::GetDefaultSystemWindow(), msgBoxMessage.c_str());
            Trace::Instance().Output(AZ::Debug::Trace::GetDefaultSystemWindow(), "\n==================================================================\n");

            EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "O3DE Initialization Failed", msgBoxMessage.c_str(), false);
        }
    } // namespace Debug
} // namespace AZ
