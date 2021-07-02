/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <precompiled.h>

#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>
#include <QToolButton>

#include "UpgradeHelper.h"

#include <Asset/Functions/ScriptCanvasFunctionAsset.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <Editor/Settings.h>
#include <Editor/View/Windows/Tools/UpgradeTool/UpgradeHelper.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ui_UpgradeHelper.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>

namespace ScriptCanvasEditor
{
    UpgradeHelper::UpgradeHelper(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent)
        , m_ui(new Ui::UpgradeHelper())
    {
        m_ui->setupUi(this);

        resize(700, 100);

        m_ui->tableWidget->horizontalHeader()->setVisible(false);
        m_ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

        int rows = 0;
        auto& graphsToUpgrade = AZ::Interface<IUpgradeRequests>::Get()->GetGraphsThatNeedManualUpgrade();
        for (auto& assetId : graphsToUpgrade)
        {
            auto assetInfo = ScriptCanvasEditor::AssetHelpers::GetAssetInfo(assetId);

            m_ui->tableWidget->insertRow(rows);

            connect(m_ui->closeButton, &QPushButton::pressed, this, &QDialog::accept);
            connect(m_ui->tableWidget, &QTableWidget::itemDoubleClicked, this, [this, rows, assetId](QTableWidgetItem* item)
                {
                    if (item && item->data(Qt::UserRole).toInt() == rows)
                    {
                        OpenGraph(assetId);
                    }
                }
            );

            auto openGraph = [this, assetId] {
                OpenGraph(assetId);
            };

            QTableWidgetItem* rowName = new QTableWidgetItem(tr(assetInfo.m_relativePath.c_str()));
            rowName->setData(Qt::UserRole, rows);
            m_ui->tableWidget->setItem(rows, 0, rowName);

            QToolButton* rowGoToButton = new QToolButton(this);
            rowGoToButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
            rowGoToButton->setToolTip("Open Graph");
            
            connect(rowGoToButton, &QToolButton::clicked, openGraph);

            m_ui->tableWidget->setCellWidget(rows, 1, rowGoToButton);

            ++rows;
        }
    }

    void UpgradeHelper::OpenGraph(AZ::Data::AssetId assetId)
    {
        // Open the graph in SC editor
        AzToolsFramework::OpenViewPane(/*LyViewPane::ScriptCanvas*/"Script Canvas");
        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());

        if (assetId.IsValid())
        {
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, assetId, -1);
        }

        if (!openOutcome)
        {
            AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
        }

    }

    UpgradeHelper::~UpgradeHelper()
    {
    }

#include <Editor/View/Windows/Tools/UpgradeTool/moc_UpgradeHelper.cpp>

}
