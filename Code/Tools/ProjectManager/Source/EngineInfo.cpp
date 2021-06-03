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

#include "EngineInfo.h"

namespace O3DE::ProjectManager
{
    EngineInfo::EngineInfo(const QString& path, const QString& name, const QString& version, const QString& thirdPartyPath)
        : m_path(path)
        , m_name(name)
        , m_version(version)
        , m_thirdPartyPath(thirdPartyPath)
    {
    }

    bool EngineInfo::IsValid() const
    {
        return !m_path.isEmpty();
    }
} // namespace O3DE::ProjectManager
