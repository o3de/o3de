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
        const AZ::Data::AssetId& defaultSourceAssetId,
        const AZStd::function<bool(const AZ::Data::AssetInfo&)>& filterCallback,
        QWidget* parent)
        : QDialog(parent)
        , m_sourceLabel(sourceLabel)
        , m_targetLabel(targetLabel)
        , m_initialPath(initialPath)
    {
        setModal(true);
        resize(400, 128);
        setMinimumSize(QSize(400, 128));
        setMaximumSize(QSize(16777215, 128));
        setWindowTitle(title);

        auto sourceSelectionComboBoxLabel = new QLabel(this);
        sourceSelectionComboBoxLabel->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        sourceSelectionComboBoxLabel->setText(sourceLabel);

        auto targetSelectionBrowserLabel = new QLabel(this);
        targetSelectionBrowserLabel->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        targetSelectionBrowserLabel->setText(targetLabel);

        m_sourceSelectionComboBox = new AssetSelectionComboBox(filterCallback, this);
        m_sourceSelectionComboBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        m_sourceSelectionComboBox->SelectAsset(defaultSourceAssetId);
        m_sourcePath = m_sourceSelectionComboBox->GetSelectedAssetSourcePath().c_str();
        QObject::connect(m_sourceSelectionComboBox, &AssetSelectionComboBox::AssetSelected, this, [this]() {
            m_sourcePath = m_sourceSelectionComboBox->GetSelectedAssetSourcePath().c_str();
        });

        m_targetSelectionBrowser = new AzQtComponents::BrowseEdit(this);
        m_targetSelectionBrowser->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        m_targetSelectionBrowser->setLineEditReadOnly(true);

        // Select a default location and unique name for the new document
        UpdateTargetPath(QFileInfo(GetUniqueFilePath(
            AZStd::string::format("%s/untitled.%s", m_initialPath.toUtf8().constData(), supportedExtensions.front().toUtf8().constData())).c_str()));

        // When the file selection button is pressed, open a file dialog to select where the document will be saved
        QObject::connect(m_targetSelectionBrowser, &AzQtComponents::BrowseEdit::attachedButtonTriggered, m_targetSelectionBrowser, [this, supportedExtensions]() {
            UpdateTargetPath(AzQtComponents::FileDialog::GetSaveFileName(this, m_targetLabel, m_targetPath, QString("(*.%1)").arg(supportedExtensions.join(");;(*."))));
        });

        // Connect ok and cancel buttons
        auto buttonBox = new QDialogButtonBox(this);
        buttonBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(sourceSelectionComboBoxLabel);
        verticalLayout->addWidget(m_sourceSelectionComboBox);
        verticalLayout->addWidget(targetSelectionBrowserLabel);
        verticalLayout->addWidget(m_targetSelectionBrowser);
        verticalLayout->addWidget(buttonBox);

        auto gridLayout = new QGridLayout(this);
        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);
    }

    CreateDocumentDialog::CreateDocumentDialog(const DocumentTypeInfo& documentType, const QString& initialPath, QWidget* parent)
        : CreateDocumentDialog(
              tr("Create %1").arg(documentType.m_documentTypeName.c_str()),
              tr("Select Type"),
              tr("Select %1 Path").arg(documentType.m_documentTypeName.c_str()),
              initialPath,
              { documentType.GetDefaultExtensionToSave().c_str() },
              documentType.m_defaultAssetIdToCreate,
              [documentType](const AZ::Data::AssetInfo& assetInfo)
              {
                  // If any asset types are specified, do early rejection tests to avoid expensive string comparisons
                  const auto& assetTypes = documentType.m_supportedAssetTypesToCreate;
                  if (assetTypes.empty() || assetTypes.find(assetInfo.m_assetType) != assetTypes.end())
                  {
                      // Additional filtering will be done against the path to the source file for this asset
                      const auto& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetInfo.m_assetId);

                      // Only add source files with extensions supported by the document types creation rules
                      // Ignore any files that are marked as non editable in the registry
                      return documentType.IsSupportedExtensionToCreate(sourcePath) &&
                          !documentType.IsSupportedExtensionToSave(sourcePath) && IsDocumentPathEditable(sourcePath);
                  }
                  return false;
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
