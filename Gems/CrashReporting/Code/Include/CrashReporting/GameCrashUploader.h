/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <Uploader/CrashUploader.h>

namespace O3de
{
    class GameCrashUploader : public CrashUploader
    {
    public:
        GameCrashUploader(int& argcount, char** argv);
        bool CheckConfirmation(const crashpad::CrashReportDatabase::Report& report) override;

        static std::string GetRootFolder();
    };
}
