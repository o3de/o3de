/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QDialog>
#include <QFileDialog>
#include <QSharedPointer>
#endif

namespace Ui
{
    class NewFileDialog;
}

namespace AssetBundler
{
    class GUIApplicationManager;

    class NewFileDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit NewFileDialog(
            QWidget* parent,
            const QString& dialogTitle,
            const QString& startingPath,
            const char* fileExtension,
            const QString& fileNameFilter,
            const AzFramework::PlatformFlags& enabledPlatforms,
            bool isRunningRule = false);
        virtual ~NewFileDialog() {}

        AZStd::string GetAbsoluteFilePath();
        AzFramework::PlatformFlags GetPlatformFlags();

        //! A standard OS-specific New File Dialog, but blocks all Qt signals from the dialog and does NOT create a new file.
        //! Use in place of the static QFileDialog functions to avoid unexpected file watcher updates.
        //! Returns the absolute path of the file the user either selected or attempted to create, or an empty string if the user canceled out of the dialog. 
        static AZStd::string OSNewFileDialog(
            QWidget* parent,
            const char* fileExtension,
            const char* fileTypeDisplayName,
            const AZStd::string& startingDirectory);

        static int FileGenerationResultMessageBox(
            QWidget* parent,
            const AZStd::vector<AZStd::string>& generatedFiles,
            bool generatedWithErrors);

    private:
        void OnBrowseButtonPressed();
        void OnPlatformSelectionChanged(const AzFramework::PlatformFlags& selectedPlatforms);
        void OnCreateFileButtonPressed();

        QSharedPointer<Ui::NewFileDialog> m_ui;
        QString m_startingPath;
        const char* m_fileExtension;
        QFileDialog m_newFileDialog;

        AZStd::string m_absoluteFilePath;

        bool m_fileNameIsValid = false;
        bool m_platformIsValid = false;
    };
} // namespace AssetBundler
