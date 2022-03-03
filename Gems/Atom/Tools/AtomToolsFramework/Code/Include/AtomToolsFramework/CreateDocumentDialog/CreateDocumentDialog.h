/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

#include <QDialog>
#include <QFileInfo>
#include <QString>

namespace AtomToolsFramework
{
    //! Dialogue to select parameters for creating a new document from an input document or template file
    class CreateDocumentDialog : public QDialog
    {
        Q_OBJECT
    public:
        CreateDocumentDialog(
            const QString& title,
            const QString& sourceLabel,
            const QString& targetLabel,
            const QString& initialPath,
            const QStringList& supportedExtensions,
            const AZ::Data::AssetId& defaultSourceAssetId,
            const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback,
            QWidget* parent = nullptr);
        ~CreateDocumentDialog() = default;

        void UpdateTargetPath(const QFileInfo& fileInfo);

        QString m_sourcePath;
        QString m_targetPath;

    private:
        QString m_sourceLabel;
        QString m_targetLabel;
        QString m_initialPath;
        AtomToolsFramework::AssetSelectionComboBox* m_sourceSelectionComboBox = {};
        AzQtComponents::BrowseEdit* m_targetSelectionBrowser = {};
    };
} // namespace AtomToolsFramework
