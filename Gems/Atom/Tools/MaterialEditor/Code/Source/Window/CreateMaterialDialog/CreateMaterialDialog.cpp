/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/CreateMaterialDialog/CreateMaterialDialog.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AtomToolsFramework/Util/Util.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>

#include <Atom/Document/MaterialDocumentSettings.h>

namespace MaterialEditor
{
    CreateMaterialDialog::CreateMaterialDialog(QWidget* parent)
        : CreateMaterialDialog(QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@")) + AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials", parent)
    {
    }

    CreateMaterialDialog::CreateMaterialDialog(const QString& path, QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::CreateMaterialDialog)
        , m_path(path)
    {
        m_ui->setupUi(this);

        InitMaterialTypeSelection();
        InitMaterialFileSelection();

        //Connect ok and cancel buttons
        QObject::connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setModal(true);
    }

    void CreateMaterialDialog::InitMaterialTypeSelection()
    {
        //Locate all material type files and add them to the combo box
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [this]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), AZ::RPI::MaterialTypeSourceData::Extension))
            {
                const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(info.m_assetId);
                if (!sourcePath.empty())
                {
                    m_materialTypeFileInfo = QFileInfo(sourcePath.c_str());
                    m_ui->m_materialTypeComboBox->addItem(m_materialTypeFileInfo.baseName(), QVariant(m_materialTypeFileInfo.absoluteFilePath()));
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        //Update the material type file info whenever the combo box selection changes 
        QObject::connect(m_ui->m_materialTypeComboBox, static_cast<void (QComboBox::*)(const int)>(&QComboBox::currentIndexChanged), this, [this]() { UpdateMaterialTypeSelection(); });
        QObject::connect(m_ui->m_materialTypeComboBox, &QComboBox::currentTextChanged, this, [this]() { UpdateMaterialTypeSelection(); });

        m_ui->m_materialTypeComboBox->model()->sort(0, Qt::AscendingOrder); 

        // Select the default material type from settings
        auto settings =
            AZ::UserSettings::CreateFind<MaterialDocumentSettings>(AZ::Crc32("MaterialDocumentSettings"), AZ::UserSettings::CT_GLOBAL);

        const int index = m_ui->m_materialTypeComboBox->findText(settings->m_defaultMaterialTypeName.c_str());
        if (index >= 0)
        {
            m_ui->m_materialTypeComboBox->setCurrentIndex(index);
        }

        UpdateMaterialTypeSelection();
    }

    void CreateMaterialDialog::InitMaterialFileSelection()
    {
        //Select a default location and unique name for the new material
        m_materialFileInfo = AtomToolsFramework::GetUniqueFileInfo(
            m_path +
            AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
            AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

        m_ui->m_materialFilePicker->setLineEditReadOnly(true);
        m_ui->m_materialFilePicker->setText(m_materialFileInfo.fileName());

        //When the file selection button is pressed, open a file dialog to select where the material will be saved
        QObject::connect(m_ui->m_materialFilePicker, &AzQtComponents::BrowseEdit::attachedButtonTriggered, m_ui->m_materialFilePicker, [this]() {
            QFileInfo fileInfo = AzQtComponents::FileDialog::GetSaveFileName(this,
                QString("Select Material Filename"),
                m_materialFileInfo.absoluteFilePath(),
                QString("Material (*.material)"));

            // Reject empty or invalid filenames which indicate user cancellation
            if (!fileInfo.absoluteFilePath().isEmpty())
            {
                m_materialFileInfo = fileInfo;
                m_ui->m_materialFilePicker->setText(m_materialFileInfo.fileName());
            }
        });
    }

    void CreateMaterialDialog::UpdateMaterialTypeSelection()
    {
        const int index = m_ui->m_materialTypeComboBox->currentIndex();
        if (index >= 0)
        {
            const QVariant itemData = m_ui->m_materialTypeComboBox->itemData(index);
            m_materialTypeFileInfo = QFileInfo(itemData.toString());
        }
    }
} // namespace MaterialEditor

#include <Window/CreateMaterialDialog/moc_CreateMaterialDialog.cpp>
