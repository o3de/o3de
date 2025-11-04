/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectExportWorker.h>
#include <ProjectManagerDefs.h>

#include <QDir>
#include <QString>

namespace O3DE::ProjectManager
{
    
    QString ProjectExportWorker::GetO3DECLIString() const
    {
        return QString("scripts/o3de.bat");
    }
    AZ::Outcome<QStringList, QString> ProjectExportWorker::ConstructKillProcessCommandArguments(const QString& pidToKill) const
    {
        return AZ::Success(QStringList { "cmd.exe", "/C", "taskkill", "/pid", pidToKill, "/f", "/t" } );
    }

} // namespace O3DE::ProjectManager
