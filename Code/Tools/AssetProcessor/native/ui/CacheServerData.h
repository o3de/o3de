/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <native/utilities/AssetUtilEBusHelper.h>
#include <native/utilities/PlatformConfiguration.h>
#include <AzCore/IO/Path/Path.h>
#endif

namespace AssetProcessor
{
    struct CacheServerData
    {
        enum class StatusLevel
        {
            None,   // uninitialized or no state
            Notice, // a notification to inform the user about state changes
            Error,  // system is configured wrong
            Active  // the system is active as a Client or Server
        };

        bool m_dirty = false;
        AssetServerMode m_cachingMode = AssetServerMode::Inactive;
        AZStd::string m_serverAddress = "";
        RecognizerContainer m_patternContainer;
        StatusLevel m_statusLevel = StatusLevel::None;
        AZStd::string m_statusMessage;
        bool m_updateStatus = false;

        void Reset();
        bool Save(const AZ::IO::Path& projectPath);
    };
}
