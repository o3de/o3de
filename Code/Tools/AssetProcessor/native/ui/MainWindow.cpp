/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "MainWindow.h"

#include "AssetTreeFilterModel.h"
#include "AssetTreeItem.h"
#include "ConnectionEditDialog.h"
#include "ProductAssetTreeItemData.h"
#include "ProductAssetTreeModel.h"
#include "ProductDependencyTreeItemData.h"
#include "SourceAssetTreeItemData.h"
#include "SourceAssetTreeModel.h"
#include <native/ui/BuilderData.h>
#include <native/ui/BuilderDataItem.h>
#include <native/ui/BuilderInfoPatternsModel.h>
#include <native/ui/BuilderInfoMetricsModel.h>
#include <native/ui/EnabledRelocationTypesModel.h>
#include <native/ui/SourceAssetTreeFilterModel.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/pointer.h>

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>

#include <native/resourcecompiler/JobsModel.h>
#include "native/ui/ui_MainWindow.h"
#include "native/ui/JobTreeViewItemDelegate.h"
#include <native/utilities/AssetServerHandler.h>
#include <native/utilities/assetUtils.h>

#include "../utilities/GUIApplicationManager.h"
#include "../utilities/ApplicationServer.h"
#include "../connection/connectionManager.h"
#include "../connection/connection.h"
#include "../resourcecompiler/rccontroller.h"
#include "../resourcecompiler/RCJobSortFilterProxyModel.h"


#include <QClipboard>
#include <QDesktopServices>
#include <QListWidget>
#include <QMessageBox>
#include <QUrl>
#include <QWidgetAction>
#include <QKeyEvent>
#include <QFileDialog>

static const QString g_jobFilteredSearchWidgetState = QStringLiteral("jobFilteredSearchWidget");
static const qint64 AssetTabFilterUpdateIntervalMs = 5000;

static const int MaxVisiblePopoutMenuRows = 20;
static const QString productMenuTitle(QObject::tr("View product asset..."));
static const QString intermediateMenuTitle(QObject::tr("View intermediate asset..."));

struct AssetRightClickMenuResult
{
    QListWidget* m_listWidget = nullptr;
    QMenu* m_assetMenu = nullptr;
};

AssetRightClickMenuResult SetupAssetRightClickMenu(QMenu* parentMenu, QString title, QString tooltip)
{
    AssetRightClickMenuResult result;
    if (!parentMenu)
    {
        return result;
    }

    result.m_assetMenu = parentMenu->addMenu(title);
    QWidgetAction* productMenuListAction = new QWidgetAction(result.m_assetMenu);
    productMenuListAction->setToolTip(tooltip);
    result.m_listWidget = new QListWidget(result.m_assetMenu);
    result.m_listWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    result.m_listWidget->setTextElideMode(Qt::ElideLeft);
    result.m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    result.m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    productMenuListAction->setDefaultWidget(result.m_listWidget);
    result.m_assetMenu->addAction(productMenuListAction);
    return result;
}

AssetRightClickMenuResult SetupProductAssetRightClickMenu(QMenu* parentMenu)
{
    return SetupAssetRightClickMenu(parentMenu, productMenuTitle, QObject::tr("Shows this product asset in the Product Assets tab."));
}

AssetRightClickMenuResult SetupIntermediateAssetRightClickMenu(QMenu* parentMenu)
{
    return SetupAssetRightClickMenu(parentMenu, intermediateMenuTitle, QObject::tr("Shows this intermediate asset in the Intermediate Assets tab."));
}

void CreateDisabledAssetRightClickMenu(QMenu* parentMenu, QMenu* existingMenu, QString title, QString tooltip)
{
    if (!parentMenu || !existingMenu)
    {
        return;
    }
    // If there were no products, then show a disabled action with a tooltip.
    // Disabled menus don't support tooltips, so remove the menu first.
    parentMenu->removeAction(existingMenu->menuAction());
    existingMenu->deleteLater();

    QAction* disabledProductTableAction = parentMenu->addAction(title);
    disabledProductTableAction->setToolTip(tooltip);
    disabledProductTableAction->setDisabled(true);
}

void ResizeAssetRightClickMenuList(QListWidget* assetList, int assetCount)
{
    // Clamp the max assets displayed at once. This is a list view, so it will show a scroll bar for anything over this.
    assetCount = AZStd::min(MaxVisiblePopoutMenuRows, assetCount);
    // Using fixed width and height because the size hints aren't working well within a qmenu popout menu.
    assetList->setFixedHeight(assetCount * assetList->sizeHintForRow(0));
    assetList->setFixedWidth(assetList->sizeHintForColumn(0));
}

MainWindow::Config MainWindow::loadConfig(QSettings& settings)
{
    using namespace AzQtComponents;

    Config config = defaultConfig();

    // Asset Status
    {
        ConfigHelpers::GroupGuard assetStatus(&settings, QStringLiteral("AssetStatus"));
        ConfigHelpers::read<int>(settings, QStringLiteral("JobStatusColumnWidth"), config.jobStatusColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobSourceColumnWidth"), config.jobSourceColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobPlatformColumnWidth"), config.jobPlatformColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobKeyColumnWidth"), config.jobKeyColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobCompletedColumnWidth"), config.jobCompletedColumnWidth);
    }

    // Event Log Details
    {
        ConfigHelpers::GroupGuard eventLogDetails(&settings, QStringLiteral("EventLogDetails"));
        ConfigHelpers::read<int>(settings, QStringLiteral("LogTypeColumnWidth"), config.logTypeColumnWidth);
    }

    // Event Log Line Details
    {
        ConfigHelpers::GroupGuard eventLogDetails(&settings, QStringLiteral("EventLogLineDetails"));
        ConfigHelpers::read<int>(settings, QStringLiteral("contextDetailsTableMaximumRows"), config.contextDetailsTableMaximumRows);
    }

    return config;
}

MainWindow::Config MainWindow::defaultConfig()
{
    // These are used if the values can't be read from AssetProcessorConfig.ini.
    Config config;

    config.jobStatusColumnWidth = 100;
    config.jobSourceColumnWidth = 160;
    config.jobPlatformColumnWidth = 100;
    config.jobKeyColumnWidth = 120;
    config.jobCompletedColumnWidth = 160;

    config.logTypeColumnWidth = 150;

    config.contextDetailsTableMaximumRows = 10;

    return config;
}

MainWindow::MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent)
    : QMainWindow(parent)
    , m_guiApplicationManager(guiApplicationManager)
    , m_jobSortFilterProxy(new AssetProcessor::JobSortFilterProxyModel(this))
    , m_logSortFilterProxy(new LogSortFilterProxy(this))
    , m_jobsModel(new AssetProcessor::JobsModel(this))
    , m_logsModel(new AzToolsFramework::Logging::LogTableModel(this))
    , ui(new Ui::MainWindow)
    , m_loggingPanel(nullptr)
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
    , m_builderList(new BuilderListModel(this))
    , m_builderListSortFilterProxy(new BuilderListSortFilterProxy(this))
    , m_builderInfoPatterns(new AssetProcessor::BuilderInfoPatternsModel(this))
    , m_enabledRelocationTypesModel(new AssetProcessor::EnabledRelocationTypesModel(this))
{
    ui->setupUi(this);

    // Don't show the "Filter by:" text on this filter widget
    ui->jobFilteredSearchWidget->clearLabelText();
    ui->detailsFilterWidget->clearLabelText();
    ui->timerContainerWidget->setVisible(false);
}

bool MainWindow::eventFilter(QObject* /*obj*/, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key::Key_Space)
        {
            // Stop space key from opening filter list.
            return true;
        }
    }
    return false;
}

