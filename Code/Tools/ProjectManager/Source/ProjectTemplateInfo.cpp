/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectTemplateInfo.h"

namespace O3DE::ProjectManager
{
    ProjectTemplateInfo::ProjectTemplateInfo(const QString& path)
        : m_path(path)
    {
    }

    bool ProjectTemplateInfo::IsValid() const
    {
        return !m_path.isEmpty() && !m_name.isEmpty();
    }
} // namespace O3DE::ProjectManager
