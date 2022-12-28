/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/AzQtComponents_Traits_Platform.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <QMessageBox>
#include <QRegularExpression>

namespace AzQtComponents
{
    bool FileDialog::IsValidFileName(const QString& fileName)
    {
        QRegularExpression validFileNameRegex("^[a-zA-Z0-9_\\-./]*$");
        QRegularExpressionMatch validFileNameMatch = validFileNameRegex.match(fileName);
        return validFileNameMatch.hasMatch();
    }

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
            QString localSelectedFilter;
            filePath = QFileDialog::getSaveFileName(parent, caption, (filePath.isEmpty()) ? dir : filePath, filter, &localSelectedFilter, options);
            if (selectedFilter)
            {
                *selectedFilter = localSelectedFilter;
            }

            if (!filePath.isEmpty())
            {
                QFileInfo fileInfo(filePath);
                QString fileName = fileInfo.fileName();

                // If the filename had invalid characters, then show a warning message and then we will re-prompt the save filename dialog
                if (!IsValidFileName(fileName))
                {
                    QMessageBox::warning(parent, QObject::tr("Invalid filename"),
                        QObject::tr("O3DE assets are restricted to alphanumeric characters, hyphens (-), underscores (_), and dots (.)\n\n%1").arg(fileName));
                    shouldPromptAgain = true;
                    continue;
                }
                else
                {
                    shouldPromptAgain = false;
                }
#if AZ_TRAIT_AZQTCOMPONENTS_FILE_DIALOG_APPLY_MISSING_EXTENSION
                // If a filter was selected, then make sure that the resulting filename ends with that extension. On systems that use the default QFileDialog, 
                // the extension is not guaranteed to be set in the resulting filename
                if (FileDialog::ApplyMissingExtension(localSelectedFilter, filePath))
                {
                    // If an extension had to be applied, then the file dialog did not handle the case of overwriting existing files.
                    // We need to check that condition before we proceed
                    QFileInfo updatedFilePath(filePath);

                    if (updatedFilePath.exists())
                    {
                        QMessageBox::StandardButton overwriteSelection = QMessageBox::question(parent, 
                                                                                               QObject::tr("File exists"),
                                                                                               QObject::tr("%1 exists. Do you want to overwrite the existing file?").arg(updatedFilePath.fileName()));
                        shouldPromptAgain = (overwriteSelection == QMessageBox::No);
                    }
                }
#endif // AZ_TRAIT_AZQTCOMPONENTS_FILE_DIALOG_APPLY_MISSING_EXTENSION
            }
            else
            {
                // If the filePath is empty, then the user cancelled the dialog so we don't need to prompt again
                shouldPromptAgain = false;
            }
        } while (shouldPromptAgain);

        return filePath;
    }

    QString FileDialog::GetSaveFileName_(
        QWidget* parent,
        const QString& caption,
        const QString& dir,
        const QString& filter,
        QString* selectedFilter,
        QFileDialog::Options options)
    {
        return GetSaveFileName(parent, caption, dir, filter, selectedFilter, options);
    }

    bool FileDialog::ApplyMissingExtension(const QString& selectedFilter, QString& filePath)
    {
        if (selectedFilter.isEmpty())
        {
            return false;
        }

        // According to the QT documentation for QFileDialog, the selected filter will come in the form
        // <Filter Name> (<filter pattern1> <filter pattern2> .. <filter patternN> )
        // 
        // For example:
        // "Images (*.gif *.png *.jpg)"
        // 
        // Extract the contents of the <filter pattern>(s) inside the parenthesis and split them based on a whitespace or comma
        const QRegularExpression filterContent(".*\\((?<filters>[^\\)]+)\\)");
        QRegularExpressionMatch filterContentMatch = filterContent.match(selectedFilter);
        if (!filterContentMatch.hasMatch())
        {
            return false;
        }
        QString filterExtensionsString = filterContentMatch.captured("filters");
        QStringList filterExtensionsFull = filterExtensionsString.split(" ", Qt::SkipEmptyParts);
        if (filterExtensionsFull.length() <= 0)
        {
            return false;
        }

        // If there are multiple suffixes in the selected filter, then default to the first one if a suffix needs to be appended
        QString defaultSuffix = filterExtensionsFull[0].mid(1);

        // Iterate through the filter patterns to see if the current filename matches
        QFileInfo fileInfo(filePath);
        bool extensionNeeded = true;
        for (const QString& filterExtensionFull : filterExtensionsFull)
        {
            QString wildcardExpression = QRegularExpression::wildcardToRegularExpression(filterExtensionFull);
            QRegularExpression filterPattern(wildcardExpression, AZ_TRAIT_AZQTCOMPONENTS_FILE_DIALOG_FILTER_CASE_SENSITIVITY);
            QRegularExpressionMatch filterPatternMatch = filterPattern.match(fileInfo.fileName());
            if (filterPatternMatch.hasMatch())
            {
                // The filename matches one of the filter patterns already, the extension does not need to be added to the filename
                extensionNeeded = false;
            }
        }
        if (extensionNeeded)
        {
            // If the current (if any) suffix does not match, automatically add the default suffix for the selected filter
            filePath.append(defaultSuffix);
        }
        return extensionNeeded;
    }
} // namespace AzQtComponents