void MainWindow::Activate()
{
    using namespace AssetProcessor;

    m_sharedDbConnection = AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection>(aznew AzToolsFramework::AssetDatabase::AssetDatabaseConnection());
    m_sharedDbConnection->OpenDatabase();

    ui->projectLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Project"))
        .arg(QDir{m_guiApplicationManager->GetProjectPath()}.absolutePath()));

    ui->rootLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Root"))
        .arg(m_guiApplicationManager->GetSystemRoot().absolutePath()));

    ui->portLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Processor port"))
        .arg(m_guiApplicationManager->GetApplicationServer()->GetServerListeningPort()));

    connect(ui->supportButton, &QPushButton::clicked, this, &MainWindow::OnSupportClicked);

    ui->buttonList->addTab(QStringLiteral("Welcome"));
    ui->buttonList->addTab(QStringLiteral("Jobs"));
    ui->buttonList->addTab(QStringLiteral("Assets"));
    ui->buttonList->addTab(QStringLiteral("Logs"));
    ui->buttonList->addTab(QStringLiteral("Connections"));
    ui->buttonList->addTab(QStringLiteral("Builders"));
    ui->buttonList->addTab(QStringLiteral("Settings"));
    ui->buttonList->addTab(QStringLiteral("Shared Cache"));
    ui->buttonList->addTab(QStringLiteral("Asset Relocation"));

    connect(ui->buttonList, &AzQtComponents::SegmentBar::currentChanged, ui->dialogStack, &QStackedWidget::setCurrentIndex);
    const int startIndex = static_cast<int>(DialogStackIndex::Welcome);
    ui->dialogStack->setCurrentIndex(startIndex);
    ui->buttonList->setCurrentIndex(startIndex);

    //Connection view
    ui->connectionTreeView->setModel(m_guiApplicationManager->GetConnectionManager());
    ui->connectionTreeView->setEditTriggers(QAbstractItemView::CurrentChanged);
    ui->connectionTreeView->header()->setSectionResizeMode(ConnectionManager::IdColumn, QHeaderView::Stretch);
    ui->connectionTreeView->header()->setSectionResizeMode(ConnectionManager::AutoConnectColumn, QHeaderView::Fixed);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::StatusColumn, 160);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::IpColumn, 150);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PortColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PlatformColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::AutoConnectColumn, 60);

    ui->connectionTreeView->header()->setStretchLastSection(false);
    connect(ui->connectionTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::OnConnectionSelectionChanged);


    ui->editConnectionButton->setEnabled(false);
    ui->removeConnectionButton->setEnabled(false);
    connect(ui->editConnectionButton, &QPushButton::clicked, this, &MainWindow::OnEditConnection);
    connect(ui->addConnectionButton, &QPushButton::clicked, this, &MainWindow::OnAddConnection);
    connect(ui->removeConnectionButton, &QPushButton::clicked, this, &MainWindow::OnRemoveConnection);
    connect(ui->connectionTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        EditConnection(index);
    });

    ui->connectionTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->connectionTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::OnConnectionContextMenu);

    //allowed list connections
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::FirstTimeAddedToRejctedList, this, &MainWindow::FirstTimeAddedToRejectedList);
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::SyncAllowedListAndRejectedList, this, &MainWindow::SyncAllowedListAndRejectedList);
    connect(ui->allowListAllowedListConnectionsListView, &QListView::clicked, this, &MainWindow::OnAllowedListConnectionsListViewClicked);
    ui->allowListAllowedListConnectionsListView->setModel(&m_allowedListAddresses);
    connect(ui->allowedListRejectedConnectionsListView, &QListView::clicked, this, &MainWindow::OnRejectedConnectionsListViewClicked);
    ui->allowedListRejectedConnectionsListView->setModel(&m_rejectedAddresses);

    connect(ui->allowedListEnableCheckBox, &QCheckBox::toggled, this, &MainWindow::OnAllowedListCheckBoxToggled);

    connect(ui->allowedListAddHostNameToolButton, &QToolButton::clicked, this, &MainWindow::OnAddHostNameAllowedListButtonClicked);
    connect(ui->allowedListAddIPToolButton, &QPushButton::clicked, this, &MainWindow::OnAddIPAllowedListButtonClicked);

    connect(ui->allowedListToAllowedListToolButton, &QPushButton::clicked, this, &MainWindow::OnToAllowedListButtonClicked);
    connect(ui->allowedListToRejectedListToolButton, &QToolButton::clicked, this, &MainWindow::OnToRejectedListButtonClicked);

    //set the input validator for ip addresses on the add address line edit
    QRegExp validHostName("^((?=.{1,255}$)[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?(?:\\.[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?)*\\.?)$");
    QRegExp validIP("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(\\/([0-9]|[1-2][0-9]|3[0-2]))?$|^((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:)))(%.+)?s*(\\/([0-9]|[1-9][0-9]|1[0-1][0-9]|12[0-8]))?$");

    QRegExpValidator* hostNameValidator = new QRegExpValidator(validHostName, this);
    ui->allowedListAddHostNameLineEdit->setValidator(hostNameValidator);

    QRegExpValidator* ipValidator = new QRegExpValidator(validIP, this);
    ui->allowedListAddIPLineEdit->setValidator(ipValidator);

    //Job view
    m_jobSortFilterProxy->setSourceModel(m_jobsModel);
    m_jobSortFilterProxy->setDynamicSortFilter(true);
    m_jobSortFilterProxy->setFilterKeyColumn(JobsModel::ColumnSource);

    ui->jobTreeView->setModel(m_jobSortFilterProxy);
    ui->jobTreeView->setSortingEnabled(true);
    ui->jobTreeView->header()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    ui->jobTreeView->setItemDelegate(new AssetProcessor::JobTreeViewItemDelegate(ui->jobTreeView));

    ui->jobTreeView->setToolTip(tr("Click to view Job Log"));

    ui->detailsFilterWidget->SetTypeFilterVisible(true);
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, m_logSortFilterProxy,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&LogSortFilterProxy::setFilterRegExp));
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, m_logSortFilterProxy, &LogSortFilterProxy::onTypeFilterChanged);

    // add filters for each logging type
    ui->detailsFilterWidget->AddTypeFilter("Status", "Debug", AzToolsFramework::Logging::LogLine::TYPE_DEBUG);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Message", AzToolsFramework::Logging::LogLine::TYPE_MESSAGE);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Warning", AzToolsFramework::Logging::LogLine::TYPE_WARNING);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Error", AzToolsFramework::Logging::LogLine::TYPE_ERROR);

    m_logSortFilterProxy->setDynamicSortFilter(true);
    m_logSortFilterProxy->setSourceModel(m_logsModel);
    m_logSortFilterProxy->setFilterKeyColumn(AzToolsFramework::Logging::LogTableModel::ColumnMessage);
    m_logSortFilterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->jobLogTableView->setModel(m_logSortFilterProxy);
    ui->jobLogTableView->setItemDelegate(new AzToolsFramework::Logging::LogTableItemDelegate(ui->jobLogTableView));
    ui->jobLogTableView->setExpandOnSelection();

    connect(ui->jobTreeView->header(), &QHeaderView::sortIndicatorChanged, m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::sort);
    connect(ui->jobTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobSelectionChanged);
    ui->jobFilteredSearchWidget->SetTypeFilterVisible(true);
    ui->jobFilteredSearchWidget->assetTypeSelectorButton()->installEventFilter(this);

    // listen for job status changes in order to update the log view with the latest log data
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, this, &MainWindow::JobStatusChanged);

    ui->jobContextLogTableView->setModel(new AzToolsFramework::Logging::ContextDetailsLogTableModel(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setItemDelegate(new AzQtComponents::TableViewItemDelegate(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setExpandOnSelection();

    ui->jobContextContainer->setVisible(false);

    connect(ui->jobLogTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobLogSelectionChanged);

    const auto statuses =
    {
        AzToolsFramework::AssetSystem::JobStatus::Failed,
        AzToolsFramework::AssetSystem::JobStatus::Completed,
        AzToolsFramework::AssetSystem::JobStatus::Queued,
        AzToolsFramework::AssetSystem::JobStatus::InProgress
    };

    const auto category = tr("Status");
    for (const auto& status : statuses)
    {
        ui->jobFilteredSearchWidget->AddTypeFilter(category, JobsModel::GetStatusInString(status, 0, 0),
            QVariant::fromValue(status));
    }

    AssetProcessor::CustomJobStatusFilter customFilter{ true };
    ui->jobFilteredSearchWidget->AddTypeFilter(category, "Completed w/ Warnings", QVariant::fromValue(customFilter));

    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::OnJobStatusFilterChanged);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_jobSortFilterProxy,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetProcessor::JobSortFilterProxyModel::setFilterRegExp));
    {
        QSettings settingsObj(this);
        ui->jobFilteredSearchWidget->readSettings(settingsObj, g_jobFilteredSearchWidgetState);
    }
    auto writeJobFilterSettings = [this]()
    {
        QSettings settingsObj(this);
        ui->jobFilteredSearchWidget->writeSettings(settingsObj, g_jobFilteredSearchWidgetState);
    };
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        this, writeJobFilterSettings);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        this, writeJobFilterSettings);

    // Asset view
    m_sourceAssetTreeFilterModel = new AssetProcessor::SourceAssetTreeFilterModel(this);
    m_sourceModel = new AssetProcessor::SourceAssetTreeModel(m_sharedDbConnection, this);
    m_sourceAssetTreeFilterModel->setSourceModel(m_sourceModel);
    ui->SourceAssetsTreeView->setModel(m_sourceAssetTreeFilterModel);
    ui->SourceAssetsTreeView->setColumnWidth(aznumeric_cast<int>(AssetTreeColumns::Extension), 80);
    ui->SourceAssetsTreeView->setColumnWidth(aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration), 170);
    connect(ui->assetDataFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_sourceAssetTreeFilterModel, static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetTreeFilterModel::FilterChanged));

    m_intermediateAssetTreeFilterModel = new AssetProcessor::AssetTreeFilterModel(this);
    m_intermediateModel = new AssetProcessor::SourceAssetTreeModel(m_sharedDbConnection, this);
    m_intermediateModel->SetOnlyShowIntermediateAssets();
    m_intermediateAssetTreeFilterModel->setSourceModel(m_intermediateModel);
    ui->IntermediateAssetsTreeView->setModel(m_intermediateAssetTreeFilterModel);
    connect(
        ui->assetDataFilteredSearchWidget,
        &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_intermediateAssetTreeFilterModel,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetTreeFilterModel::FilterChanged));


    m_productAssetTreeFilterModel = new AssetProcessor::AssetTreeFilterModel(this);
    m_productModel = new AssetProcessor::ProductAssetTreeModel(m_sharedDbConnection, this);
    m_productAssetTreeFilterModel->setSourceModel(m_productModel);
    ui->ProductAssetsTreeView->setModel(m_productAssetTreeFilterModel);
    ui->ProductAssetsTreeView->setColumnWidth(aznumeric_cast<int>(AssetTreeColumns::Extension), 80);
    connect(ui->assetDataFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_productAssetTreeFilterModel, static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetTreeFilterModel::FilterChanged));

    ui->intermediateAssetDetailsPanel->SetIsIntermediateAsset();

    AZStd::optional<AZ::s64> intermediateAssetFolderId(m_guiApplicationManager->GetAssetProcessorManager()->GetIntermediateAssetScanFolderId());
    ui->productAssetDetailsPanel->SetIntermediateAssetFolderId(intermediateAssetFolderId);
    ui->sourceAssetDetailsPanel->SetIntermediateAssetFolderId(intermediateAssetFolderId);
    ui->intermediateAssetDetailsPanel->SetIntermediateAssetFolderId(intermediateAssetFolderId);

    AzQtComponents::StyleManager::setStyleSheet(ui->sourceAssetDetailsPanel, QStringLiteral("style:AssetProcessor.qss"));
    AzQtComponents::StyleManager::setStyleSheet(ui->intermediateAssetDetailsPanel, QStringLiteral("style:AssetProcessor.qss"));
    AzQtComponents::StyleManager::setStyleSheet(ui->productAssetDetailsPanel, QStringLiteral("style:AssetProcessor.qss"));

    ui->sourceAssetDetailsPanel->RegisterAssociatedWidgets(
        ui->SourceAssetsTreeView,
        m_sourceModel,
        m_sourceAssetTreeFilterModel,
        ui->IntermediateAssetsTreeView,
        m_intermediateModel,
        m_intermediateAssetTreeFilterModel,
        ui->ProductAssetsTreeView,
        m_productModel,
        m_productAssetTreeFilterModel,
        ui->assetsTabWidget);
    ui->intermediateAssetDetailsPanel->RegisterAssociatedWidgets(
        ui->SourceAssetsTreeView,
        m_sourceModel,
        m_sourceAssetTreeFilterModel,
        ui->IntermediateAssetsTreeView,
        m_intermediateModel,
        m_intermediateAssetTreeFilterModel,
        ui->ProductAssetsTreeView,
        m_productModel,
        m_productAssetTreeFilterModel,
        ui->assetsTabWidget);
    ui->productAssetDetailsPanel->RegisterAssociatedWidgets(
        ui->SourceAssetsTreeView,
        m_sourceModel,
        m_sourceAssetTreeFilterModel,
        ui->IntermediateAssetsTreeView,
        m_intermediateModel,
        m_intermediateAssetTreeFilterModel,
        ui->ProductAssetsTreeView,
        m_productModel,
        m_productAssetTreeFilterModel,
        ui->assetsTabWidget);
    ui->productAssetDetailsPanel->SetScannerInformation(ui->missingDependencyScanResults, m_guiApplicationManager->GetAssetProcessorManager()->GetDatabaseConnection());
    ui->productAssetDetailsPanel->SetupDependencyGraph(
        ui->ProductAssetsTreeView, m_guiApplicationManager->GetAssetProcessorManager()->GetDatabaseConnection());
    ui->productAssetDetailsPanel->SetScanQueueEnabled(false);

    connect(ui->SourceAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, ui->sourceAssetDetailsPanel, &SourceAssetDetailsPanel::AssetDataSelectionChanged);
    connect(ui->IntermediateAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, ui->intermediateAssetDetailsPanel, &SourceAssetDetailsPanel::AssetDataSelectionChanged);
    connect(ui->ProductAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, ui->productAssetDetailsPanel, &ProductAssetDetailsPanel::AssetDataSelectionChanged);
    connect(ui->assetsTabWidget, &QTabWidget::currentChanged, this, &MainWindow::OnAssetTabChange);

    ui->ProductAssetsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ProductAssetsTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowProductAssetContextMenu);

    ui->SourceAssetsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SourceAssetsTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowSourceAssetContextMenu);

    ui->IntermediateAssetsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->IntermediateAssetsTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowIntermediateAssetContextMenu);

    ui->productAssetDetailsPanel->GetOutgoingProductDependenciesTreeView()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
        ui->productAssetDetailsPanel->GetOutgoingProductDependenciesTreeView(), &QWidget::customContextMenuRequested, this,
        &MainWindow::ShowOutgoingProductDependenciesContextMenu);
    ui->productAssetDetailsPanel->GetIncomingProductDependenciesTreeView()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
        ui->productAssetDetailsPanel->GetIncomingProductDependenciesTreeView(), &QWidget::customContextMenuRequested, this,
        &MainWindow::ShowIncomingProductDependenciesContextMenu);

    SetupAssetSelectionCaching();
    // the first time we open that panel we can refresh it.
    m_connectionForResettingAssetsView = connect(
        ui->dialogStack,
        &QStackedWidget::currentChanged,
        this,
        [&](int index)
        {
            if (index == static_cast<int>(DialogStackIndex::Assets))
            {
                // the first time we show the asset window, reset the model since its so expensive to do on every startup
                // and many times, the user does not even go to that panel.
                m_sourceModel->Reset();
                m_intermediateModel->Reset();
                m_productModel->Reset();
                QObject::disconnect(m_connectionForResettingAssetsView);
            }
        });

    //Log View
    m_loggingPanel = ui->LoggingPanel;
    m_loggingPanel->SetStorageID(AZ_CRC_CE("AssetProcessor::LogPanel"));

    connect(ui->logButton, &QPushButton::clicked, this, &MainWindow::DesktopOpenJobLogs);

    if (!m_loggingPanel->LoadState())
    {
        // if unable to load state then show the default tabs
        ResetLoggingPanel();
    }

    AzQtComponents::ConfigHelpers::loadConfig<Config, MainWindow>(m_fileSystemWatcher, &m_config, QStringLiteral("style:AssetProcessorConfig.ini"), this, std::bind(&MainWindow::ApplyConfig, this));
    ApplyConfig();

    connect(m_loggingPanel, &AzToolsFramework::LogPanel::StyledLogPanel::TabsReset, this, &MainWindow::ResetLoggingPanel);
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, m_jobsModel, &AssetProcessor::JobsModel::OnJobStatusChanged);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::JobRemoved, m_jobsModel, &AssetProcessor::JobsModel::OnJobRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnSourceRemoved);
    connect(
        m_guiApplicationManager->GetAssetProcessorManager(),
        &AssetProcessor::AssetProcessorManager::JobProcessDurationChanged,
        m_jobsModel,
        &AssetProcessor::JobsModel::OnJobProcessDurationChanged);
    connect(
        m_guiApplicationManager->GetAssetProcessorManager(),
        &AssetProcessor::AssetProcessorManager::CreateJobsDurationChanged,
        m_sourceModel,
        &AssetProcessor::SourceAssetTreeModel::OnCreateJobsDurationChanged);

    connect(ui->jobTreeView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobViewContextMenu);
    connect(ui->jobContextLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowLogLineContextMenu);
    connect(ui->jobLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobLogContextMenu);

    m_jobsModel->PopulateJobsFromDatabase();

    // Builders Tab:
    m_builderData = new BuilderData(m_sharedDbConnection, this);
    m_builderListSortFilterProxy->setDynamicSortFilter(true);
    m_builderListSortFilterProxy->setSourceModel(m_builderList);
    m_builderListSortFilterProxy->sort(0);
    ui->builderList->setModel(m_builderListSortFilterProxy);
    ui->builderInfoPatternsTableView->setModel(m_builderInfoPatterns);
    m_builderInfoMetrics = new BuilderInfoMetricsModel(m_builderData, this);
    m_builderInfoMetricsSort = new BuilderInfoMetricsSortModel(this);
    m_builderInfoMetricsSort->setSourceModel(m_builderInfoMetrics);
    m_builderInfoMetricsSort->setSortRole(aznumeric_cast<int>(BuilderInfoMetricsModel::Role::SortRole));
    ui->builderInfoMetricsTreeView->setModel(m_builderInfoMetricsSort);
    ui->builderInfoMetricsTreeView->setColumnWidth(0, 400);
    ui->builderInfoMetricsTreeView->setColumnWidth(1, 70);
    ui->builderInfoMetricsTreeView->setColumnWidth(2, 150);
    ui->builderInfoMetricsTreeView->setColumnWidth(3, 150);
    connect(ui->builderList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::BuilderTabSelectionChanged);
    connect(m_guiApplicationManager, &GUIApplicationManager::OnBuildersRegistered, [this]()
    {
        if(m_builderList)
        {
            m_builderList->Reset();

            if(m_builderListSortFilterProxy)
            {
                m_builderListSortFilterProxy->sort(0);
            }
        }

        if (m_builderInfoMetrics)
        {
            m_builderInfoMetrics->Reset();
        }
    });
    connect(
        m_guiApplicationManager->GetAssetProcessorManager(),
        &AssetProcessorManager::JobProcessDurationChanged,
        m_builderData,
        &BuilderData::OnProcessJobDurationChanged);
    connect(
        m_guiApplicationManager->GetAssetProcessorManager(),
        &AssetProcessorManager::CreateJobsDurationChanged,
        m_builderData,
        &BuilderData::OnCreateJobsDurationChanged);
    connect(m_builderData, &BuilderData::DurationChanged, m_builderInfoMetrics, &BuilderInfoMetricsModel::OnDurationChanged);

    // Settings tab:
    connect(ui->fullScanButton, &QPushButton::clicked, this, &MainWindow::OnRescanButtonClicked);

    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->modtimeSkippingCheckBox);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->disableStartupScanCheckBox);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->debugOutputCheckBox);

    const auto apm = m_guiApplicationManager->GetAssetProcessorManager();

    // Note: the settings can't be used in ::MainWindow(), because the application name
    // hasn't been set up and therefore the settings will load from somewhere different than later
    // on.
    // Read the current settings to give command line options a chance to override the default
    QSettings settings;
    settings.beginGroup("Options");
    bool zeroAnalysisModeFromSettings = settings.value("EnableZeroAnalysis", QVariant(true)).toBool() || apm->GetModtimeSkippingFeatureEnabled();
    bool enableBuilderDebugFlag = settings.value("EnableBuilderDebugFlag", QVariant(false)).toBool() || apm->GetBuilderDebugFlag();
    bool initialScanSkippingEnabled = settings.value("SkipInitialScan", QVariant(false)).toBool() || apm->GetInitialScanSkippingFeatureEnabled();
    settings.endGroup();

    // zero analysis flag
    apm->SetEnableModtimeSkippingFeature(zeroAnalysisModeFromSettings);
    ui->modtimeSkippingCheckBox->setCheckState(zeroAnalysisModeFromSettings ? Qt::Checked : Qt::Unchecked);

    // Connect after updating settings to avoid saving a command line override
    QObject::connect(ui->modtimeSkippingCheckBox, &QCheckBox::stateChanged, this,
        [this](int newCheckState)
    {
        bool newOption = newCheckState == Qt::Checked ? true : false;
        m_guiApplicationManager->GetAssetProcessorManager()->SetEnableModtimeSkippingFeature(newOption);
        QSettings settingsInCallback;
        settingsInCallback.beginGroup("Options");
        settingsInCallback.setValue("EnableZeroAnalysis", QVariant(newOption));
        settingsInCallback.endGroup();
    });

    // output debug flag
    apm->SetBuilderDebugFlag(enableBuilderDebugFlag);
    ui->debugOutputCheckBox->setCheckState(enableBuilderDebugFlag ? Qt::Checked : Qt::Unchecked);

    QObject::connect(ui->debugOutputCheckBox, &QCheckBox::stateChanged, this,
        [this](int newCheckState)
        {
            bool newOption = newCheckState == Qt::Checked ? true : false;
            m_guiApplicationManager->GetAssetProcessorManager()->SetBuilderDebugFlag(newOption);
            QSettings settingsInCallback;
            settingsInCallback.beginGroup("Options");
            settingsInCallback.setValue("EnableBuilderDebugFlag", QVariant(newOption));
            settingsInCallback.endGroup();
        });

    apm->SetInitialScanSkippingFeature(initialScanSkippingEnabled);
    ui->disableStartupScanCheckBox->setCheckState(initialScanSkippingEnabled ? Qt::Checked : Qt::Unchecked);

    QObject::connect(ui->disableStartupScanCheckBox, &QCheckBox::stateChanged, this,
        [](int newCheckState)
    {
        bool newOption = newCheckState == Qt::Checked ? true : false;
        // don't change initial scan skipping feature value, as it's only relevant on the first scan
        // save the value for the next run
        QSettings settingsInCallback;
        settingsInCallback.beginGroup("Options");
        settingsInCallback.setValue("SkipInitialScan", QVariant(newOption));
        settingsInCallback.endGroup();
    });

    // Shared Cache tab:
    SetupAssetServerTab();

    m_enabledRelocationTypesModel->Reset();
    ui->AssetRelocationExtensionListView->setModel(m_enabledRelocationTypesModel);

    ui->MetaCreationDelayValue->setText(tr("%1 milliseconds").arg(m_guiApplicationManager->GetAssetProcessorManager()->GetMetaCreationDelay()));

}

