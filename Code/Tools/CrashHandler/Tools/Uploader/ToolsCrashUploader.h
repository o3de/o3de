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

// LY Editor  Crashpad Upload Handler Extension

#pragma once

#include <Uploader/CrashUploader.h>

namespace O3de
{
    class ToolsCrashUploader : public CrashUploader
    {
    public:
        ToolsCrashUploader(int& argcount, char** argv);
        virtual bool CheckConfirmation(const crashpad::CrashReportDatabase::Report& report) override;

        static std::string GetRootFolder();
    };
}