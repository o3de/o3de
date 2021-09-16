/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoInfo.h>

namespace O3DE::ProjectManager
{
    GemRepoInfo::GemRepoInfo(
        const QString& name, const QString& creator, const QString& summary, const QDateTime& lastUpdated, bool isAdded = true)
        : m_name(name)
        , m_creator(creator)
        , m_summary(summary)
        , m_lastUpdated(lastUpdated)
        , m_isAdded(isAdded)
    {
    }
    
    bool GemRepoInfo::IsValid() const
    {
        return !m_name.isEmpty();
    }

    bool GemRepoInfo::operator<(const GemRepoInfo& gemRepoInfo) const
    {
        return (m_lastUpdated < gemRepoInfo.m_lastUpdated);
    }
} // namespace O3DE::ProjectManager