void MainWindow::BuilderTabSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.size() > 0)
    {
        const auto proxyIndex = selected.indexes().at(0);
        if (!proxyIndex.isValid())
        {
            return;
        }
        const auto& index = m_builderListSortFilterProxy->mapToSource(proxyIndex);

        AssetProcessor::BuilderInfoList builders;
        AssetProcessor::AssetBuilderInfoBus::Broadcast(&AssetProcessor::AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);

        AZ_Assert(index.isValid(), "BuilderTabSelectionChanged index out of bounds");

        const auto builder = builders[index.row()];
        m_builderInfoPatterns->Reset(builder);
        ui->builderInfoMetricsTreeView->setRootIndex(
            m_builderInfoMetricsSort->mapFromSource(m_builderInfoMetrics->index(m_builderData->m_builderGuidToIndex[builder.m_busId], 0)));
        ui->builderInfoMetricsTreeView->expandToDepth(0);
        ui->builderInfoHeaderValueName->setText(builder.m_name.c_str());
        ui->builderInfoDetailsValueType->setText(
            builder.m_builderType == AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal ? "Internal" : "External");
        ui->builderInfoDetailsValueFingerprint->setText(builder.m_analysisFingerprint.c_str());
        ui->builderInfoDetailsValueVersionNumber->setText(QString::number(builder.m_version));
        ui->builderInfoDetailsValueBusId->setText(builder.m_busId.ToFixedString().c_str());
    }
}

namespace MainWindowInternal
{
    enum class PatternColumns
    {
        Enabled = 0,
        Name = 1,
        Type = 2,
        Pattern = 3,
        Remove = 4
    };
}

void MainWindow::SetupAssetServerTab()
{
    using namespace AssetProcessor;
    using namespace MainWindowInternal;

    m_cacheServerData.Reset();

    ui->serverCacheModeOptions->addItem(QString("Inactive"), aznumeric_cast<int>(AssetProcessor::AssetServerMode::Inactive));
    ui->serverCacheModeOptions->addItem(QString("Server"), aznumeric_cast<int>(AssetProcessor::AssetServerMode::Server));
    ui->serverCacheModeOptions->addItem(QString("Client"), aznumeric_cast<int>(AssetProcessor::AssetServerMode::Client));

    // Asset Cache Server support button
    QObject::connect(ui->sharedCacheSupport, &QPushButton::clicked, this,
        []()
        {
            QDesktopServices::openUrl(
                QStringLiteral("https://o3de.org/docs/user-guide/assets/asset-processor/asset-cache-server/"));

        });

    QObject::connect(ui->serverCacheModeOptions,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this,
        [this](int newIndex)
        {
            AssetServerMode inputAssetServerMode = aznumeric_cast<AssetServerMode>(newIndex);
            AssetServerBus::BroadcastResult(this->m_cacheServerData.m_cachingMode, &AssetServerBus::Events::GetRemoteCachingMode);
            if (this->m_cacheServerData.m_cachingMode != inputAssetServerMode)
            {
                this->m_cacheServerData.m_dirty = true;
                this->m_cacheServerData.m_cachingMode = inputAssetServerMode;
                this->m_cacheServerData.m_updateStatus = false;
                this->CheckAssetServerStates();
            }
        });

    // serverAddressToolButton
    ui->serverAddressToolButton->setIcon(QIcon(":Browse_on.png"));
    QObject::connect(ui->serverAddressToolButton, &QToolButton::clicked, this,
        [this]()
        {
            auto path = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Choose remote folder.")));
            if (!path.isEmpty())
            {
                ui->serverAddressLineEdit->setText(path);
            }
        });

    QObject::connect(ui->serverAddressLineEdit, &QLineEdit::textChanged, this,
        [this]()
        {
            SetServerAddress(this->ui->serverAddressLineEdit->text().toUtf8().data());
        });

    QObject::connect(ui->sharedCacheSubmitButton, &QPushButton::clicked, this,
        [this]()
        {
            if (this->m_cacheServerData.m_dirty)
            {
                bool changedServerAddress = false;
                AssembleAssetPatterns();
                AssetServerBus::BroadcastResult(changedServerAddress, &AssetServerBus::Events::SetServerAddress, this->m_cacheServerData.m_serverAddress);
                if (changedServerAddress)
                {
                    AZ::IO::Path projectPath(m_guiApplicationManager->GetProjectPath().toUtf8().data());
                    if (this->m_cacheServerData.Save(projectPath))
                    {
                        AssetServerBus::Broadcast(&AssetServerBus::Events::SetRemoteCachingMode, this->m_cacheServerData.m_cachingMode);
                        this->m_cacheServerData.Reset();
                    }
                }
                else if (this->m_cacheServerData.m_cachingMode != AssetServerMode::Inactive)
                {
                    this->m_cacheServerData.m_statusLevel = CacheServerData::StatusLevel::Error;
                    this->m_cacheServerData.m_statusMessage = AZStd::string::format("**Error**: Invalid server address!");
                }
                this->CheckAssetServerStates();
            }
        });

    QObject::connect(ui->sharedCacheDiscardButton, &QPushButton::clicked, this,
        [this]()
        {
            this->m_cacheServerData.Reset();
            this->ResetAssetServerView();
            this->m_cacheServerData.m_statusLevel = CacheServerData::StatusLevel::Notice;
            this->m_cacheServerData.m_statusMessage = AZStd::string::format("Reset configuration.");
            this->m_cacheServerData.m_updateStatus = true;
            this->CheckAssetServerStates();
        });

    // setting up the patterns table
    QObject::connect(ui->sharedCacheAddPattern, &QPushButton::clicked, this,
        [this]()
        {
            AddPatternRow("New Name", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard, "", true);
            this->m_cacheServerData.m_dirty = true;
            this->m_cacheServerData.m_updateStatus = false;
            this->CheckAssetServerStates();
        });

    ui->sharedCacheTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->sharedCacheTable->horizontalHeader()->setSectionResizeMode(aznumeric_cast<int>(PatternColumns::Enabled), QHeaderView::Fixed);
    ui->sharedCacheTable->horizontalHeader()->setSectionResizeMode(aznumeric_cast<int>(PatternColumns::Remove), QHeaderView::Fixed);
    ui->sharedCacheTable->setAlternatingRowColors(true);

    ResetAssetServerView();
    CheckAssetServerStates();
}

