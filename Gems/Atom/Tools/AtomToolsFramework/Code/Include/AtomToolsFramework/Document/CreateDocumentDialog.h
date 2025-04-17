/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>

#include <QDialog>
#include <QFileInfo>
#include <QString>

namespace AtomToolsFramework
{
    //! CreateDocumentDialog allows the user to select from a filtered set of template documents to create a derived/child document at the
    //! path selected in the file picker.
    class CreateDocumentDialog : public QDialog
    {
        Q_OBJECT
    public:
        using FilterFn = AZStd::function<bool(const AZStd::string&)>;

        CreateDocumentDialog(
            const QString& title,
            const QString& sourceLabel,
            const QString& targetLabel,
            const QString& initialPath,
            const QStringList& supportedExtensions,
            const QString& defaultSourcePath,
            const FilterFn& filterFn,
            QWidget* parent = nullptr);

        CreateDocumentDialog(const DocumentTypeInfo& documentType, const QString& initialPath, QWidget* parent = nullptr);

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
