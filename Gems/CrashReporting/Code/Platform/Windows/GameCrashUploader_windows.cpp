/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

#include <CrashReporting/GameCrashUploader.h>
#include <CrashSupport.h>

namespace O3de
{

    bool GameCrashUploader::CheckConfirmation(const crashpad::CrashReportDatabase::Report& report)
    {
        if (!m_noConfirmation)
        {
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
            char noConfirmation[64]{};
            size_t variableSize = 0;
            getenv_s(&variableSize, noConfirmation, AZ_ARRAY_SIZE(noConfirmation), "LY_NO_CONFIRM");
            if (variableSize == 0)
#else
            const char* noConfirmation = getenv("LY_NO_CONFIRM");
            if (noConfirmation == nullptr)
#endif
            
            {
                AZStd::wstring sendDialogMessage;
                AZStd::to_wstring(sendDialogMessage, m_executableName.c_str());

                sendDialogMessage += L" has encountered a fatal error.  We're sorry for the inconvenience.\n\nA crash debugging file has been created at:\n";
                sendDialogMessage += report.file_path.value().c_str();
                sendDialogMessage += L"\n\nIf you are willing to submit this file to Amazon it will help us improve the Lumberyard experience.  We will treat this report as confidential.\n\nWould you like to send the error report?";

                int msgboxID = MessageBoxW(
                    nullptr,
                    sendDialogMessage.data(),
                    L"Send Error Report",
                    (MB_ICONEXCLAMATION | MB_YESNO | MB_SYSTEMMODAL)
                );

                if (msgboxID == IDNO)
                {
                    return false;
                }
            }
        }
        return true;
    }
}
