/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialComponentExporter.h>
#include <Material/EditorMaterialComponentUtil.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentExporter
        {
            AZStd::string GetExportPathByAssetId(const AZ::Data::AssetId& assetId, const AZStd::string& materialSlotName)
            {
                AZStd::string exportPath;
                if (assetId.IsValid())
                {
                    exportPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(assetId);
                    AZ::StringFunc::Path::StripExtension(exportPath);
                    exportPath += "_";
                    exportPath += materialSlotName;
                    exportPath += ".";
                    exportPath += AZ::RPI::MaterialSourceData::Extension;
                    AZ::StringFunc::Path::Normalize(exportPath);
                }
                return exportPath;
            }

            bool OpenExportDialog(ExportItemsContainer& exportItems)
            {
                // Sort material entries so they are ordered by name in the table
                AZStd::sort(exportItems.begin(), exportItems.end(),
                    [](const auto& a, const auto& b) { return a.GetMaterialSlotName() < b.GetMaterialSlotName(); });

                QWidget* activeWindow = nullptr;
                AzToolsFramework::EditorWindowRequestBus::BroadcastResult(activeWindow, &AzToolsFramework::EditorWindowRequests::GetAppMainWindow);

                // Constructing a dialog with a table to display all configurable material export items
                QDialog dialog(activeWindow);
                dialog.setWindowTitle("Generate/Manage Source Materials");

                const QStringList headerLabels = { "Material Slot", "Material Filename", "Overwrite" };
                const int MaterialSlotColumn = 0;
                const int MaterialFileColumn = 1;
                const int OverwriteFileColumn = 2;

                // Create a table widget that will be filled with all of the data and options for each exported material
                QTableWidget* tableWidget = new QTableWidget(&dialog);
                tableWidget->setColumnCount(headerLabels.size());
                tableWidget->setRowCount((int)exportItems.size());
                tableWidget->setHorizontalHeaderLabels(headerLabels);
                tableWidget->setSortingEnabled(false);
                tableWidget->setAlternatingRowColors(true);
                tableWidget->setCornerButtonEnabled(false);
                tableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
                tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
                tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
                tableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

                // Force the table to stretch its header to fill the entire width of the dialog
                tableWidget->horizontalHeader()->setSectionResizeMode(MaterialSlotColumn, QHeaderView::ResizeToContents);
                tableWidget->horizontalHeader()->setSectionResizeMode(MaterialFileColumn, QHeaderView::Stretch);
                tableWidget->horizontalHeader()->setSectionResizeMode(OverwriteFileColumn, QHeaderView::ResizeToContents);
                tableWidget->horizontalHeader()->setStretchLastSection(false);

                // Hide row numbers
                tableWidget->verticalHeader()->setVisible(false);

                int row = 0;
                for (ExportItem& exportItem : exportItems)
                {
                    QFileInfo fileInfo(GetExportPathByAssetId(exportItem.GetOriginalAssetId(), exportItem.GetMaterialSlotName()).c_str());

                    // Configuring initial settings based on whether or not the target file already exists
                    exportItem.SetExportPath(fileInfo.absoluteFilePath().toUtf8().constData());
                    exportItem.SetExists(fileInfo.exists());
                    exportItem.SetOverwrite(false);

                    // Populate the table with data for every column
                    tableWidget->setItem(row, MaterialSlotColumn, new QTableWidgetItem());
                    tableWidget->setItem(row, MaterialFileColumn, new QTableWidgetItem());
                    tableWidget->setItem(row, OverwriteFileColumn, new QTableWidgetItem());

                    // Create a check box for toggling the enabled state of this item
                    QCheckBox* materialSlotCheckBox = new QCheckBox(tableWidget);
                    materialSlotCheckBox->setChecked(exportItem.GetEnabled());
                    materialSlotCheckBox->setText(exportItem.GetMaterialSlotName().c_str());
                    tableWidget->setCellWidget(row, MaterialSlotColumn, materialSlotCheckBox);

                    // Create a file picker widget for selecting the save path for the exported material
                    AzQtComponents::BrowseEdit* materialFileWidget = new AzQtComponents::BrowseEdit(tableWidget);
                    materialFileWidget->setLineEditReadOnly(true);
                    materialFileWidget->setClearButtonEnabled(false);
                    materialFileWidget->setEnabled(exportItem.GetEnabled());
                    materialFileWidget->setText(fileInfo.fileName());
                    tableWidget->setCellWidget(row, MaterialFileColumn, materialFileWidget);

                    // Create a check box for toggling the overwrite state of this item
                    QWidget* overwriteCheckBoxContainer = new QWidget(tableWidget);
                    QCheckBox* overwriteCheckBox = new QCheckBox(overwriteCheckBoxContainer);
                    overwriteCheckBox->setChecked(exportItem.GetOverwrite());
                    overwriteCheckBox->setEnabled(exportItem.GetEnabled() && exportItem.GetExists());

                    overwriteCheckBoxContainer->setLayout(new QHBoxLayout(overwriteCheckBoxContainer));
                    overwriteCheckBoxContainer->layout()->addWidget(overwriteCheckBox);
                    overwriteCheckBoxContainer->layout()->setAlignment(Qt::AlignCenter);
                    overwriteCheckBoxContainer->layout()->setContentsMargins(0, 0, 0, 0);

                    tableWidget->setCellWidget(row, OverwriteFileColumn, overwriteCheckBoxContainer);

                    // Whenever the selection is updated, automatically apply the change to the export item
                    QObject::connect(materialSlotCheckBox, &QCheckBox::stateChanged, materialSlotCheckBox, [&exportItem, materialFileWidget, materialSlotCheckBox, overwriteCheckBox]([[maybe_unused]] int state) {
                        exportItem.SetEnabled(materialSlotCheckBox->isChecked());
                        materialFileWidget->setEnabled(exportItem.GetEnabled());
                        overwriteCheckBox->setEnabled(exportItem.GetEnabled() && exportItem.GetExists());
                    });

                    // Whenever the overwrite check box is updated, automatically apply the change to the export item
                    QObject::connect(overwriteCheckBox, &QCheckBox::stateChanged, overwriteCheckBox, [&exportItem, overwriteCheckBox]([[maybe_unused]] int state) {
                        exportItem.SetOverwrite(overwriteCheckBox->isChecked());
                    });

                    // Whenever the browse button is clicked, open a save file dialog in the same location as the current export file setting
                    QObject::connect(materialFileWidget, &AzQtComponents::BrowseEdit::attachedButtonTriggered, materialFileWidget, [&dialog, &exportItem, materialFileWidget, overwriteCheckBox]() {
                        QFileInfo fileInfo = AzQtComponents::FileDialog::GetSaveFileName(&dialog,
                            QString("Select Material Filename"),
                            exportItem.GetExportPath().c_str(),
                            QString("Material (*.material)"),
                            nullptr,
                            QFileDialog::DontConfirmOverwrite);

                        // Only update the export data if a valid path and filename was selected
                        if (!fileInfo.absoluteFilePath().isEmpty())
                        {
                            exportItem.SetExportPath(fileInfo.absoluteFilePath().toUtf8().constData());
                            exportItem.SetExists(fileInfo.exists());
                            exportItem.SetOverwrite(fileInfo.exists());

                            // Update the controls to display the new state
                            materialFileWidget->setText(fileInfo.fileName());
                            overwriteCheckBox->setChecked(exportItem.GetOverwrite());
                            overwriteCheckBox->setEnabled(exportItem.GetEnabled() && exportItem.GetExists());
                        }
                    });

                    ++row;
                }

                tableWidget->sortItems(MaterialSlotColumn);

                // Create the bottom row of the dialog with action buttons for exporting or canceling the operation
                QDialogButtonBox* buttonBox = new QDialogButtonBox(&dialog);
                buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                // Create a heading label for the top of the dialog
                QLabel* labelWidget = new QLabel("\nSelect the material slots that you want to generate new source materials for. Edit the material file name and location using the file picker.\n", &dialog);
                labelWidget->setWordWrap(true);

                QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
                dialogLayout->addWidget(labelWidget);
                dialogLayout->addWidget(tableWidget);
                dialogLayout->addWidget(buttonBox);
                dialog.setLayout(dialogLayout);
                dialog.setModal(true);

                // Forcing the initial dialog size to accomodate typical content.
                // Temporarily settng fixed size because dialog.show/exec invokes WindowDecorationWrapper::showEvent.
                // This forces the dialog to be centered and sized based on the layout of content.
                // Resizing the dialog after show will not be centered and moving the dialog programatically doesn't m0ve the custmk frame. 
                dialog.setFixedSize(500, 200);
                dialog.show();

                // Removing fixed size to allow drag resizing
                dialog.setMinimumSize(0, 0);
                dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

                // Return true if the user press the export button
                return dialog.exec() == QDialog::Accepted;
            }

            bool ExportMaterialSourceData(const ExportItem& exportItem)
            {
                if (!exportItem.GetEnabled() || exportItem.GetExportPath().empty())
                {
                    return false;
                }

                if (exportItem.GetExists() && !exportItem.GetOverwrite())
                {
                    return true;
                }

                EditorMaterialComponentUtil::MaterialEditData editData;
                if (!EditorMaterialComponentUtil::LoadMaterialEditDataFromAssetId(exportItem.GetOriginalAssetId(), editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentExporter", false, "Failed to load material data.");
                    return false;
                }

                if (!EditorMaterialComponentUtil::SaveSourceMaterialFromEditData(exportItem.GetExportPath(), editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentExporter", false, "Failed to save material data.");
                    return false;
                }

                return true;
            }
        } // namespace EditorMaterialComponentExporter
    } // namespace Render
} // namespace AZ
