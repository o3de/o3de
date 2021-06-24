/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Logging/MissingAssetNotificationBus.h>

namespace AzFramework { class LogFile; }

namespace AzFramework
{
    class MissingAssetLogger
        : public MissingAssetNotificationBus::Handler
    {
    public:
        MissingAssetLogger();
        ~MissingAssetLogger();

        void FileMissing(const char* filePath) override;

    private:
        AZStd::unique_ptr<LogFile> m_logFile;
    };
}