void MainWindow::AddPatternRow(AZStd::string_view name, AssetBuilderSDK::AssetBuilderPattern::PatternType type, AZStd::string_view pattern, bool enable)
{
    using namespace AssetBuilderSDK;
    using namespace MainWindowInternal;

    int row = ui->sharedCacheTable->rowCount();
    ui->sharedCacheTable->insertRow(row);

    auto updateStatus = [this](int)
    {
        this->m_cacheServerData.m_dirty = true;
        this->m_cacheServerData.m_updateStatus = false;
        this->CheckAssetServerStates();
    };

    QObject::connect(ui->sharedCacheTable, &QTableWidget::cellChanged, this,
        [this](int, int)
        {
            this->m_cacheServerData.m_dirty = true;
            this->CheckAssetServerStates();
        });

    // Enabled check mark
    auto* enableChackmark = new QCheckBox();
    enableChackmark->setChecked(enable);
    QObject::connect(enableChackmark, &QCheckBox::stateChanged, ui->sharedCacheTable, updateStatus);
    ui->sharedCacheTable->setCellWidget(row, aznumeric_cast<int>(PatternColumns::Enabled), enableChackmark);
    ui->sharedCacheTable->setColumnWidth(aznumeric_cast<int>(PatternColumns::Enabled), 8);
    enableChackmark->setToolTip(tr("Temporarily disable the pattern by unchecking this box"));

    // Name
    auto* nameWidgetItem = new QTableWidgetItem(name.data());
    ui->sharedCacheTable->setItem(row, aznumeric_cast<int>(PatternColumns::Name), nameWidgetItem);
    nameWidgetItem->setToolTip(tr("Name of the pattern or title name of an asset builder"));

    // Type combo
    auto* combo = new QComboBox();
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->sharedCacheTable, updateStatus);
    combo->addItem("Wildcard", QVariant(AssetBuilderPattern::PatternType::Wildcard));
    combo->addItem("Regex", QVariant(AssetBuilderPattern::PatternType::Regex));
    combo->setCurrentIndex(aznumeric_cast<int>(type));
    ui->sharedCacheTable->setCellWidget(row, aznumeric_cast<int>(PatternColumns::Type), combo);
    combo->setToolTip(tr("Wildcard is a file wild card pattern; Regex is a regular expression pattern"));

    // Pattern
    auto* patternWidgetItem = new QTableWidgetItem(pattern.data());
    ui->sharedCacheTable->setItem(row, aznumeric_cast<int>(PatternColumns::Pattern), patternWidgetItem);
    patternWidgetItem->setToolTip(tr("String pattern to match source assets"));

    // Remove button
    auto* button = new QPushButton();
    button->setFlat(true);
    button->setIcon(QIcon(":/Delete.png"));
    button->setIconSize(QSize(14, 14));
    button->setStyleSheet("QPushButton { background-color: transparent; border: 0px }");
    ui->sharedCacheTable->setCellWidget(row, aznumeric_cast<int>(PatternColumns::Remove), button);
    ui->sharedCacheTable->setColumnWidth(aznumeric_cast<int>(PatternColumns::Remove), 16);
    button->setToolTip(tr("Removes the pattern to be considered for caching"));
    QObject::connect(button, &QPushButton::clicked, this,
        [this]()
        {
            this->ui->sharedCacheTable->removeRow(this->ui->sharedCacheTable->currentRow());
            this->m_cacheServerData.m_dirty = true;
            this->CheckAssetServerStates();
        });
}

void MainWindow::AssembleAssetPatterns()
{
    using namespace AssetBuilderSDK;
    using namespace MainWindowInternal;

    AssetProcessor::RecognizerContainer patternContainer;
    int row = 0;
    for(; row < ui->sharedCacheTable->rowCount(); ++row)
    {
        auto pattern = AZStd::pair<AZStd::string, AssetProcessor::AssetRecognizer>();

        auto* itemName = ui->sharedCacheTable->item(row, aznumeric_cast<int>(PatternColumns::Name));
        auto* itemPattern = ui->sharedCacheTable->item(row, aznumeric_cast<int>(PatternColumns::Pattern));
        auto* itemType = qobject_cast<QComboBox*>(ui->sharedCacheTable->cellWidget(row, aznumeric_cast<int>(PatternColumns::Type)));
        auto* itemCheck = qobject_cast<QCheckBox*>(ui->sharedCacheTable->cellWidget(row, aznumeric_cast<int>(PatternColumns::Enabled)));

        pattern.first = itemName->text().toUtf8().data();

        AZStd::string filePattern { itemPattern->text().toUtf8().data() };
        AssetBuilderPattern::PatternType patternType{};
        auto typeData = itemType->itemData(itemType->currentIndex());
        if (typeData.toInt() == aznumeric_cast<int>(AssetBuilderPattern::PatternType::Regex))
        {
            patternType = AssetBuilderPattern::PatternType::Regex;
        }
        pattern.second.m_patternMatcher = { filePattern, patternType };
        pattern.second.m_checkServer = (itemCheck->checkState() == Qt::CheckState::Checked);

        patternContainer.emplace(AZStd::move(pattern));
    }

    m_cacheServerData.m_patternContainer = AZStd::move(patternContainer);
}

void MainWindow::CheckAssetServerStates()
{
    using namespace AssetProcessor;

    if (m_cacheServerData.m_dirty)
    {
        ui->sharedCacheSubmitButton->setEnabled(true);
        ui->sharedCacheDiscardButton->setEnabled(true);
    }
    else
    {
        ui->sharedCacheSubmitButton->setEnabled(false);
        ui->sharedCacheDiscardButton->setEnabled(false);
    }

    switch (m_cacheServerData.m_statusLevel)
    {
        case CacheServerData::StatusLevel::None:
        {
            m_cacheServerData.m_updateStatus = true;
            ui->sharedCacheStatus->setStyleSheet("QLabel#sharedCacheStatus");
            ui->sharedCacheStatus->setText("");
            break;
        }
        case CacheServerData::StatusLevel::Notice:
        {
            ui->sharedCacheStatus->setText(m_cacheServerData.m_statusMessage.c_str());
            ui->sharedCacheStatus->setProperty("highlight", "blue");
            ui->sharedCacheStatus->style()->unpolish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->style()->polish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->update();
            break;
        }
        case CacheServerData::StatusLevel::Active:
        {
            m_cacheServerData.m_updateStatus = false;
            ui->sharedCacheStatus->setText(m_cacheServerData.m_statusMessage.c_str());
            ui->sharedCacheStatus->setProperty("highlight", "green");
            ui->sharedCacheStatus->style()->unpolish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->style()->polish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->update();
            break;
        }
        case CacheServerData::StatusLevel::Error:
        {
            m_cacheServerData.m_updateStatus = false;
            ui->sharedCacheStatus->setText(m_cacheServerData.m_statusMessage.c_str());
            ui->sharedCacheStatus->setProperty("highlight", "red");
            ui->sharedCacheStatus->style()->unpolish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->style()->polish(ui->sharedCacheStatus);
            ui->sharedCacheStatus->update();
            break;
        }
        default:
            break;
    }

    if (m_cacheServerData.m_updateStatus)
    {
        // Change message to status after a few moments
        QTimer::singleShot(1000 * 5, this, [this] {
            if (this->m_cacheServerData.m_cachingMode == AssetServerMode::Inactive)
            {
                this->m_cacheServerData.m_statusLevel = CacheServerData::StatusLevel::Notice;
                this->m_cacheServerData.m_statusMessage = "Inactive";
            }
            else
            {
                this->m_cacheServerData.m_statusLevel = CacheServerData::StatusLevel::Active;
                this->m_cacheServerData.m_statusMessage = "Active";
            }
            this->m_cacheServerData.m_updateStatus = false;
            this->CheckAssetServerStates();
            });
    }
}

void MainWindow::ResetAssetServerView()
{
    using namespace AssetProcessor;

    ui->serverCacheModeOptions->setCurrentIndex(aznumeric_cast<int>(m_cacheServerData.m_cachingMode));
    ui->serverAddressLineEdit->setText(QString(m_cacheServerData.m_serverAddress.c_str()));

    ui->sharedCacheTable->setRowCount(0);
    for (const auto& pattern : m_cacheServerData.m_patternContainer)
    {
        AddPatternRow(
            pattern.second.m_name,
            pattern.second.m_patternMatcher.GetBuilderPattern().m_type,
            pattern.second.m_patternMatcher.GetBuilderPattern().m_pattern,
            pattern.second.m_checkServer);
    }

    m_cacheServerData.m_dirty = false;
    m_cacheServerData.m_statusLevel = CacheServerData::StatusLevel::None;
    m_cacheServerData.m_statusMessage.clear();
    CheckAssetServerStates();
}

void MainWindow::SetServerAddress(AZStd::string_view serverAddress)
{
    using namespace AssetProcessor;

    AssetServerBus::BroadcastResult(
        this->m_cacheServerData.m_serverAddress,
        &AssetServerBus::Events::GetServerAddress);

    if (this->m_cacheServerData.m_serverAddress != serverAddress)
    {
        this->m_cacheServerData.m_dirty = true;
        this->m_cacheServerData.m_serverAddress = serverAddress;
        this->CheckAssetServerStates();
    }
}

