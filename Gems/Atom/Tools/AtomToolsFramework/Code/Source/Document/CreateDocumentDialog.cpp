/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace AtomToolsFramework
{
    CreateDocumentDialog::CreateDocumentDialog(
        const QString& title,
        const QString& sourceLabel,
        const QString& targetLabel,
        const QString& initialPath,
        const QStringList& supportedExtensions,
        const QString& defaultSourcePath,
        const FilterFn& filterFn,
        QWidget* parent)
        : QDialog(parent)
        , m_sourceLabel(sourceLabel)
        , m_targetLabel(targetLabel)
        , m_initialPath(initialPath)
    {
        setModal(true);
        setMinimumWidth(600);
        resize(500, 128);
        setWindowTitle(title);

        // Create the layout for all the widgets to be stacked vertically.
        auto verticalLayout = new QVBoxLayout();

        // The source selection combo box is used to pick from a set of source files or templates that can be used as a starting point or
        // parent for a new document. If there is no filter then no source selection widgets or connections will be made.
        if (filterFn)
        {
            auto sourceSelectionComboBoxLabel = new QLabel(this);
            sourceSelectionComboBoxLabel->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
            sourceSelectionComboBoxLabel->setText(sourceLabel);
            verticalLayout->addWidget(sourceSelectionComboBoxLabel);

            m_sourceSelectionComboBox = new AssetSelectionComboBox(filterFn, this);
            m_sourceSelectionComboBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
            m_sourceSelectionComboBox->SelectPath(defaultSourcePath.toUtf8().constData());
            m_sourcePath = m_sourceSelectionComboBox->GetSelectedPath().c_str();
            QObject::connect(m_sourceSelectionComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
                m_sourcePath = QString::fromUtf8(path.data(), static_cast<int>(path.size()));
            });
            verticalLayout->addWidget(m_sourceSelectionComboBox);
        }

        auto targetSelectionBrowserLabel = new QLabel(this);
        targetSelectionBrowserLabel->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
        targetSelectionBrowserLabel->setText(targetLabel);
        verticalLayout->addWidget(targetSelectionBrowserLabel);

        m_targetSelectionBrowser = new AzQtComponents::BrowseEdit(this);
        m_targetSelectionBrowser->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
        m_targetSelectionBrowser->setLineEditReadOnly(true);
        verticalLayout->addWidget(m_targetSelectionBrowser);

        // Select a default location and unique name for the new document
        UpdateTargetPath(QFileInfo(GetUniqueFilePath(
            AZStd::string::format("%s/untitled.%s", m_initialPath.toUtf8().constData(), supportedExtensions.front().toUtf8().constData())).c_str()));

        // When the file selection button is pressed, open a file dialog to select where the document will be saved
        QObject::connect(m_targetSelectionBrowser, &AzQtComponents::BrowseEdit::attachedButtonTriggered, m_targetSelectionBrowser, [this, supportedExtensions]() {
            UpdateTargetPath(AzQtComponents::FileDialog::GetSaveFileName(this, m_targetLabel, m_targetPath, QString("(*.%1)").arg(supportedExtensions.join(");;(*."))));
        });

        // Connect ok and cancel buttons
        auto buttonBox = new QDialogButtonBox(this);
        buttonBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        verticalLayout->addWidget(buttonBox);

        auto gridLayout = new QGridLayout(this);
        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);
    }

    CreateDocumentDialog::CreateDocumentDialog(const DocumentTypeInfo& documentType, const QString& initialPath, QWidget* parent)
        : CreateDocumentDialog(
              tr("Create %1 Document").arg(documentType.m_documentTypeName.c_str()),
              tr("Select source file, type, or template to create %1 document").arg(documentType.m_documentTypeName.c_str()),
              tr("Select target path to save %1 document").arg(documentType.m_documentTypeName.c_str()),
              initialPath,
              { documentType.GetDefaultExtensionToSave().c_str() },
              documentType.m_defaultDocumentTemplate.c_str(),
              documentType.m_supportedExtensionsToCreate.empty() ?
              FilterFn():
              [documentType](const AZStd::string& path)
              {
                // Only add source files with extensions supported by the document types creation rules
                // Ignore any files that are marked as non editable in the registry
                return documentType.IsSupportedExtensionToCreate(path) && !documentType.IsSupportedExtensionToSave(path) && IsDocumentPathEditable(path);
              },
              parent)
    {
    }

    void CreateDocumentDialog::UpdateTargetPath(const QFileInfo& fileInfo)
    {
        if (!fileInfo.absoluteFilePath().isEmpty())
        {
            m_targetPath = fileInfo.absoluteFilePath();
            m_targetSelectionBrowser->setText(m_targetPath);
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Document/moc_CreateDocumentDialog.cpp>
