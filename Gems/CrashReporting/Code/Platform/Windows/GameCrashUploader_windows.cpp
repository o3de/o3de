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

#include <AzCore/PlatformIncl.h>

#include <CrashReporting/GameCrashUploader.h>
#include <CrashSupport.h>

#include <stdlib.h>
#include <locale>
#include <codecvt>

#pragma warning(disable : 4996)

namespace O3de
{

    bool GameCrashUploader::CheckConfirmation(const crashpad::CrashReportDatabase::Report& report)
    {
        if (!m_noConfirmation)
        {
            const char* noConfirmation = getenv("LY_NO_CONFIRM");
            if (noConfirmation == nullptr)
            {

                std::wstring sendDialogMessage;

                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                sendDialogMessage = converter.from_bytes(m_executableName);

                sendDialogMessage += L" has encountered a fatal error.  We're sorry for the inconvenience.\n\nA crash debugging file has been created at:\n";
                sendDialogMessage += report.file_path.value();
                sendDialogMessage += L"\n\nIf you are willing to submit this file to Amazon it will help us improve the Lumberyard experience.  We will treat this report as confidential.\n\nWould you like to send the error report?";

                int msgboxID = MessageBoxW(
                    NULL,
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
