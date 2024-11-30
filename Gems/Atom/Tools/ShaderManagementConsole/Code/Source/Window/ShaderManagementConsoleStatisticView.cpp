/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <Window/ShaderManagementConsoleStatisticView.h>
#include <ShaderManagementConsoleRequestBus.h>

#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>

namespace ShaderManagementConsole
{
    ShaderManagementConsoleStatisticView::ShaderManagementConsoleStatisticView(ShaderVariantStatisticData statisticData, QWidget* parent)
        : QTableWidget(parent)
    {
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setSelectionMode(QAbstractItemView::NoSelection);
        horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        setContextMenuPolicy(Qt::CustomContextMenu);

        m_statisticData = statisticData;
        BuildTable();
    }

    ShaderManagementConsoleStatisticView::~ShaderManagementConsoleStatisticView()
    {
    }

    void ShaderManagementConsoleStatisticView::BuildTable()
    {
        QSignalBlocker blocker(this);

        clear();
        setRowCount(static_cast<int>(m_statisticData.m_shaderVariantUsage.size()));
        setColumnCount(static_cast<int>(m_statisticData.m_shaderOptionUsage.size()));


        int optionColumn = 0;
        for (auto const& optionUsage : m_statisticData.m_shaderOptionUsage)
        {
            AZ::Name optionName = optionUsage.first;
            setHorizontalHeaderItem(optionColumn, new QTableWidgetItem(optionName.GetCStr()));
            optionColumn++;
        }

        int row = 0;
        for (auto const& variantUsage : m_statisticData.m_shaderVariantUsage)
        {
            ShaderVariantInfo info = variantUsage.second;
            auto* countVHeader = new QTableWidgetItem(QString::number(info.m_count));
            countVHeader->setToolTip(tr("Count of materials x shaders using this variant ID"));
            setVerticalHeaderItem(row, countVHeader);

            for (int column = 0; column < columnCount(); ++column)
            {
                QTableWidgetItem* widgetItem = horizontalHeaderItem(column);

                for (auto& shaderOptionDescriptor : info.m_shaderOptionGroup.GetShaderOptionDescriptors())
                {
                    AZ::Name optionName = shaderOptionDescriptor.GetName();
                    AZ::RPI::ShaderOptionValue optionValue = info.m_shaderOptionGroup.GetValue(optionName);

                    if (!optionValue.IsValid())
                    {
                        continue;
                    }
                    AZ::Name valueName = shaderOptionDescriptor.GetValueName(optionValue);
                    QByteArray ba = widgetItem->text().toLocal8Bit();
                    const char* columnTitle = ba.data();

                    if (strcmp(columnTitle, optionName.GetCStr()) == 0)
                    {
                        int count = m_statisticData.m_shaderOptionUsage[optionName][valueName];
                        AZStd::string itemText = AZStd::string::format("%s     %d", valueName.GetCStr(), count);
                        auto* cellWidget = new QTableWidgetItem(itemText.c_str());
                        cellWidget->setToolTip(tr(u8"value \u23B5 usage count of this value"));
                        setItem(row, column, cellWidget);
                        break;
                    }
                }
            }

            row++;
        }

        connect(this, &QTableWidget::customContextMenuRequested, this, &ShaderManagementConsoleStatisticView::ShowContextMenu);    
    }

    void ShaderManagementConsoleStatisticView::ShowContextMenu(const QPoint& pos)
    {
        //QTableWidgetItem* item = itemAt(pos);
        QMenu contextMenu(tr("Context menu"), this);
        QString optionText = horizontalHeaderItem(currentColumn())->text();
        AZ::Name optionName = AZ::Name(optionText.toLocal8Bit().data());

        QAction* action = new QAction(QString(tr("See materials using %1")).arg(optionText), this);
        connect(action, &QAction::triggered, this, [this, optionName]() {
                ShowMaterialList(optionName);
                });
        contextMenu.addAction(action);

        contextMenu.exec(mapToGlobal(pos));
    }

    void ShaderManagementConsoleStatisticView::ShowMaterialList(AZ::Name optionName)
    {
        AZStd::vector<AZ::Data::AssetId> materialAssetIdList;
        ShaderManagementConsoleRequestBus::BroadcastResult(
            materialAssetIdList, &ShaderManagementConsoleRequestBus::Events::GetAllMaterialAssetIds);

        QString materialList;

        QProgressDialog progressDialog(AzToolsFramework::GetActiveWindow());
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setMaximum(static_cast<int>(materialAssetIdList.size()));
        progressDialog.setMaximumWidth(400);
        progressDialog.setMaximumHeight(100);
        progressDialog.setWindowTitle(tr("Gather information from material assets"));
        progressDialog.setLabelText(tr("Gather shader variant information..."));

        int materialCount = 0;
        for (int i = 0; i < materialAssetIdList.size(); ++i)
        {
            AZStd::vector<AZ::RPI::ShaderCollection::Item> shaderItemList;
            ShaderManagementConsoleRequestBus::BroadcastResult(
                shaderItemList, &ShaderManagementConsoleRequestBus::Events::GetMaterialInstanceShaderItems, materialAssetIdList[i]);

            for (auto& shaderItem : shaderItemList)
            {
                bool found = false;
                for (auto& descriptor : shaderItem.GetShaderOptionGroup().GetShaderOptionDescriptors())
                {
                    if (descriptor.GetName() == optionName)
                    {
                        // the material is using this option
                        AZ::IO::Path assetPath = AZ::IO::Path(AZ::RPI::AssetUtils::GetSourcePathByAssetId(materialAssetIdList[i]));
                        materialList += QString(assetPath.Stem().Native().data());
                        materialList += "\n";
                        materialCount++;
                        found = true;
                        break;
                    }
                }

                if (found)
                {
                    break;
                }
            }

            progressDialog.setValue(i);

            if (progressDialog.wasCanceled())
            {
                return;
            }
        }

        progressDialog.close();

        if (!materialList.isEmpty())
        {
            QMessageBox msgBox(AzToolsFramework::GetActiveWindow());
            QString message = QString(tr("%2 materials used %1. Show details for the complete list."))
                                  .arg(optionName.GetCStr()).arg(QString::number(materialCount));
            msgBox.setText(message);
            msgBox.setDetailedText(materialList);
            msgBox.exec();
        }
        else
        {
            QMessageBox msgBox(AzToolsFramework::GetActiveWindow());
            msgBox.setText(tr("There are no material using this option now."));
            msgBox.exec();
        }
    }
}
