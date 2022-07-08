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
        enum class ErrorLevel
        {
            None,
            Notice,
            Warning,
            Error
        };

        bool m_dirty = false;
        AssetServerMode m_cachingMode = AssetServerMode::Inactive;
        AZStd::string m_serverAddress = "";
        RecognizerContainer m_patternContainer;
        ErrorLevel m_errorLevel = ErrorLevel::None;
        AZStd::string m_errorMessage;

        void Reset();
        bool Save(const AZ::IO::Path& projectPath);
    };
}
