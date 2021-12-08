/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QScrollBar>
#include <QToolButton>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Settings.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ModelTraits.h>
#include <Editor/View/Windows/Tools/UpgradeTool/UpgradeHelper.h>
#include <Editor/View/Windows/Tools/UpgradeTool/UpgradeHelper.h>
#include <Editor/View/Windows/Tools/UpgradeTool/ui_UpgradeHelper.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

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
        const VersionExplorer::ModificationResults* result = nullptr;
        VersionExplorer::ModelRequestsBus::BroadcastResult(result, &VersionExplorer::ModelRequestsTraits::GetResults);

        if (result && !result->m_failures.empty())
        {

            for (auto& failedUpdate : result->m_failures)
            {
                auto asset = failedUpdate.asset;
                
                m_ui->tableWidget->insertRow(rows);

                connect(m_ui->closeButton, &QPushButton::pressed, this, &QDialog::accept);
                connect(m_ui->tableWidget, &QTableWidget::itemDoubleClicked, this, [this, rows, asset](QTableWidgetItem* item)
                    {
                        if (item && item->data(Qt::UserRole).toInt() == rows)
                        {
                            OpenGraph(asset);
                        }
                    }
                );

                auto openGraph = [this, asset] {
                    OpenGraph(asset);
                };

                QTableWidgetItem* rowName = new QTableWidgetItem(tr(asset.Path().c_str()));
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
    }

    void UpgradeHelper::OpenGraph(const SourceHandle& asset)
    {
        // Open the graph in SC editor
        AzToolsFramework::OpenViewPane(/*LyViewPane::ScriptCanvas*/"Script Canvas");
        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());

        if (!asset.Path().empty())
        {
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, asset, Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
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
