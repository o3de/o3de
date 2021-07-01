/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PathValidator.h"

#include <QWidget>
#include <QFileInfo>
#include <QDir>

namespace O3DE::ProjectManager
{
    PathValidator::PathValidator(PathMode pathMode, QWidget* parent)
        : QValidator(parent)
        , m_pathMode(pathMode)
    {
    }

    void PathValidator::setAllowEmpty(bool allowEmpty)
    {
        m_allowEmpty = allowEmpty;
    }

    void PathValidator::setPathMode(PathMode pathMode)
    {
        m_pathMode = pathMode;
    }

    QValidator::State PathValidator::validate(QString &text, int &) const
    {
        if(text.isEmpty())
        {
            return m_allowEmpty ? QValidator::Acceptable : QValidator::Intermediate;
        }

        QFileInfo pathInfo(text);
        if(!pathInfo.dir().exists())
        {
            return QValidator::Intermediate;
        }

        switch(m_pathMode) 
        {
        case PathMode::AnyFile://acceptable, as long as it's not an directoy
            return pathInfo.isDir() ? QValidator::Intermediate : QValidator::Acceptable;
        case PathMode::ExistingFile://must be an existing file
            return pathInfo.exists() && pathInfo.isFile() ? QValidator::Acceptable : QValidator::Intermediate;
        case PathMode::ExistingFolder://must be an existing folder
            return pathInfo.exists() && pathInfo.isDir() ? QValidator::Acceptable : QValidator::Intermediate;
        default:
            Q_UNREACHABLE();
        }

        return QValidator::Invalid;
    }

} // namespace O3DE::ProjectManager
