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
        return !m_path.isEmpty();
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
        case O3DEFoundation:
            return "Open 3D Foundation";
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
} // namespace O3DE::ProjectManager
