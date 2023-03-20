/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
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
