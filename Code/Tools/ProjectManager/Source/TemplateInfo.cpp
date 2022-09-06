/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TemplateInfo.h"

namespace O3DE::ProjectManager
{
    TemplateInfo::TemplateInfo(const QString& path)
        : m_path(path)
    {
    }

    bool TemplateInfo::IsValid() const
    {
        return !m_path.isEmpty() && !m_name.isEmpty();
    }
} // namespace O3DE::ProjectManager
