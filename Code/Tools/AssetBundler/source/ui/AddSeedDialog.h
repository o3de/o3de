/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QDialog>
#include <QSharedPointer>
#endif

namespace Ui
{
    class AddSeedDialog;
}

namespace AssetBundler
{
    class GUIApplicationManager;

    class AddSeedDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit AddSeedDialog(
            QWidget* parent,
            const AzFramework::PlatformFlags& enabledPlatforms,
            const AZStd::string& platformSpecificCachePath);
        virtual ~AddSeedDialog() {}

        AZStd::string GetFileName();
        AzFramework::PlatformFlags GetPlatformFlags();

    private:
        void OnBrowseFileButtonPressed();
        void OnPlatformSelectionChanged(const AzFramework::PlatformFlags& selectedPlatforms);

        void FormatRelativePathString(QString& relativePath);

        QSharedPointer<Ui::AddSeedDialog> m_ui;
        QString m_platformSpecificCachePath;
        bool m_isAddSeedDialog = false;

        AZStd::string m_fileName;

        bool m_fileNameIsValid = false;
        bool m_platformIsValid = false;
    };
} // namespace AssetBundler
