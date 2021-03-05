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

#include "NameConflictWarning.hxx"

#include <AzCore/std/string/conversions.h>

namespace AzToolsFramework
{
    namespace Layers
    {
        NameConflictWarning::NameConflictWarning(QWidget* parent, const AZStd::unordered_map<AZStd::string, int>& nameConflictMapping)
            : QMessageBox(parent)
        {
            setWindowTitle(tr("Unable to save layers with duplicate names"));
            setIcon(QMessageBox::Warning);

            QString conflicts;
            for (const AZStd::pair<AZStd::string, int>& nameConflict : nameConflictMapping)
            {
                QString layerHierachy = QString(nameConflict.first.c_str());
                int lastOccurrence = layerHierachy.lastIndexOf(".");

                QString entityName = layerHierachy.right(layerHierachy.length() - lastOccurrence - 1);
                QString entityDirectory = lastOccurrence == -1 ? "" : layerHierachy.left(lastOccurrence);
                entityDirectory = entityDirectory.replace(".", replacementStr);
                entityDirectory = lastOccurrence == -1 ? QObject::tr("at the root level") : QObject::tr("in %1").arg(entityDirectory);

                QString currentConflict = QObject::tr("%1 %2s %3\n").arg(nameConflict.second).arg(entityName).arg(entityDirectory);
                conflicts = QObject::tr("%1%2").arg(conflicts).arg(currentConflict);
            }

            setText(QObject::tr("Save failed. Layer names at the same hierarchy level must be unique.\n\n"
                                "%1\n"
                                "Rename these layers and try again.").arg(conflicts));
        }
    }
}
