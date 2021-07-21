/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GemInfo.h"

namespace O3DE::ProjectManager
{
    GemInfo::GemInfo(const QString& name, const QString& creator, const QString& summary, Platforms platforms, bool isAdded)
        : m_name(name)
        , m_creator(creator)
        , m_summary(summary)
        , m_platforms(platforms)
        , m_isAdded(isAdded)
    {
    }
    
    bool GemInfo::IsValid() const
    {
        return !m_name.isEmpty() && !m_path.isEmpty();
    }

    QString GemInfo::GetPlatformString(Platform platform)
    {
        switch (platform)
        {
        case Android:
            return "Android";
        case iOS:
            return "iOS";
        case Linux:
            return "Linux";
        case macOS:
            return "macOS";
        case Windows:
            return "Windows";
        default:
            return "<Unknown Platform>";
        }
    }

    QString GemInfo::GetTypeString(Type type)
    {
        switch (type)
        {
        case Asset:
            return "Asset";
        case Code:
            return "Code";
        case Tool:
            return "Tool";
        default:
            return "<Unknown Type>";
        }
    }

    QString GemInfo::GetGemOriginString(GemOrigin origin)
    {
        switch (origin)
        {
        case Open3DEEngine:
            return "Open 3D Engine";
        case Local:
            return "Local";
        default:
            return "<Unknown Gem Origin>";
        }
    }

    bool GemInfo::IsPlatformSupported(Platform platform) const
    {
        return (m_platforms & platform);
    }

    bool GemInfo::operator<(const GemInfo& gemInfo) const
    {
        return (m_displayName < gemInfo.m_displayName);
    }
} // namespace O3DE::ProjectManager
