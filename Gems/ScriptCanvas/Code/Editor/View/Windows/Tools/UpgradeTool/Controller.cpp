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

#include <Editor/View/Windows/Tools/UpgradeTool/ui_View.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Controller::Controller(QWidget* parent)
            : AzQtComponents::StyledDialog(parent)
            , m_view(new Ui::View())
        {
            m_view->setupUi(this);
            m_view->tableWidget->horizontalHeader()->setVisible(false);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            m_view->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
            m_view->tableWidget->setColumnWidth(3, 22);
            m_view->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
            m_view->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
            connect(m_view->scanButton, &QPushButton::pressed, this, &Controller::OnButtonPressScan);
            connect(m_view->closeButton, &QPushButton::pressed, this, &Controller::OnButtonPressClose);
            connect(m_view->upgradeAllButton, &QPushButton::pressed, this, &Controller::OnButtonPressUpgrade);
            m_view->progressBar->setValue(0);
            m_view->progressBar->setVisible(false);

            UpgradeNotificationsBus::Handler::BusConnect();
            ModelNotificationsBus::Handler::BusConnect();
        }

        Controller::~Controller()
        {
            delete m_view;
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
                textCursor.insertText("\n");
                textCursor.insertText(entry.c_str());
            }

            scrollBar->setValue(scrollBar->maximum());
            m_view->textEdit->moveCursor(QTextCursor::StartOfLine);
            LogBus::Broadcast(&LogTraits::Clear);
        }

        void Controller::EnableAllUpgradeButtons()
        {
            for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
            {
                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(true);
                }
            }
        }

        QList<QTableWidgetItem*> Controller::FindTableItems(const SourceHandle& info)
        {
            return m_view->tableWidget->findItems(info.Path().c_str(), Qt::MatchFlag::MatchExactly);
        }

        void Controller::OnButtonPressClose()
        {
            reject();
        }

        void Controller::OnButtonPressScan()
        {
            // \todo move to another file
            auto isUpToDate = [this](const SourceHandle& asset)
            {
                auto graphComponent = asset.Get();

                AZ_Warning
                    ( ScriptCanvas::k_VersionExplorerWindow.data()
                    , asset.Get() != nullptr
                    , "InspectAsset: %s, failed to load valid graph"
                    , asset.Path().c_str());

                return graphComponent
                    && (!graphComponent->GetVersion().IsLatest() || m_view->forceUpgrade->isChecked())
                        ? ScanConfiguration::Filter::Include
                        : ScanConfiguration::Filter::Exclude;
            };

            ScanConfiguration config;
            config.reportFilteredGraphs = !m_view->onlyShowOutdated->isChecked();
            config.filter = isUpToDate;

            SetLoggingPreferences();
            ModelRequestsBus::Broadcast(&ModelRequestsTraits::Scan, config);
        }

        void Controller::OnButtonPressUpgrade()
        {
            OnButtonPressUpgradeImplementation({});
        }

        void Controller::OnButtonPressUpgradeImplementation(const SourceHandle& assetInfo)
        {
            auto simpleUpdate = [this](SourceHandle& asset)
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), asset.Get() != nullptr
                    , "The Script Canvas asset must have a Graph component");

                if (asset.Get())
                {
                    asset.Mod()->UpgradeGraph
                        ( asset
                        , m_view->forceUpgrade->isChecked() ? Graph::UpgradeRequest::Forced : Graph::UpgradeRequest::IfOutOfDate
                        , m_view->verbose->isChecked());
                }
            };

            auto onReadyOnlyFile = [this]()->bool
            {
                int result = QMessageBox::No;
                QMessageBox mb
                    ( QMessageBox::Warning
                    , QObject::tr("Failed to Save Upgraded File")
                    , QObject::tr("The upgraded file could not be saved because the file is read only.\n"
                        "Do you want to make it writeable and overwrite it?")
                    , QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No
                    , this);
                result = mb.exec();
                return result == QMessageBox::YesToAll;
            };

            SetLoggingPreferences();
            ModifyConfiguration config;
            config.modifySingleAsset = assetInfo;
            config.modification = simpleUpdate;
            config.onReadOnlyFile = onReadyOnlyFile;
            config.backupGraphBeforeModification = m_view->makeBackupCheckbox->isChecked();
            ModelRequestsBus::Broadcast(&ModelRequestsTraits::Modify, config);
        }

        void Controller::OnButtonPressUpgradeSingle(const SourceHandle& info)
        {
            OnButtonPressUpgradeImplementation(info);
        }

        void Controller::OnUpgradeModificationBegin([[maybe_unused]] const ModifyConfiguration& config, const SourceHandle& info)
        {
            for (auto* item : FindTableItems(info))
            {
                int row = item->row();
                SetRowBusy(row);
                m_view->tableWidget->setCellWidget(row, ColumnAction, nullptr);
            }
        }

        void Controller::OnUpgradeModificationEnd
            ( [[maybe_unused]] const ModifyConfiguration& config
            , const SourceHandle& info
            , ModificationResult result)
        {
            if (result.errorMessage.empty())
            {
                VE_LOG("Successfully modified %s", result.asset.Path().c_str());
            }
            else
            {
                VE_LOG("Failed to modify %s: %s", result.asset.Path().c_str(), result.errorMessage.data());
            }

            for (auto* item : FindTableItems(info))
            {
                int row = item->row();

                if (result.errorMessage.empty())
                {
                    SetRowSucceeded(row);
                }
                else
                {
                    SetRowFailed(row, "");

                    if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                    {
                        button->setEnabled(false);
                    }
                }
            }

            m_view->progressBar->setVisible(true);
            ++m_handledAssetCount;
            m_view->progressBar->setValue(m_handledAssetCount);
            AddLogEntries();
        }

        void Controller::OnGraphUpgradeComplete(ScriptCanvasEditor::SourceHandle& asset, bool skipped)
        {
            ModificationResult result;
            result.asset = asset;

            if (skipped)
            {
                result.errorMessage = "Failed in editor upgrade state machine - check logs";
            }

            ModificationNotificationsBus::Broadcast(&ModificationNotificationsTraits::ModificationComplete, result);
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

            QString spinnerText = QStringLiteral("Scan Complete");
            spinnerText.append(QString::asprintf(" - Discovered: %zu, Failed: %zu, Upgradeable: %zu, Up-to-date: %zu"
                , result.m_catalogAssets.size()
                , result.m_loadErrors.size()
                , result.m_unfiltered.size()
                , result.m_filteredAssets.size()));

            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(false);
            m_view->progressBar->setVisible(false);
            EnableAllUpgradeButtons();

            if (!result.m_unfiltered.empty())
            {
                m_view->upgradeAllButton->setEnabled(true);
            }
        }

        void Controller::OnScanFilteredGraph(const SourceHandle& info)
        {
            OnScannedGraph(info, Filtered::Yes);
        }

        void Controller::OnScannedGraph(const SourceHandle& assetInfo, [[maybe_unused]] Filtered filtered)
        {
            const int rowIndex = m_view->tableWidget->rowCount();

            if (filtered == Filtered::No || !m_view->onlyShowOutdated->isChecked())
            {
                m_view->tableWidget->insertRow(rowIndex);
                QTableWidgetItem* rowName = new QTableWidgetItem(tr(assetInfo.Path().c_str()));
                m_view->tableWidget->setItem(rowIndex, static_cast<int>(ColumnAsset), rowName);
                SetRowSucceeded(rowIndex);

                if (filtered == Filtered::No)
                {
                    QPushButton* upgradeButton = new QPushButton(this);
                    upgradeButton->setText("Upgrade");
                    upgradeButton->setEnabled(false);
                    SetRowBusy(rowIndex);

                    connect
                        ( upgradeButton
                        , &QPushButton::pressed
                        , this
                        , [this, assetInfo]()
                        {
                            this->OnButtonPressUpgradeSingle(assetInfo);
                        });

                    m_view->tableWidget->setCellWidget(rowIndex, static_cast<int>(ColumnAction), upgradeButton);
                }
                                
                bool result = false;
                AZ::Data::AssetInfo info;
                AZStd::string watchFolder;
                QByteArray assetNameUtf8 = assetInfo.Path().c_str();
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

                m_view->tableWidget->setCellWidget(rowIndex, static_cast<int>(ColumnBrowse), browseButton);
            }

            OnScannedGraphResult(assetInfo);
        }

        void Controller::OnScannedGraphResult([[maybe_unused]] const SourceHandle& info)
        {
            m_view->progressBar->setValue(aznumeric_cast<int>(m_handledAssetCount));
            ++m_handledAssetCount;
            AddLogEntries();
        }

        void Controller::OnScanLoadFailure(const SourceHandle& info)
        {
            const int rowIndex = m_view->tableWidget->rowCount();
            m_view->tableWidget->insertRow(rowIndex);
            QTableWidgetItem* rowName = new QTableWidgetItem
                ( tr(AZStd::string::format("Load Error: %s", info.Path().c_str()).c_str()));
            m_view->tableWidget->setItem(rowIndex, static_cast<int>(ColumnAsset), rowName);
            SetRowFailed(rowIndex, "Load failed");
            OnScannedGraphResult(info);
        }

        void Controller::OnScanUnFilteredGraph(const SourceHandle& info)
        {
            OnScannedGraph(info, Filtered::No);
        }

        void Controller::OnUpgradeBegin
            ( const ModifyConfiguration& config
            , [[maybe_unused]] const AZStd::vector<SourceHandle>& assets)
        {
            QString spinnerText = QStringLiteral("Upgrade in progress - ");
            if (!config.modifySingleAsset.Path().empty())
            {
                spinnerText.append(" single graph");

                if (assets.size() == 1)
                {
                    for (auto* item : FindTableItems(assets.front()))
                    {
                        int row = item->row();
                        SetRowBusy(row);
                    }
                }
            }
            else
            {
                for (int row = 0; row < m_view->tableWidget->rowCount(); ++row)
                {
                    if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                    {
                        button->setEnabled(false);
                    }

                    SetRowBusy(row);
                }

                spinnerText.append(" all scanned graphs");
            }

            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(true);
        }

        void Controller::SetLoggingPreferences()
        {
            LogBus::Broadcast(&LogTraits::SetVerbose, m_view->verbose->isChecked());
            LogBus::Broadcast(&LogTraits::SetVersionExporerExclusivity, m_view->updateReportingOnly->isChecked());
        }

        void Controller::SetSpinnerIsBusy(bool isBusy)
        {
            m_view->spinner->SetIsBusy(isBusy);
            m_view->spinner->SetBusyIconSize(16);
        }

        void Controller::OnUpgradeComplete(const ModificationResults& result)
        {
            QString spinnerText = QStringLiteral("Upgrade Complete - ");
            spinnerText.append(QString::asprintf(" - Upgraded: %zu, Failed: %zu"
                , result.m_successes.size()
                , result.m_failures.size()));
            m_view->spinner->SetText(spinnerText);
            SetSpinnerIsBusy(false);
            AddLogEntries();
            EnableAllUpgradeButtons();
            m_view->scanButton->setEnabled(true);
        }

        void Controller::OnUpgradeDependenciesGathered(const SourceHandle& info, Result result)
        {
            for (auto* item : FindTableItems(info))
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

                if (QPushButton* button = qobject_cast<QPushButton*>(m_view->tableWidget->cellWidget(row, ColumnAction)))
                {
                    button->setEnabled(true);
                }
            }

            m_view->progressBar->setVisible(true);
            ++m_handledAssetCount;
            m_view->progressBar->setValue(m_handledAssetCount);
            AddLogEntries();
        }

        void Controller::OnUpgradeDependencySortBegin
            ( [[maybe_unused]] const ModifyConfiguration& config
            , const AZStd::vector<SourceHandle>& assets)
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
            , const AZStd::vector<SourceHandle>& assets
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
            AddLogEntries();
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
