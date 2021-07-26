/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
