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

                // Check if the filename has any invalid characters
                QRegExp validFileNameRegex("^[a-zA-Z0-9_\\-./]*$");

                // If the filename had invalid characters, then show a warning message and then we will re-prompt the save filename dialog
                if (!validFileNameRegex.exactMatch(fileName))
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
                        QMessageBox::StandardButton overwriteSelection = QMessageBox::warning(parent, 
                                                                                              QObject::tr("File exists"),
                                                                                              QObject::tr("%1 exists. Do you want to overwrite the existing file?").arg(updatedFilePath.fileName()),
                                                                                              QMessageBox::Yes | QMessageBox::No);
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

    bool FileDialog::ApplyMissingExtension(const QString& selectedFilter, QString& filePath)
    {
        if (selectedFilter.isEmpty())
        {
            return false;
        }

        // Parse out the filter extension(s) which will exist inside parenthesis, and will be comma or space delimited.
        int firstParen = selectedFilter.lastIndexOf('(');
        int lastParen = selectedFilter.lastIndexOf(')');
        if ((firstParen<0) || (lastParen<=firstParen))
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
        const QRegExp filterContentSeperators("[\\s,]");
        int filterExtensionsLength = lastParen - firstParen - 1;
        QString filterExtensionsString = selectedFilter.mid(firstParen + 1, filterExtensionsLength);
        QStringList filterExtensionsFull = filterExtensionsString.split(filterContentSeperators, Qt::SkipEmptyParts);
        if (filterExtensionsFull.length() <= 0)
        {
            return false;
        }

        // If there are multiple suffixes in the selected filter, then default to the first one if a suffix needs to be appended
        QFileInfo   fileInfo(filePath);
        QString     defaultSuffix = filterExtensionsFull[0].mid(1);

        // Compare the suffix (if any) to the suffixes in the current selected filter to determine if it needs to be added or not
        QString     currentSuffix = fileInfo.completeSuffix();
        bool        extensionNeeded = true;
        for (const QString& filterExtensionFull: filterExtensionsFull)
        {
            QRegExp filterPattern(filterExtensionFull, 
                                  AZ_TRAIT_AZQTCOMPONENTS_FILE_DIALOG_FILTER_CASE_SENSITIVITY, 
                                  AZ_TRAIT_AZQTCOMPONENTS_FILE_DIALOG_FILTER_REGEX_SYNTAX);

            if (filterPattern.exactMatch(fileInfo.fileName()))
            {
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