void MainWindow::SetupAssetSelectionCaching()
{
    // Connect the source model resetting to preserve selection and restore it after the model is reset.
    connect(m_sourceModel, &QAbstractItemModel::modelAboutToBeReset, [&]()
    {
        QItemSelection sourceSelection = m_sourceAssetTreeFilterModel->mapSelectionToSource(ui->SourceAssetsTreeView->selectionModel()->selection());

        if (sourceSelection.indexes().count() == 0 || !sourceSelection.indexes()[0].isValid())
        {
            return;
        }
        QModelIndex sourceModelIndex = sourceSelection.indexes()[0];
        AssetProcessor::AssetTreeItem* childItem = static_cast<AssetProcessor::AssetTreeItem*>(sourceModelIndex.internalPointer());
            m_cachedSourceAssetSelection =
                AssetProcessor::SourceAndScanID(childItem->GetData()->m_assetDbName, childItem->GetData()->m_scanFolderID);
    });

    connect(m_sourceModel, &QAbstractItemModel::modelReset, [&]()
    {
            if (m_cachedSourceAssetSelection.first.empty() ||
                m_cachedSourceAssetSelection.second == AzToolsFramework::AssetDatabase::InvalidEntryId)
        {
            return;
        }
        QModelIndex goToIndex = m_sourceModel->GetIndexForSource(m_cachedSourceAssetSelection.first, m_cachedSourceAssetSelection.second);
        // If the cached selection was deleted or is no longer available, clear the selection.
        if (!goToIndex.isValid())
        {
            m_cachedSourceAssetSelection.first.clear();
            m_cachedSourceAssetSelection.second = AzToolsFramework::AssetDatabase::InvalidEntryId;
            ui->ProductAssetsTreeView->selectionModel()->clearSelection();
            // ClearSelection says in the Qt docs that the selectionChange signal will be sent, but that wasn't happening,
            // so force the details panel to refresh.
            ui->sourceAssetDetailsPanel->AssetDataSelectionChanged(QItemSelection(), QItemSelection());
            ui->intermediateAssetDetailsPanel->AssetDataSelectionChanged(QItemSelection(), QItemSelection());
            return;
        }
        m_sourceAssetTreeFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_sourceAssetTreeFilterModel->mapFromSource(goToIndex);
        ui->SourceAssetsTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        ui->SourceAssetsTreeView->selectionModel()->select(filterIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
    });

    // Connect the product model resetting to preserve selection and restore it after the model is reset.
    connect(m_productModel, &QAbstractItemModel::modelAboutToBeReset, [&]()
    {
        QItemSelection productSelection = m_productAssetTreeFilterModel->mapSelectionToSource(ui->ProductAssetsTreeView->selectionModel()->selection());

        if (productSelection.indexes().count() == 0 || !productSelection.indexes()[0].isValid())
        {
            return;
        }
        QModelIndex productModelIndex = productSelection.indexes()[0];
        AssetProcessor::AssetTreeItem* childItem = static_cast<AssetProcessor::AssetTreeItem*>(productModelIndex.internalPointer());
        m_cachedProductAssetSelection = childItem->GetData()->m_assetDbName;
    });

    connect(m_productModel, &QAbstractItemModel::modelReset, [&]()
    {
        if (m_cachedProductAssetSelection.empty())
        {
            return;
        }
        QModelIndex goToIndex = m_productModel->GetIndexForProduct(m_cachedProductAssetSelection);
        // If the cached selection was deleted or is no longer available, clear the selection.
        if (!goToIndex.isValid())
        {
            m_cachedProductAssetSelection.clear();
            ui->ProductAssetsTreeView->selectionModel()->clearSelection();
            // ClearSelection says in the Qt docs that the selectionChange signal will be sent, but that wasn't happening,
            // so force the details panel to refresh.
            ui->productAssetDetailsPanel->AssetDataSelectionChanged(QItemSelection(), QItemSelection());
            return;
        }
        m_productAssetTreeFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_productAssetTreeFilterModel->mapFromSource(goToIndex);
        ui->ProductAssetsTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        ui->ProductAssetsTreeView->selectionModel()->select(filterIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
    });
}

void MainWindow::OnRescanButtonClicked()
{
    m_guiApplicationManager->Rescan();
}

void MainWindow::OnSupportClicked(bool /*checked*/)
{
    QDesktopServices::openUrl(
        QStringLiteral("https://o3de.org/docs/user-guide/assets/pipeline/"));
}

void MainWindow::EditConnection(const QModelIndex& index)
{
    if (index.data(ConnectionManager::UserConnectionRole).toBool())
    {
        ConnectionEditDialog dialog(m_guiApplicationManager->GetConnectionManager(), index, this);

        dialog.exec();
    }
}

void MainWindow::OnConnectionContextMenu(const QPoint& point)
{
    QPersistentModelIndex index = ui->connectionTreeView->indexAt(point);

    bool isUserConnection = index.isValid() && index.data(ConnectionManager::UserConnectionRole).toBool();
    QMenu menu(this);

    QAction* editConnectionAction = menu.addAction("&Edit connection...");
    editConnectionAction->setEnabled(isUserConnection);
    connect(editConnectionAction, &QAction::triggered, this, [index, this] {
        EditConnection(index);
    });

    menu.exec(ui->connectionTreeView->viewport()->mapToGlobal(point));
}

void MainWindow::OnEditConnection(bool /*checked*/)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    Q_ASSERT(selectedIndices.count() > 0);

    // Only edit the first connection. Guaranteed above by the edit connection button only being enabled if one connection is selected
    EditConnection(selectedIndices[0]);
}

void MainWindow::OnAddConnection(bool /*checked*/)
{
    m_guiApplicationManager->GetConnectionManager()->addUserConnection();
}

void MainWindow::OnAllowedListConnectionsListViewClicked()
{
    ui->allowedListRejectedConnectionsListView->clearSelection();
}

void MainWindow::OnRejectedConnectionsListViewClicked()
{
    ui->allowListAllowedListConnectionsListView->clearSelection();
}

void MainWindow::OnAllowedListCheckBoxToggled()
{
    if (!ui->allowedListEnableCheckBox->isChecked())
    {
        //warn this is not safe
        if(QMessageBox::Ok == QMessageBox::warning(this, tr("!!!WARNING!!!"), tr("Turning off allowed listing poses a significant security risk as it would allow any device to connect to your asset processor and that device will have READ/WRITE access to the Asset Processors file system. Only do this if you sure you know what you are doing and accept the risks."),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
            ui->allowedListRejectedConnectionsListView->clearSelection();
            ui->allowListAllowedListConnectionsListView->clearSelection();
            ui->allowedListAddHostNameLineEdit->setEnabled(false);
            ui->allowedListAddHostNameToolButton->setEnabled(false);
            ui->allowedListAddIPLineEdit->setEnabled(false);
            ui->allowedListAddIPToolButton->setEnabled(false);
            ui->allowListAllowedListConnectionsListView->setEnabled(false);
            ui->allowedListRejectedConnectionsListView->setEnabled(false);
            ui->allowedListToAllowedListToolButton->setEnabled(false);
            ui->allowedListToRejectedListToolButton->setEnabled(false);
        }
        else
        {
            ui->allowedListEnableCheckBox->setChecked(true);
        }
    }
    else
    {
        ui->allowedListAddHostNameLineEdit->setEnabled(true);
        ui->allowedListAddHostNameToolButton->setEnabled(true);
        ui->allowedListAddIPLineEdit->setEnabled(true);
        ui->allowedListAddIPToolButton->setEnabled(true);
        ui->allowListAllowedListConnectionsListView->setEnabled(true);
        ui->allowedListRejectedConnectionsListView->setEnabled(true);
        ui->allowedListToAllowedListToolButton->setEnabled(true);
        ui->allowedListToRejectedListToolButton->setEnabled(true);
    }

    m_guiApplicationManager->GetConnectionManager()->AllowedListingEnabled(ui->allowedListEnableCheckBox->isChecked());
}

void MainWindow::OnAddHostNameAllowedListButtonClicked()
{
    QString text = ui->allowedListAddHostNameLineEdit->text();
    const QRegExpValidator *hostnameValidator = static_cast<const QRegExpValidator *>(ui->allowedListAddHostNameLineEdit->validator());
    int pos;
    QValidator::State state = hostnameValidator->validate(text, pos);
    if (state == QValidator::Acceptable)
    {
        auto lineEdit = ui->allowedListAddHostNameLineEdit;
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(text);
        lineEdit->clear();
        // Clear error state set in LineEdit.
        lineEdit->setProperty(AzQtComponents::HasError, false);
        auto errorToolButton = lineEdit->findChild<QToolButton*>(AzQtComponents::ErrorToolButton);
        if (errorToolButton && AzQtComponents::LineEdit::errorIconEnabled(lineEdit))
        {
            errorToolButton->setVisible(false);
        }
    }
}

void MainWindow::OnAddIPAllowedListButtonClicked()
{
    QString text = ui->allowedListAddIPLineEdit->text();
    const QRegExpValidator *ipValidator = static_cast<const QRegExpValidator *>(ui->allowedListAddIPLineEdit->validator());
    int pos;
    QValidator::State state = ipValidator->validate(text, pos);
    if (state== QValidator::Acceptable)
    {
        auto lineEdit = ui->allowedListAddIPLineEdit;
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(text);
        lineEdit->clear();
        // Clear error state set in LineEdit.
        lineEdit->setProperty(AzQtComponents::HasError, false);
        auto errorToolButton = lineEdit->findChild<QToolButton*>(AzQtComponents::ErrorToolButton);
        if (errorToolButton && AzQtComponents::LineEdit::errorIconEnabled(lineEdit))
        {
            errorToolButton->setVisible(false);
        }
    }
}

void MainWindow::OnToRejectedListButtonClicked()
{
    QModelIndexList indices = ui->allowListAllowedListConnectionsListView->selectionModel()->selectedIndexes();
    if(!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.first().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveAddressFromAllowedList(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddRejectedAddress(itemText, true);
    }
}

void MainWindow::OnToAllowedListButtonClicked()
{
    QModelIndexList indices = ui->allowedListRejectedConnectionsListView->selectionModel()->selectedIndexes();
    if (!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.front().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveRejectedAddress(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(itemText);
    }
}

void MainWindow::OnRemoveConnection(bool /*checked*/)
{
    ConnectionManager* manager = m_guiApplicationManager->GetConnectionManager();

    QModelIndexList list = ui->connectionTreeView->selectionModel()->selectedRows();
    for (QModelIndex index: list)
    {
        manager->removeConnection(index);
    }
}

void MainWindow::OnConnectionSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    int selectionCount = selectedIndices.count();

    bool anyUserConnectionsSelected = false;
    for (int i = 0; i < selectionCount; i++)
    {
        QModelIndex selectedIndex = selectedIndices[i];
        if (selectedIndex.data(ConnectionManager::UserConnectionRole).toBool())
        {
            anyUserConnectionsSelected = true;
            break;
        }
    }

    ui->removeConnectionButton->setEnabled(anyUserConnectionsSelected);
    ui->editConnectionButton->setEnabled(anyUserConnectionsSelected && (selectionCount == 1));
}


MainWindow::~MainWindow()
{
    m_guiApplicationManager = nullptr;
    delete ui;
}



void MainWindow::ShowWindow()
{
    show();
    AzQtComponents::bringWindowToTop(this);
}


void MainWindow::SyncAllowedListAndRejectedList(QStringList allowedList, QStringList rejectedList)
{
    m_allowedListAddresses.setStringList(allowedList);
    m_rejectedAddresses.setStringList(rejectedList);
}

void MainWindow::FirstTimeAddedToRejectedList(QString ipAddress)
{
    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setText(tr("!!!Rejected Connection!!!"));
    msgBox->setInformativeText(ipAddress + tr(" tried to connect and was rejected because it was not on the allowed list. If you want this connection to be allowed go to connections tab and add it to allowed list."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::NonModal);
    msgBox->setModal(false);
    msgBox->show();
}

void MainWindow::SaveLogPanelState()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->SaveState();
    }
}

AssetProcessor::ProductDependencyTreeItem* MainWindow::GetProductAssetFromDependencyTreeView(bool isOutgoing, const QPoint& pos)
{
    const QModelIndex assetIndex =
        (isOutgoing ? ui->productAssetDetailsPanel->GetOutgoingProductDependenciesTreeView()->indexAt(pos)
                    : ui->productAssetDetailsPanel->GetIncomingProductDependenciesTreeView()->indexAt(pos));
    if (!assetIndex.isValid())
    {
        return static_cast<AssetProcessor::ProductDependencyTreeItem*>(nullptr);
    }
    return static_cast<AssetProcessor::ProductDependencyTreeItem*>(assetIndex.internalPointer());
}

void MainWindow::ShowOutgoingProductDependenciesContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    const ProductDependencyTreeItem* cachedAsset = GetProductAssetFromDependencyTreeView(true, pos);

    if (!cachedAsset || !cachedAsset->GetData())
    {
        return;
    }
    AZStd::string productName = cachedAsset->GetData()->m_productName;

    QMenu menu(this);
    menu.setToolTipsVisible(true);
    QAction* productAction = menu.addAction(
        tr("Go to product asset"), this,
        [&, productName]()
        {
            ui->sourceAssetDetailsPanel->GoToProduct(productName);
        });
    if (productName.empty())
    {
        productAction->setDisabled(true);
        productAction->setToolTip(tr("This asset is currently selected."));
    }
    else
    {
        productAction->setToolTip(tr("Selects this asset."));
    }
    menu.exec(ui->productAssetDetailsPanel->GetOutgoingProductDependenciesTreeView()->viewport()->mapToGlobal(pos));
}

void MainWindow::ShowIncomingProductDependenciesContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    const ProductDependencyTreeItem* cachedAsset = GetProductAssetFromDependencyTreeView(false, pos);

    if (!cachedAsset || !cachedAsset->GetData())
    {
        return;
    }

    AZStd::string productName = cachedAsset->GetData()->m_productName;
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    QAction* productAction = menu.addAction(
        tr("Go to product asset"), this,
        [&, productName]()
        {
            ui->sourceAssetDetailsPanel->GoToProduct(productName);
        });
    if (productName.empty())
    {
        productAction->setDisabled(true);
        productAction->setToolTip(tr("This asset is currently selected."));
    }
    else
    {
        productAction->setToolTip(tr("Selects this asset."));
    }
    menu.exec(ui->productAssetDetailsPanel->GetIncomingProductDependenciesTreeView()->viewport()->mapToGlobal(pos));
}


