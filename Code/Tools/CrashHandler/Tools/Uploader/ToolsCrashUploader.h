/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
