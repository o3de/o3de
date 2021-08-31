/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <QMessageBox>
#include <QRegExp>

namespace AzQtComponents
{
    QString FileDialog::GetSaveFileName(QWidget* parent, const QString& caption, const QString& dir,
        const QString& filter, QString* selectedFilter, QFileDialog::Options options)
    {
        bool shouldPromptAgain = false;
        QString filePath;

        do
        {
            // Trigger Qt's save filename dialog
            // If filePath isn't empty, it means we are prompting again because the filename was invalid,
            // so pass it instead of the directory so the filename is pre-filled in for the user
            filePath = QFileDialog::getSaveFileName(parent, caption, (filePath.isEmpty()) ? dir : filePath, filter, selectedFilter, options);

            if (!filePath.isEmpty())
            {
                QFileInfo fileInfo(filePath);
                QString fileName = fileInfo.fileName();

                // Check if the filename has any invalid characters
                QRegExp validFileNameRegex("^[a-zA-Z0-9_\\-./]*$");
                shouldPromptAgain = !validFileNameRegex.exactMatch(fileName);

                // If the filename had invalid characters, then show a warning message and then we will re-prompt the save filename dialog
                if (shouldPromptAgain)
                {
                    QMessageBox::warning(parent, QObject::tr("Invalid filename"),
                        QObject::tr("O3DE assets are restricted to alphanumeric characters, hyphens (-), underscores (_), and dots (.)\n\n%1").arg(fileName));
                }
            }
            else
            {
                // If the filePath is empty, then the user cancelled the dialog so we don't need to prompt again
                shouldPromptAgain = false;
            }
        } while (shouldPromptAgain);

        return filePath;
    }
} // namespace AzQtComponents
