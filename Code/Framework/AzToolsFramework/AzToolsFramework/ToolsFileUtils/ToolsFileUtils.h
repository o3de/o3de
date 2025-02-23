/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/base.h>
#include <QString>

namespace AzToolsFramework
{
    namespace ToolsFileUtils
    {
        AZTF_API bool SetModificationTime(const char* const filename, AZ::u64 modificationTime);

        AZTF_API bool GetFreeDiskSpace(const QString& path, qint64& outFreeDiskSpace);
    }
}