void MainWindow::ResetTimers()
{
    m_scanTime = m_analysisTime = m_processTime = 0;
    m_scanTimer.restart();
    m_analysisTimer.invalidate();
    m_processTimer.invalidate();
}

void MainWindow::CheckStartAnalysisTimers()
{
    if (m_scanTimer.isValid())
    {
        m_scanTime = m_scanTimer.elapsed();
        m_scanTimer.invalidate();
    }
    if (!m_analysisTimer.isValid() && !m_analysisTime)
    {
        m_analysisTimer.start();
    }
}

void MainWindow::CheckStartProcessTimers()
{
    if (m_analysisTimer.isValid())
    {
        m_analysisTime = m_analysisTimer.restart();
        m_analysisTimer.invalidate();
    }

    if (!m_processTimer.isValid() && !m_processTime)
    {
        m_processTimer.start();
    }
}

void MainWindow::CheckEndAnalysisTimer()
{
    if (m_analysisTimer.isValid() && !m_analysisTime)
    {
        m_analysisTime = m_analysisTimer.elapsed();
        m_analysisTimer.invalidate();
    }
}

void MainWindow::CheckEndProcessTimer()
{
    if (m_processTimer.isValid() && !m_processTime)
    {
        m_processTime = m_processTimer.elapsed();
        m_processTimer.invalidate();
    }
}

QString MainWindow::FormatStringTime(qint64 msTime) const
{
    int msecInt{ static_cast<int>(msTime) };
    int timeHrs{ msecInt / (1000 * 60 * 60) };
    msecInt = msecInt % (1000 * 60 * 60);

    int timeMins{ msecInt / (1000 * 60) };
    msecInt = msecInt % (1000 * 60 );

    int timeSecs{ msecInt / 1000 };
    int timeMsec{ msecInt % 1000 };

    QTime timeVal{ timeHrs, timeMins, timeSecs, timeMsec };

    if (timeHrs)
    {
        return timeVal.toString("h:mm:ss.z");
    }
    return timeVal.toString("mm:ss.z");
}

void MainWindow::IntervalAssetTabFilterRefresh()
{
    if (ui->buttonList->currentIndex() != static_cast<int>(DialogStackIndex::Assets)
        || !ui->assetDataFilteredSearchWidget->hasStringFilter())
    {
        return;
    }

    if (!m_filterRefreshTimer.isValid())
    {
        m_filterRefreshTimer.start();
    }

    if (m_filterRefreshTimer.elapsed() >= AssetTabFilterUpdateIntervalMs)
    {
        emit ui->assetDataFilteredSearchWidget->TextFilterChanged(ui->assetDataFilteredSearchWidget->textFilter());
        m_filterRefreshTimer.restart();
    }
}

void MainWindow::ShutdownAssetTabFilterRefresh()
{
    if (m_filterRefreshTimer.isValid())
    {
        emit ui->assetDataFilteredSearchWidget->TextFilterChanged(ui->assetDataFilteredSearchWidget->textFilter());
    }

    m_filterRefreshTimer.invalidate();
}

void MainWindow::OnAssetProcessorStatusChanged(const AssetProcessor::AssetProcessorStatusEntry entry)
{
    using namespace AssetProcessor;
    QString text;
    switch (entry.m_status)
    {
    case AssetProcessorStatus::Initializing_Gems:
        text = tr("Initializing Gem...%1").arg(entry.m_extraInfo);
        break;
    case AssetProcessorStatus::Initializing_Builders:
        text = tr("Initializing Builders...");
        break;
    case AssetProcessorStatus::Scanning_Started:
        ResetTimers();
        text = tr("Scanning...");
        break;
    case AssetProcessorStatus::Analyzing_Jobs:
        CheckStartAnalysisTimers();
        m_createJobCount = entry.m_count;

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text += tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);

            if (!entry.m_extraInfo.isEmpty())
            {
                text += tr("<p style='font-size:small;'>%1</p>").arg(entry.m_extraInfo);
            }

            ui->timerContainerWidget->setVisible(false);
            ui->productAssetDetailsPanel->SetScanQueueEnabled(false);

            IntervalAssetTabFilterRefresh();
        }
        else
        {
            CheckEndAnalysisTimer();
            text = tr("Idle...");
            ui->timerContainerWidget->setVisible(true);
            m_guiApplicationManager->RemoveOldTempFolders();
            // Once the asset processor goes idle, enable the scan queue.
            // This minimizes the potential for over-reporting missing dependencies (if a queued job would resolve them)
            // and prevents running too many threads with too much work (scanning + processing jobs both take time).
            ui->productAssetDetailsPanel->SetScanQueueEnabled(true);

            ShutdownAssetTabFilterRefresh();
        }
        break;
    case AssetProcessorStatus::Processing_Jobs:
        CheckStartProcessTimers();
        m_processJobsCount = entry.m_count;

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text = tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);
            ui->timerContainerWidget->setVisible(false);
            ui->productAssetDetailsPanel->SetScanQueueEnabled(false);

            IntervalAssetTabFilterRefresh();
        }
        else
        {
            CheckEndProcessTimer();
            text = tr("Idle...");
            ui->timerContainerWidget->setVisible(true);
            m_guiApplicationManager->RemoveOldTempFolders();
            // Once the asset processor goes idle, enable the scan queue.
            // This minimizes the potential for over-reporting missing dependencies (if a queued job would resolve them)
            // and prevents running too many threads with too much work (scanning + processing jobs both take time).
            ui->productAssetDetailsPanel->SetScanQueueEnabled(true);
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel,
                "Job processing completed. Asset Processor is currently idle. Process time: %s\n",
                FormatStringTime(m_processTime).toUtf8().constData());
            ShutdownAssetTabFilterRefresh();
        }
        break;
    default:
        text = QString();
        break;
    }

    ui->APStatusValueLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Status"))
        .arg(text));

    ui->lastScanTimer->setText(FormatStringTime(m_scanTime));
    ui->analysisTimer->setText(FormatStringTime(m_analysisTime));
    ui->processingTimer->setText(FormatStringTime(m_processTime));
}

void MainWindow::HighlightAsset(QString assetPath)
{
    // Make sure that the currently active tab is the job list
    ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));

    // clear all filters
    ui->jobFilteredSearchWidget->ClearTextFilter();
    ui->jobFilteredSearchWidget->ClearTypeFilter();

    // jobs are listed with relative source asset paths, so we need to remove
    // the scan folder prefix from the absolute path
    bool success = true;
    AZStd::vector<AZStd::string> scanFolders;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders, scanFolders);
    if (success)
    {
        for (auto& scanFolder : scanFolders)
        {
            if (assetPath.startsWith(scanFolder.c_str(), Qt::CaseInsensitive))
            {
                // + 1 for the path separator
                assetPath = assetPath.mid(static_cast<int>(scanFolder.size() + 1));
                break;
            }
        }
    }

    // apply the filter for our asset path
    ui->jobFilteredSearchWidget->SetTextFilter(assetPath);

    // select the first item in the list
    ui->jobTreeView->setCurrentIndex(m_jobSortFilterProxy->index(0, 0));
}

void MainWindow::OnAssetTabChange(int index)
{
    AssetTabIndex tabIndex = static_cast<AssetTabIndex>(index);
    switch (tabIndex)
    {
    case AssetTabIndex::Source:
        ui->sourceAssetDetailsPanel->setVisible(true);
        ui->intermediateAssetDetailsPanel->setVisible(false);
        ui->productAssetDetailsPanel->setVisible(false);
        break;
    case AssetTabIndex::Product:
        ui->sourceAssetDetailsPanel->setVisible(false);
        ui->intermediateAssetDetailsPanel->setVisible(false);
        ui->productAssetDetailsPanel->setVisible(true);
        break;
    case AssetTabIndex::Intermediate:
        ui->sourceAssetDetailsPanel->setVisible(false);
        ui->intermediateAssetDetailsPanel->setVisible(true);
        ui->productAssetDetailsPanel->setVisible(false);
        break;
    }
}

void MainWindow::ApplyConfig()
{
    using namespace AssetProcessor;

    // Asset Status
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnStatus, m_config.jobStatusColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnSource, m_config.jobSourceColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnPlatform, m_config.jobPlatformColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnJobKey, m_config.jobKeyColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnCompleted, m_config.jobCompletedColumnWidth);

    // Event Log Details
    ui->jobLogTableView->header()->resizeSection(AzToolsFramework::Logging::LogTableModel::ColumnType, m_config.logTypeColumnWidth);
}

MainWindow::LogSortFilterProxy::LogSortFilterProxy(QObject* parentOjbect) : QSortFilterProxyModel(parentOjbect)
{
}

bool MainWindow::LogSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_logTypes.empty())
    {
        QModelIndex testIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Q_ASSERT(testIndex.isValid());
        auto indexLogType = static_cast<AzToolsFramework::Logging::LogLine::LogType>(testIndex.data(AzToolsFramework::Logging::LogTableModel::LogTypeRole).toInt());
        if (!m_logTypes.contains(indexLogType))
        {
            return false;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void MainWindow::LogSortFilterProxy::onTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    beginResetModel();
    m_logTypes.clear();

    for (auto typeIter = activeTypeFilters.begin(), endIter = activeTypeFilters.end(); typeIter != endIter; ++typeIter)
    {
        m_logTypes.insert(static_cast<AzToolsFramework::Logging::LogLine::LogType>(typeIter->metadata.toInt()));
    }
    endResetModel();
}

void MainWindow::SetContextLogDetails(const QMap<QString, QString>& details)
{
    auto model = qobject_cast<AzToolsFramework::Logging::ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());
    model->SetDetails(details);

    if (!details.isEmpty())
    {
        int tableRows = details.size();
        if(tableRows > m_config.contextDetailsTableMaximumRows)
        {
            tableRows = m_config.contextDetailsTableMaximumRows;
        }

        ui->jobContextLogTableView->setMinimumHeight(ui->jobContextLogTableView->sizeHintForRow(0) * tableRows);
        ui->jobDialogSplitter->setSizes({ ui->jobDialogSplitter->height(), ui->jobDialogSplitter->height(), 0 });
    }

    ui->jobContextContainer->setVisible(!details.isEmpty());
}

void MainWindow::ClearContextLogDetails()
{
    SetContextLogDetails({});
}

void MainWindow::UpdateJobLogView(QModelIndex selectedIndex)
{
    if (!m_logsModel)
    {
        return;
    }

    // SelectionMode is set to QAbstractItemView::SingleSelection so there is only one selected item at a time
    AZStd::string jobLog = m_jobSortFilterProxy->data(selectedIndex, AssetProcessor::JobsModel::DataRoles::logRole).toString().toUtf8().data();

    m_logsModel->Clear();
    AzToolsFramework::Logging::LogLine::ParseLog(jobLog.c_str(), jobLog.length() + 1,
        AZStd::bind(&AzToolsFramework::Logging::LogTableModel::AppendLineAsync, m_logsModel, AZStd::placeholders::_1));
    m_logsModel->CommitLines();
    ui->jobLogTableView->scrollToBottom();
    ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogTableView);
}

void MainWindow::JobSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.indexes().length() != 0)
    {
        UpdateJobLogView(selected.indexes()[0]);
    }
    // The only alternative is that there has been only a deselection, as otherwise both selected and deselected would be empty
    else
    {
        ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogPlaceholderLabel);
    }

    ClearContextLogDetails();
}

