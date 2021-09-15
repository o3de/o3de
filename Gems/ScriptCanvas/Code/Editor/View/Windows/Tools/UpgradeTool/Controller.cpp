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

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Editor/Settings.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Controller.h>
#include <Editor/View/Windows/Tools/UpgradeTool/LogTraits.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

#include <Editor/View/Windows/Tools/UpgradeTool/ui_Controller.h>
#include <Editor/View/Windows/Tools/UpgradeTool/moc_Controller.cpp>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Controller::Controller(QWidget* parent)
            : AzQtComponents::StyledDialog(parent)
            , m_view(new Ui::Controller())
        {
            m_view->setupUi(this);
            m_view->tableWidget->horizontalHeader()->setVisible(false);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
            m_view->tableWidget->setColumnWidth(3, 22);
            m_view->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
            m_view->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
            connect(m_view->scanButton, &QPushButton::pressed, this, &Controller::OnScanButtonPress);
            connect(m_view->closeButton, &QPushButton::pressed, this, &Controller::OnCloseButtonPress);
            connect(m_view->upgradeAllButton, &QPushButton::pressed, this, &Controller::OnUpgradeButtonPress);
            m_view->progressBar->setValue(0);
            m_view->progressBar->setVisible(false);

            ModelNotificationsBus::Handler::BusConnect();
        }

        void Controller::AddLogEntries()
        {
            const AZStd::vector<AZStd::string>* logs = nullptr;
            LogBus::BroadcastResult(logs, &LogTraits::GetEntries);
            if (!logs || logs->empty())
            {
                return;
            }

            const QTextCursor oldCursor = m_view->textEdit->textCursor();
            QScrollBar* scrollBar = m_view->textEdit->verticalScrollBar();

            m_view->textEdit->moveCursor(QTextCursor::End);
            QTextCursor textCursor = m_view->textEdit->textCursor();

            for (auto& entry : *logs)
            {
                auto line = "\n" + entry;
                textCursor.insertText(line.c_str());
            }

            scrollBar->setValue(scrollBar->maximum());
            m_view->textEdit->moveCursor(QTextCursor::StartOfLine);
            LogBus::Broadcast(&LogTraits::Clear);
        }

        void Controller::OnCloseButtonPress()
        {
            reject();
        }

        void Controller::OnScanButtonPress()
        {
            // \todo move to another file
            auto isUpToDate = [this](AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                AZ::Entity* scriptCanvasEntity = nullptr;

                if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
                {
                    ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
                    if (!scriptCanvasAsset)
                    {
                        AZ_Warning
                            ( ScriptCanvas::k_VersionExplorerWindow.data()
                            , false
                            , "InspectAsset: %s, AsestData failed to return ScriptCanvasAsset"
                            , asset.GetHint().c_str());
                        return true;
                    }

                    scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
                    AZ_Assert(scriptCanvasEntity, "The Script Canvas asset must have a valid entity");
                }

                auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
                AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");
                return !m_view->forceUpgrade->isChecked() && graphComponent->GetVersion().IsLatest();
            };

            ScanConfiguration config;
            config.reportFilteredGraphs = !m_view->onlyShowOutdated->isChecked();
            config.filter = isUpToDate;

            ModelRequestsBus::Broadcast(&ModelRequestsTraits::Scan, config);
        }

        void Controller::OnScanBegin(size_t assetCount)
        {
            m_handledAssetCount = 0;
            m_view->tableWidget->setRowCount(0);
            m_view->progressBar->setVisible(true);
            m_view->progressBar->setRange(0, aznumeric_cast<int>(assetCount));
            m_view->progressBar->setValue(0);
            m_view->scanButton->setEnabled(false);
            m_view->upgradeAllButton->setEnabled(false);
            m_view->onlyShowOutdated->setEnabled(false);

            QString spinnerText = QStringLiteral("Scan in progress - gathering graphs that can be updated");
            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(true);
        }

        void Controller::OnScanComplete(const ScanResult& result)
        {
            m_view->onlyShowOutdated->setEnabled(true);

            // Enable all the Upgrade buttons
            for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(true);
                }
            }

            QString spinnerText = QStringLiteral("Scan Complete");
            spinnerText.append(QString::asprintf(" - Discovered: %zu, Failed: %zu, Upgradeable: %zu, Up-to-date: %zu"
                , result.m_catalogAssets.size()
                , result.m_loadErrors.size()
                , result.m_unfiltered.size()
                , result.m_filteredAssets.size()));

            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(false);
            m_view->progressBar->setVisible(false);

            if (!result.m_unfiltered.empty())
            {
                m_view->upgradeAllButton->setEnabled(true);
            }
        }

        void Controller::OnScanFilteredGraph(const AZ::Data::AssetInfo& info)
        {
            OnScannedGraph(info, Filtered::Yes);
        }

        void Controller::OnScannedGraph(const AZ::Data::AssetInfo& assetInfo, Filtered filtered)
        {
            m_view->tableWidget->insertRow(m_handledAssetCount);
            QTableWidgetItem* rowName = new QTableWidgetItem(tr(assetInfo.m_relativePath.c_str()));
            m_view->tableWidget->setItem(m_handledAssetCount, static_cast<int>(ColumnAsset), rowName);
            SetRowSucceeded(m_handledAssetCount);

            if (filtered == Filtered::No)
            {
                QPushButton* rowGoToButton = new QPushButton(this);
                rowGoToButton->setText("Upgrade");
                rowGoToButton->setEnabled(false);
                SetRowBusy(m_handledAssetCount);
// \\ todo restore this
//                 connect(rowGoToButton, &QPushButton::clicked, [this, rowGoToButton, assetInfo] {
// 
// //                     request upgrade of a single graph
// //                     AZ::SystemTickBus::QueueFunction([this, rowGoToButton, assetInfo]() {
// //                         // Queue the process state change because we can't connect to the SystemTick bus in a Qt lambda
// //                         UpgradeSingle(rowGoToButton, spinner, assetInfo);
// //                     });
// // 
// //                     AZ::SystemTickBus::ExecuteQueuedEvents();
// 
//                 });

                m_view->tableWidget->setCellWidget(m_handledAssetCount, static_cast<int>(ColumnAction), rowGoToButton);
            }

            char resolvedBuffer[AZ_MAX_PATH_LEN] = { 0 };
            AZStd::string path = AZStd::string::format("@devroot@/%s", assetInfo.m_relativePath.c_str());
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(path.c_str(), resolvedBuffer, AZ_MAX_PATH_LEN);
            AZ::StringFunc::Path::GetFullPath(resolvedBuffer, path);
            AZ::StringFunc::Path::Normalize(path);

            bool result = false;
            AZ::Data::AssetInfo info;
            AZStd::string watchFolder;
            QByteArray assetNameUtf8 = assetInfo.m_relativePath.c_str();
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult
                ( result
                , &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath
                , assetNameUtf8
                , info
                , watchFolder);
            AZ_Error
                ( ScriptCanvas::k_VersionExplorerWindow.data()
                , result
                , "Failed to locate asset info for '%s'.", assetNameUtf8.constData());

            QToolButton* browseButton = new QToolButton(this);
            browseButton->setToolTip(AzQtComponents::fileBrowserActionName());
            browseButton->setIcon(QIcon(":/stylesheet/img/UI20/browse-edit.svg"));

            QString absolutePath = QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str());
            connect(browseButton, &QPushButton::clicked, [absolutePath] {
                AzQtComponents::ShowFileOnDesktop(absolutePath);
            });

            m_view->tableWidget->setCellWidget(m_handledAssetCount, static_cast<int>(ColumnBrowse), browseButton);
            OnScannedGraphResult(assetInfo);
        }

        void Controller::OnScannedGraphResult([[maybe_unused]] const AZ::Data::AssetInfo& info)
        {
            m_view->progressBar->setValue(aznumeric_cast<int>(m_handledAssetCount));
            ++m_handledAssetCount;
            AddLogEntries();
        }

        void Controller::OnScanLoadFailure(const AZ::Data::AssetInfo& info)
        {
            m_view->tableWidget->insertRow(m_handledAssetCount);
            QTableWidgetItem* rowName = new QTableWidgetItem
                ( tr(AZStd::string::format("Load Error: %s", info.m_relativePath.c_str()).c_str()));
            m_view->tableWidget->setItem(m_handledAssetCount, static_cast<int>(ColumnAsset), rowName);
            SetRowFailed(m_handledAssetCount, "Load failed");
            OnScannedGraphResult(info);
        }

        void Controller::OnScanUnFilteredGraph(const AZ::Data::AssetInfo& info)
        {
            OnScannedGraph(info, Filtered::No);
        }

        void Controller::OnUpgradeButtonPress()
        {
            auto simpleUpdate = [this](AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                if (asset.GetType() == azrtti_typeid<ScriptCanvasAsset>())
                {
                    ScriptCanvasAsset* scriptCanvasAsset = asset.GetAs<ScriptCanvasAsset>();
                    AZ_Assert(scriptCanvasAsset, "Unable to get the asset of ScriptCanvasAsset, but received type: %s"
                        , azrtti_typeid<ScriptCanvasAsset>().template ToString<AZStd::string>().c_str());
                    if (!scriptCanvasAsset)
                    {
                        return;
                    }

                    AZ::Entity* scriptCanvasEntity = scriptCanvasAsset->GetScriptCanvasEntity();
                    AZ_Assert(scriptCanvasEntity, "View::UpgradeGraph The Script Canvas asset must have a valid entity");
                    if (!scriptCanvasEntity)
                    {
                        return;
                    }

                    AZ::Entity* queryEntity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(queryEntity, &AZ::ComponentApplicationRequests::FindEntity, scriptCanvasEntity->GetId());
                    if (queryEntity)
                    {
                        if (queryEntity->GetState() == AZ::Entity::State::Active)
                        {
                            queryEntity->Deactivate();
                        }

                        scriptCanvasEntity = queryEntity;
                    }

                    if (scriptCanvasEntity->GetState() == AZ::Entity::State::Constructed)
                    {
                        scriptCanvasEntity->Init();
                    }

                    if (scriptCanvasEntity->GetState() == AZ::Entity::State::Init)
                    {
                        scriptCanvasEntity->Activate();
                    }

                    AZ_Assert(scriptCanvasEntity->GetState() == AZ::Entity::State::Active, "Graph entity is not active");
                    auto graphComponent = scriptCanvasEntity->FindComponent<ScriptCanvasEditor::Graph>();
                    AZ_Assert(graphComponent, "The Script Canvas entity must have a Graph component");
                    if (graphComponent)
                    {
                        graphComponent->UpgradeGraph
                            ( asset
                            , m_view->forceUpgrade->isChecked() ? Graph::UpgradeRequest::Forced : Graph::UpgradeRequest::IfOutOfDate
                            , m_view->verbose->isChecked());
                    }
                }
            };

            ModifyConfiguration config;
            config.modification = simpleUpdate;
            config.backupGraphBeforeModification = m_view->makeBackupCheckbox->isChecked();
            ModelRequestsBus::Broadcast(&ModelRequestsTraits::Modify, config);
        }

        void Controller::OnUpgradeBegin
            ( const ModifyConfiguration& config
            , [[maybe_unused]] const AZStd::vector<AZ::Data::AssetInfo>& assets)
        {
            for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(false);
                    SetRowBusy(row);
                }
            }

            QString spinnerText = QStringLiteral("Upgrade in progress - ");
            if (config.modifySingleAsset)
            {
                spinnerText.append(" single graph");
            }
            else
            {
                spinnerText.append(" all scanned graphs");
            }

            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(true);
        }

        void Controller::SetSpinnerIsBusy(bool isBusy)
        {
            m_view->spinner->SetIsBusy(isBusy);
            m_view->spinner->SetBusyIconSize(16);
        }

        void Controller::OnUpgradeComplete()
        {
            SetSpinnerIsBusy(false);
        }

        void Controller::OnUpgradeDependenciesGathered(const AZ::Data::AssetInfo& info, Result result)
        {
            QList<QTableWidgetItem*> items = m_view->tableWidget->findItems(info.m_relativePath.c_str(), Qt::MatchFlag::MatchExactly);
            if (!items.isEmpty())
            {
                for (auto* item : items)
                {
                    int row = item->row();

                    if (result == Result::Success)
                    {
                        SetRowSucceeded(row);
                    }
                    else
                    {
                        SetRowFailed(row, "");
                    }
                }
            }

            m_view->progressBar->setVisible(true);
            ++m_handledAssetCount;
            m_view->progressBar->setValue(m_handledAssetCount);
        }

        void Controller::OnUpgradeDependencySortBegin
            ( [[maybe_unused]] const ModifyConfiguration& config
            , const AZStd::vector<AZ::Data::AssetInfo>& assets)
        {
            m_handledAssetCount = 0;
            m_view->progressBar->setVisible(true);
            m_view->progressBar->setRange(0, aznumeric_caster(assets.size()));
            m_view->progressBar->setValue(0);
            m_view->scanButton->setEnabled(false);
            m_view->upgradeAllButton->setEnabled(false);
            m_view->onlyShowOutdated->setEnabled(false);

            for (int row = 0; row != m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(false);
                    SetRowBusy(row);
                }
            }

            QString spinnerText = QStringLiteral("Upgrade in progress - gathering dependencies for the scanned graphs");
            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(true);
        }

        void Controller::OnUpgradeDependencySortEnd
            ( [[maybe_unused]] const ModifyConfiguration& config
            , const AZStd::vector<AZ::Data::AssetInfo>& assets
            , [[maybe_unused]] const AZStd::vector<size_t>& sortedOrder)
        {
            m_handledAssetCount = 0;
            m_view->progressBar->setRange(0, aznumeric_caster(assets.size()));
            m_view->progressBar->setValue(0);
            m_view->progressBar->setVisible(true);

            for (int row = 0; row != m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(false);
                    SetRowPending(row);
                }
            }

            QString spinnerText = QStringLiteral("Upgrade in progress - gathering dependencies is complete");
            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(false);
        }

        void Controller::SetRowBusy(int index)
        {
            if (index >= m_view->tableWidget->rowCount())
            {
                return;
            }

            AzQtComponents::StyledBusyLabel* busy = new AzQtComponents::StyledBusyLabel(this);
            busy->SetBusyIconSize(16);
            m_view->tableWidget->setCellWidget(index, ColumnStatus, busy);         
        }

        void Controller::SetRowFailed(int index, AZStd::string_view message)
        {
            if (index >= m_view->tableWidget->rowCount())
            {
                return;
            }

            QToolButton* doneButton = new QToolButton(this);
            doneButton->setIcon(QIcon(":/stylesheet/img/UI20/titlebar-close.svg"));
            doneButton->setToolTip(message.data());
            m_view->tableWidget->setCellWidget(index, ColumnStatus, doneButton);
        }

        void Controller::SetRowPending(int index)
        {
            m_view->tableWidget->removeCellWidget(index, ColumnStatus);
        }

        void Controller::SetRowsBusy()
        {
            for (int i = 0; i != m_view->tableWidget->rowCount(); ++i)
            {
                SetRowBusy(i);
            }
        }

        void Controller::SetRowSucceeded(int index)
        {
            if (index >= m_view->tableWidget->rowCount())
            {
                return;
            }

            QToolButton* doneButton = new QToolButton(this);
            doneButton->setIcon(QIcon(":/stylesheet/img/UI20/checkmark-menu.svg"));
            m_view->tableWidget->setCellWidget(index, ColumnStatus, doneButton);
        }
    }
}