void MainWindow::JobStatusChanged([[maybe_unused]] AssetProcessor::JobEntry entry, [[maybe_unused]] AzToolsFramework::AssetSystem::JobStatus status)
{
    QModelIndexList selectedIndexList = ui->jobTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexList.empty())
    {
        return;
    }

    QModelIndex selectedIndex = selectedIndexList.at(0);

    // retrieve cachedJobInfo for the selected entry
    const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(selectedIndex);
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_jobsModel->getItem(sourceIndex.row());
    AZ_Assert(cachedJobInfo, "Failed to find cached job info");
    if (!cachedJobInfo)
    {
        return;
    }

    // ignore the notification if it's not for the selected entry
    if (cachedJobInfo->m_elementId.GetSourceAssetReference() != entry.m_sourceAssetReference)
    {
        return;
    }

    UpdateJobLogView(selectedIndex);
}

void MainWindow::JobLogSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.count() == 1)
    {
        const QMap<QString, QString> details = selected.indexes().first().data(AzToolsFramework::Logging::LogTableModel::DetailsRole).value<QMap<QString, QString>>();
        SetContextLogDetails(details);
    }
    else
    {
        ClearContextLogDetails();
    }
}

void MainWindow::DesktopOpenJobLogs()
{
    char resolvedDir[AZ_MAX_PATH_LEN * 2];
    QString currentDir;
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(AssetUtilities::ComputeJobLogFolder().c_str(), resolvedDir, sizeof(resolvedDir));

    currentDir = QString::fromUtf8(resolvedDir);

    if (QFile::exists(currentDir))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir));
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[Error] Logs folder (%s) does not exists.\n", currentDir.toUtf8().constData());
    }
}

void MainWindow::ResetLoggingPanel()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Debug", "", ""));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Messages", "", "", true, true, true, false));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
    }
}

void MainWindow::ShowJobLogContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* line = menu.addAction(tr("Copy line"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::LogLineTextRole).toString());
    });
    QAction* lineDetails = menu.addAction(tr("Copy line with details"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::CompleteLogLineTextRole).toString());
    });
    menu.addAction(tr("Copy all"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(m_logsModel->toString(true));
    });

    if (!sourceIndex.isValid())
    {
        line->setEnabled(false);
        lineDetails->setEnabled(false);
    }

    menu.exec(ui->jobLogTableView->viewport()->mapToGlobal(pos));
}

static QString FindAbsoluteFilePath(const AssetProcessor::CachedJobInfo* cachedJobInfo)
{
    return cachedJobInfo ? cachedJobInfo->m_elementId.GetSourceAssetReference().AbsolutePath().c_str() : QString{};
};

static void SendShowInAssetBrowserResponse(const QString& filePath, ConnectionManager* connectionManager, unsigned int connectionId, QByteArray data)
{
    Connection* connection = connectionManager->getConnection(connectionId);
    if (!connection)
    {
        return;
    }

    AzToolsFramework::AssetSystem::WantAssetBrowserShowResponse response;
    bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(data.data(), data.size(), response);
    AZ_Assert(readFromStream, "AssetProcessor failed to deserialize from stream");
    if (!readFromStream)
    {
        return;
    }

#ifdef AZ_PLATFORM_WINDOWS
    // Required on Windows to allow the other process to come to the foreground
    AllowSetForegroundWindow(response.m_processId);
#endif // #ifdef AZ_PLATFORM_WINDOWS

    AzToolsFramework::AssetSystem::AssetBrowserShowRequest message;
    message.m_filePath = filePath.toUtf8().data();
    connection->Send(AzFramework::AssetSystem::DEFAULT_SERIAL, message);
}

void MainWindow::ShowJobViewContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;

    auto cachedJobInfoAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
        return m_jobsModel->getItem(sourceIndex.row());
    };

    const CachedJobInfo* item = cachedJobInfoAt(pos);

    if (!item)
    {
        return;
    }
    QMenu menu;
    menu.setToolTipsVisible(true);

    // Find a connection to an Editor, if it exists. This is used for showing this asset in the Asset Browser, if the Editor is available.
    ConnectionManager* connectionManager = m_guiApplicationManager->GetConnectionManager();
    Connection* editorConnection = nullptr;
    auto& connectionMap = connectionManager->getConnectionMap();
    auto connections = connectionMap.values();
    for (auto connection : connections)
    {
        using namespace AzFramework::AssetSystem;
        // If there is more than one Editor connected, this will only show this asset in the first connected Editor's asset browser.
        if (connection->Identifier() == ConnectionIdentifiers::Editor)
        {
            editorConnection = connection;
            break;
        }
    }

    QAction* showInAssetBrowserAction = menu.addAction("Show in Asset Browser", this, [&]()
    {
        if (!editorConnection)
        {
            return;
        }

        QString filePath = FindAbsoluteFilePath(item);

        AzToolsFramework::AssetSystem::WantAssetBrowserShowRequest requestMessage;

        // Ask the Editor, and only the Editor, if it wants to receive
        // the message for showing an asset in the AssetBrowser.
        // This also allows the Editor to send back it's Process ID, which
        // allows the Windows platform to call AllowSetForegroundWindow()
        // which is required to bring the Editor window to the foreground
        unsigned int connectionId = editorConnection->ConnectionId();
        editorConnection->SendRequest(
            requestMessage,
            [connectionManager, connectionId, filePath](AZ::u32 /*type*/, QByteArray callbackData)
            {
                    SendShowInAssetBrowserResponse(filePath, connectionManager, connectionId, callbackData);
                });
        });
    // Disable the menu option if there is no Editor connection.
    showInAssetBrowserAction->setEnabled(editorConnection != nullptr);
    if (!editorConnection)
    {
        showInAssetBrowserAction->setToolTip(tr("Showing in the Asset Browser requires an active connection to the Editor."));
    }
    else
    {
        showInAssetBrowserAction->setToolTip(tr("Sends a request to the Editor to display this asset in the Asset Browser."));
    }

    menu.addAction("Reprocess Source Asset", this, [this, &item]()
    {
        QString pathToSource = FindAbsoluteFilePath(item);
        m_guiApplicationManager->GetAssetProcessorManager()->RequestReprocess(pathToSource);
    });

    // Only completed items will be available in the assets tab.
    QAction* assetTabSourceAction = menu.addAction(tr("View source asset"), this, [&]()
    {
        ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
        ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
        ui->sourceAssetDetailsPanel->GoToSource(item->m_elementId.GetSourceAssetReference().AbsolutePath().c_str());
    });

    // Get the builder index outside the action, so the action can be disabled if it is not available.
    QModelIndex builderIndex = m_builderList->GetIndexForBuilder(item->m_builderGuid);

    QAction* assetTabBuilderAction = menu.addAction(tr("View builder"), this, [&]()
    {
        ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Builders));
        ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Builders));

        QModelIndex filterIndex = m_builderListSortFilterProxy->mapFromSource(builderIndex);
        ui->builderList->scrollTo(filterIndex);
        ui->builderList->selectionModel()->setCurrentIndex(filterIndex, QItemSelectionModel::ClearAndSelect);
    });
    assetTabBuilderAction->setEnabled(builderIndex.isValid());
    if (builderIndex.isValid())
    {
        assetTabBuilderAction->setToolTip(tr("Show the builder for this job in the Builder tab."));
    }
    else
    {
        assetTabBuilderAction->setToolTip(tr("The builder is unavailable for this asset."));
    }
    

    if (item->m_jobState != AzToolsFramework::AssetSystem::JobStatus::Completed)
    {
        QString disabledActionTooltip(tr("Only completed jobs are available in the Assets tab."));
        assetTabSourceAction->setToolTip(disabledActionTooltip);
        assetTabSourceAction->setDisabled(true);

        // Disabled menus don't support tooltips, so add it as an action, instead.
        QAction* productMenuAction = menu.addAction(productMenuTitle);
        productMenuAction->setToolTip(disabledActionTooltip);
        productMenuAction->setDisabled(true);
    }
    else
    {
        assetTabSourceAction->setToolTip(tr("Show the source asset for this job in the Assets tab."));

        AssetRightClickMenuResult productAssetMenu(SetupProductAssetRightClickMenu(&menu));
        AssetRightClickMenuResult intermediateAssetMenu(SetupIntermediateAssetRightClickMenu(&menu));

        auto productMenuItemClicked = [this, &menu](QListWidgetItem* item)
        {
            if (item)
            {
                ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
                ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
                AZStd::string productFromQString(item->text().toUtf8().data());
                ui->sourceAssetDetailsPanel->GoToProduct(productFromQString);
                menu.close();
            }
        };
        connect(productAssetMenu.m_listWidget, &QListWidget::itemClicked, this, productMenuItemClicked);

        auto intermediateMenuItemClicked = [this, &menu](QListWidgetItem* item)
        {
            if (item)
            {
                ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
                ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));

                QVariant data = item->data(Qt::UserRole);
                ui->sourceAssetDetailsPanel->GoToSource(data.toString().toUtf8().constData());
                menu.close();
            }
        };
        connect(intermediateAssetMenu.m_listWidget, &QListWidget::itemClicked, this, intermediateMenuItemClicked);

        int intermediateCount = 0;
        int productCount = 0;
        m_sharedDbConnection->QueryJobByJobRunKey(
            item->m_jobRunKey,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            m_sharedDbConnection->QueryProductByJobID(
                jobEntry.m_jobID,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
            {
                if (productEntry.m_productName.empty())
                {
                    return true;
                }

                auto productPath = AssetUtilities::ProductPath::FromDatabasePath(productEntry.m_productName);

                if (IsProductOutputFlagSet(productEntry, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset))
                {
                    ++intermediateCount;
                    auto productItem = new QListWidgetItem { AssetUtilities::StripAssetPlatform(productEntry.m_productName), intermediateAssetMenu.m_listWidget };
                    productItem->setData(Qt::UserRole, productPath.GetIntermediatePath().c_str());
                    intermediateAssetMenu.m_listWidget->addItem(productItem);
                }
                else
                {
                    ++productCount;
                    auto productItem = new QListWidgetItem{ productEntry.m_productName.c_str(), productAssetMenu.m_listWidget };
                    productAssetMenu.m_listWidget->addItem(productItem);
                }
                return true; // Keep iterating, add all products.
            });
            return false; // Stop iterating, there should only be one job with this run key.
        });

        if (productCount == 0)
        {
            CreateDisabledAssetRightClickMenu(&menu, productAssetMenu.m_assetMenu, productMenuTitle, tr("This job created no products."));
            productAssetMenu.m_assetMenu = nullptr;
        }
        else
        {
            ResizeAssetRightClickMenuList(productAssetMenu.m_listWidget, productCount);
        }

        if (intermediateCount == 0)
        {
            CreateDisabledAssetRightClickMenu(&menu, intermediateAssetMenu.m_assetMenu, intermediateMenuTitle, tr("This job created no intermediate product assets."));
            intermediateAssetMenu.m_assetMenu = nullptr;
        }
        else
        {
            ResizeAssetRightClickMenuList(intermediateAssetMenu.m_listWidget, intermediateCount);
        }
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AzQtComponents::ShowFileOnDesktop(FindAbsoluteFilePath(item));
    });
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view the source asset for this job."));

    menu.addAction(tr("Open"), this, [&]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(FindAbsoluteFilePath(item)));
    });

    menu.addAction(tr("Copy"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(FindAbsoluteFilePath(item)));
    });

    // Get the internal path to the log file
    const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
    const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
    QVariant pathVariant = m_jobsModel->data(sourceIndex, JobsModel::logFileRole);

    // Get the absolute path of the log file
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    char resolvedPath[AZ_MAX_PATH_LEN];
    fileIO->ResolvePath(pathVariant.toByteArray().constData(), resolvedPath, AZ_MAX_PATH_LEN);

    QFileInfo fileInfo(resolvedPath);
    auto openLogFile = menu.addAction(tr("Open log file"), this, [&]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
    });
    openLogFile->setEnabled(fileInfo.exists());

    auto logDir = fileInfo.absoluteDir();
    auto openLogFolder = menu.addAction(tr("Open folder with log file"), this, [&]()
    {
        if (fileInfo.exists())
        {
            AzQtComponents::ShowFileOnDesktop(fileInfo.absoluteFilePath());
        }
        else
        {
            // If the file doesn't exist, but the directory does, just open the directory
            AzQtComponents::ShowFileOnDesktop(logDir.absolutePath());
        }
    });
    // Only open and show the folder if the file actually exists, otherwise it's confusing
    openLogFolder->setEnabled(fileInfo.exists());

    menu.exec(ui->jobTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::SelectJobAndMakeVisible(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }
    // Make sure the job is visible, clear any existing filters.
    // This has to be done before getting the filter index, because it will change.
    ui->jobFilteredSearchWidget->ClearTextFilter();
    ui->jobFilteredSearchWidget->ClearTypeFilter();

    ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));
    ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));
    QModelIndex proxyIndex = m_jobSortFilterProxy->mapFromSource(index);
    ui->jobTreeView->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::EnsureVisible);
    // This isn't an asset tree, but use the same selection mode when selecting this row.
    // Setting the current index works a bit better than just selecting, because the item will be treated as
    // active for purposes of keyboard navigation and additional row highlighting (if the tree view itself gains focus)
    ui->jobTreeView->selectionModel()->setCurrentIndex(proxyIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
}

void MainWindow::ShowIntermediateAssetContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    auto sourceAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->IntermediateAssetsTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_intermediateAssetTreeFilterModel->mapToSource(proxyIndex);
        return static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
    };

    const AssetTreeItem* cachedAsset = sourceAt(pos);

    if (!cachedAsset)
    {
        return;
    }

    // Intermediate assets are functionally source assets, with an upstream source asset that generated them.
    QMenu menu(this);

    QAction* sourceAssetAction = menu.addAction(tr("View source asset"), this, [&]()
    {
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(cachedAsset->GetData());
        // Generate the product path for this intermediate asset.
        AZStd::string productPathForIntermediateAsset =
            AssetUtilities::GetRelativeProductPathForIntermediateSourcePath(sourceItemData->m_assetDbName);

        // Retrieve the source asset for that product.
        m_sharedDbConnection->QueryProductByProductName(
            productPathForIntermediateAsset.c_str(),
            [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
        {
                m_sharedDbConnection->QuerySourceByProductID(
                    productEntry.m_productID,
                    [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                {
                    ui->sourceAssetDetailsPanel->GoToSource(SourceAssetReference(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str()).AbsolutePath().c_str());
                    return false; // Don't keep iterating
                });
                return false;
        });
    });
    sourceAssetAction->setToolTip(tr("Show the source asset for this intermediate asset."));

    BuildSourceAssetTreeContextMenu(menu, *cachedAsset);

    menu.exec(ui->SourceAssetsTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::ShowSourceAssetContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    auto sourceAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->SourceAssetsTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_sourceAssetTreeFilterModel->mapToSource(proxyIndex);
        return static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
    };

    const AssetTreeItem* cachedAsset = sourceAt(pos);

    if (!cachedAsset)
    {
        return;
    }
    QMenu menu(this);
    BuildSourceAssetTreeContextMenu(menu, *cachedAsset);

    menu.exec(ui->SourceAssetsTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::BuildSourceAssetTreeContextMenu(QMenu& menu, const AssetProcessor::AssetTreeItem& sourceAssetTreeItem)
{
    using namespace AssetProcessor;
    menu.setToolTipsVisible(true);
    const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(sourceAssetTreeItem.GetData());


    QString jobMenuText(tr("View job..."));

    AssetRightClickMenuResult productAssetMenu(SetupProductAssetRightClickMenu(&menu));
    AssetRightClickMenuResult intermediateAssetMenu(SetupIntermediateAssetRightClickMenu(&menu));

    QMenu* jobMenu = menu.addMenu(jobMenuText);
    jobMenu->setToolTipsVisible(true);

    auto productMenuItemClicked = [this, &menu](QListWidgetItem* item)
    {
        if (item)
        {
            AZStd::string productFromQString(item->text().toUtf8().data());
            ui->sourceAssetDetailsPanel->GoToProduct(productFromQString);
            menu.close();
        }
    };
    connect(productAssetMenu.m_listWidget, &QListWidget::itemClicked, this, productMenuItemClicked);

    auto intermediateMenuItemClicked = [this, &menu](QListWidgetItem* item)
    {
        if (item)
        {
            QVariant data = item->data(Qt::UserRole);
            ui->sourceAssetDetailsPanel->GoToSource(data.toString().toUtf8().constData());
            menu.close();
        }
    };
    connect(intermediateAssetMenu.m_listWidget, &QListWidget::itemClicked, this, intermediateMenuItemClicked);

    int intermediateCount = 0;
    int productCount = 0;
    SourceAssetReference sourceAsset(sourceItemData->m_scanFolderInfo.m_scanFolder.c_str(), sourceItemData->m_sourceInfo.m_sourceName.c_str());
    m_sharedDbConnection->QueryJobBySourceID(sourceItemData->m_sourceInfo.m_sourceID,
        [this, &jobMenu, &productAssetMenu, &intermediateAssetMenu, &intermediateCount, &productCount, sourceAsset](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
    {
        QAction* jobAction = jobMenu->addAction(tr("with key %1 for platform %2").arg(jobEntry.m_jobKey.c_str(), jobEntry.m_platform.c_str()), this, [this, jobEntry, sourceAsset]()
        {
            QModelIndex jobIndex = m_jobsModel->GetJobFromSourceAndJobInfo(sourceAsset, jobEntry.m_platform, jobEntry.m_jobKey);
            SelectJobAndMakeVisible(jobIndex);
        });
        jobAction->setToolTip(tr("Show this job in the Jobs tab."));

        m_sharedDbConnection->QueryProductByJobID(
        jobEntry.m_jobID,
        [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
        {
            if (productEntry.m_productName.empty())
            {
                return true;
            }

            auto productPath = AssetUtilities::ProductPath::FromDatabasePath(productEntry.m_productName);

            if (IsProductOutputFlagSet(productEntry, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset))
            {
                ++intermediateCount;
                auto* productItem =
                    new QListWidgetItem{ AZStd::string(AssetUtilities::StripAssetPlatformNoCopy(productEntry.m_productName)).c_str(),
                                            intermediateAssetMenu.m_listWidget };
                productItem->setData(Qt::UserRole, productPath.GetIntermediatePath().c_str());
                intermediateAssetMenu.m_listWidget->addItem(productItem);
            }
            else
            {
                ++productCount;
                productAssetMenu.m_listWidget->addItem(productEntry.m_productName.c_str());
            }
            return true; // Keep iterating, add all products.
        });
        return true;
    });

    if (productCount == 0)
    {
        CreateDisabledAssetRightClickMenu(&menu, productAssetMenu.m_assetMenu, productMenuTitle, tr("This source asset has no products."));
        productAssetMenu.m_assetMenu = nullptr;
    }
    else
    {
        ResizeAssetRightClickMenuList(productAssetMenu.m_listWidget, productCount);
    }

    if (intermediateCount == 0)
    {
        CreateDisabledAssetRightClickMenu(&menu, intermediateAssetMenu.m_assetMenu, intermediateMenuTitle, tr("This job created no intermediate product asset."));
        intermediateAssetMenu.m_assetMenu = nullptr;
    }
    else
    {
        ResizeAssetRightClickMenuList(intermediateAssetMenu.m_listWidget, intermediateCount);
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(sourceAssetTreeItem);
        if (pathToSource.IsSuccess())
        {
            AzQtComponents::ShowFileOnDesktop(pathToSource.GetValue());
        }
    });
    QString fileOrFolder(sourceAssetTreeItem.getChildCount() > 0 ? tr("folder") : tr("file"));
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view this %1.").arg(fileOrFolder));

    QAction* copyFullPathAction = menu.addAction(tr("Copy full path"), this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(sourceAssetTreeItem);
        if (pathToSource.IsSuccess())
        {
            QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(pathToSource.GetValue()));
        }
    });

    copyFullPathAction->setToolTip(tr("Copies the full path to this file to your clipboard."));

    QString reprocessFolder{ tr("Reprocess Folder") };
    QString reprocessFile{ tr("Reprocess File") };

    QAction* reprocessAssetAction = menu.addAction(sourceAssetTreeItem.getChildCount() ? reprocessFolder : reprocessFile, this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(sourceAssetTreeItem);
        m_guiApplicationManager->GetAssetProcessorManager()->RequestReprocess(pathToSource.GetValue());
    });

    QString reprocessFolderTip{ tr("Put the source assets in the selected folder back in the processing queue") };
    QString reprocessFileTip{ tr("Put the source asset back in the processing queue") };

    reprocessAssetAction->setToolTip(sourceAssetTreeItem.getChildCount() ? reprocessFolderTip : reprocessFileTip);
}

void MainWindow::ShowProductAssetContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    auto productAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->ProductAssetsTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_productAssetTreeFilterModel->mapToSource(proxyIndex);
        return static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
    };

    const AssetTreeItem* cachedAsset = productAt(pos);

    if (!cachedAsset)
    {
        return;
    }

    QMenu menu(this);
    menu.setToolTipsVisible(true);
    const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(cachedAsset->GetData());

    QAction* jobAction = menu.addAction("View job", this, [&]()
    {
        if (!productItemData)
        {
            return;
        }

        QModelIndex jobIndex = m_jobsModel->GetJobFromProduct(productItemData->m_databaseInfo, *m_sharedDbConnection);
        SelectJobAndMakeVisible(jobIndex);
    });

    QAction* sourceAssetAction = menu.addAction("View source asset", this, [&]()
    {
        if (!productItemData)
        {
            return;
        }
        m_sharedDbConnection->QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            ui->sourceAssetDetailsPanel->GoToSource(SourceAssetReference(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str()).AbsolutePath().c_str());
            return false; // Don't keep iterating
        });
    });

    if (cachedAsset->getChildCount() > 0)
    {
        sourceAssetAction->setDisabled(true);
        sourceAssetAction->setToolTip(tr("Folders do not have source assets."));
        jobAction->setDisabled(true);
        jobAction->setToolTip(tr("Folders do not have associated jobs."));
    }
    else
    {
        sourceAssetAction->setToolTip(tr("Selects the source asset associated with this product asset."));
        jobAction->setToolTip(tr("Selects the job that created this product asset in the Jobs tab."));
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AZ::Outcome<QString> pathToProduct = GetAbsolutePathToProduct(*cachedAsset);
        if (pathToProduct.IsSuccess())
        {
            AzQtComponents::ShowFileOnDesktop(pathToProduct.GetValue());
        }

    });

    QString fileOrFolder(cachedAsset->getChildCount() > 0 ? tr("folder") : tr("file"));
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view this %1.").arg(fileOrFolder));

    QAction* copyFullPathAction = menu.addAction(tr("Copy full path"), this, [&]()
    {
        AZ::Outcome<QString> pathToProduct = GetAbsolutePathToProduct(*cachedAsset);
        if (pathToProduct.IsSuccess())
        {
            QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(pathToProduct.GetValue()));
        }
    });

    copyFullPathAction->setToolTip(tr("Copies the full path for this %1 to your clipboard.").arg(fileOrFolder));

    QAction* sourceAssetReprocessAction = menu.addAction("Reprocess source asset", this, [&]()
    {
        if (!productItemData )
        {
            return;
        }
        m_sharedDbConnection->QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            m_sharedDbConnection->QueryScanFolderByScanFolderID(sourceEntry.m_scanFolderPK,[&] (AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanfolder)
            {
                QString reprocessSource{ scanfolder.m_scanFolder.c_str() };
                reprocessSource.append("/");
                reprocessSource.append(sourceEntry.m_sourceName.c_str());
                m_guiApplicationManager->GetAssetProcessorManager()->RequestReprocess(reprocessSource);
                return false; // Don't keep iterating
            });
            return false; // Don't keep iterating
        });
    });
    if (cachedAsset->getChildCount() > 0)
    {
        sourceAssetReprocessAction->setDisabled(true);
    }
    sourceAssetReprocessAction->setToolTip(tr("Reprocess the source asset which created this product"));

    menu.exec(ui->ProductAssetsTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::ShowLogLineContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobContextLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobContextLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* key = menu.addAction(tr("Copy selected key"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnKey).data().toString());
    });
    QAction* value = menu.addAction(tr("Copy selected value"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnValue).data().toString());
    });
    menu.addAction(tr("Copy all values"), this, [&]()
    {
        auto model = qobject_cast<ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());
        QGuiApplication::clipboard()->setText(model->toString());
    });

    if (!sourceIndex.isValid())
    {
        key->setEnabled(false);
        value->setEnabled(false);
    }

    menu.exec(ui->jobContextLogTableView->viewport()->mapToGlobal(pos));
}

