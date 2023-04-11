/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>
#include "native/utilities/LogPanel.h"
#include <QPointer>
#include "native/assetprocessor.h"
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <QElapsedTimer>
#include <ui/BuilderListModel.h>
#include <native/utilities/AssetUtilEBusHelper.h>
#include <native/utilities/PlatformConfiguration.h>
#include <native/ui/CacheServerData.h>
#endif

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
    }
}

namespace Ui {
    class MainWindow;
}
class GUIApplicationManager;
class QListWidgetItem;
class QFileSystemWatcher;
class QSettings;

namespace AssetProcessor
{
    class AssetTreeFilterModel;
    class JobSortFilterProxyModel;
    class JobsModel;
    class AssetTreeItem;
    class ProductAssetTreeModel;
    class SourceAssetTreeModel;
    class ProductDependencyTreeItem;
    class JobEntry;
    class BuilderData;
    class BuilderInfoPatternsModel;
    class BuilderInfoMetricsModel;
    class BuilderInfoMetricsSortModel;
    class EnabledRelocationTypesModel;
}

class MainWindow
    : public QMainWindow
{
    Q_OBJECT

public:

    // Tracks which asset tab the asset page is on.
    enum class AssetTabIndex
    {
        Source = 0,
        Intermediate = 1,
        Product = 2
    };

    // This order is actually driven by the layout in the UI file.
    // If the order is changed in the UI file, it should be changed here, too.
    enum class DialogStackIndex
    {
        Welcome, // The welcome screen provides some basic info on the Asset Processor.
        Jobs,
        Assets,
        Logs,
        Connections,
        Builders,
        Tools,
        AssetRelocation
    };

    struct Config
    {
        // These default values are used if the values can't be read from AssetProcessorConfig.ini,
        // and the call to defaultConfig fails.

        // Asset Status
        int jobStatusColumnWidth = -1;
        int jobSourceColumnWidth = -1;
        int jobPlatformColumnWidth = -1;
        int jobKeyColumnWidth = -1;
        int jobCompletedColumnWidth = -1;

        // Event Log Details
        int logTypeColumnWidth = -1;

        // Event Log Line Details
        int contextDetailsTableMaximumRows = -1;
    };

    /*!
     * Loads the button config data from a settings object.
     */
    static Config loadConfig(QSettings& settings);

    /*!
     * Returns default button config data.
     */
    static Config defaultConfig();

    explicit MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent = 0);
    void Activate();
    ~MainWindow() override;

public Q_SLOTS:
    void ShowWindow();

    void SyncAllowedListAndRejectedList(QStringList allowedList, QStringList rejectedList);
    void FirstTimeAddedToRejectedList(QString ipAddress);
    void SaveLogPanelState();
    void OnAssetProcessorStatusChanged(const AssetProcessor::AssetProcessorStatusEntry entry);

    void OnRescanButtonClicked();
    void HighlightAsset(QString assetPath);

    void OnAssetTabChange(int index);
    void BuilderTabSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

protected Q_SLOTS:
    void ApplyConfig();
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
private:

    class LogSortFilterProxy : public QSortFilterProxyModel
    {
    public:
        LogSortFilterProxy(QObject* parentOjbect);
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
        void onTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters);

    private:
        QSet<AzToolsFramework::Logging::LogLine::LogType> m_logTypes;
    };

    Ui::MainWindow* ui;
    GUIApplicationManager* m_guiApplicationManager;
    AzToolsFramework::Logging::LogTableModel* m_logsModel;
    AssetProcessor::JobSortFilterProxyModel* m_jobSortFilterProxy;
    LogSortFilterProxy* m_logSortFilterProxy;
    AssetProcessor::JobsModel* m_jobsModel;
    AssetProcessor::SourceAssetTreeModel* m_sourceModel = nullptr;
    AssetProcessor::SourceAssetTreeModel* m_intermediateModel = nullptr;
    AssetProcessor::ProductAssetTreeModel* m_productModel = nullptr;
    AssetProcessor::AssetTreeFilterModel* m_sourceAssetTreeFilterModel = nullptr;
    AssetProcessor::AssetTreeFilterModel* m_intermediateAssetTreeFilterModel = nullptr;
    AssetProcessor::AssetTreeFilterModel* m_productAssetTreeFilterModel = nullptr;
    QPointer<AssetProcessor::LogPanel> m_loggingPanel;
    int m_processJobsCount = 0;
    int m_createJobCount = 0;
    QFileSystemWatcher* m_fileSystemWatcher;
    Config m_config;

    AssetProcessor::BuilderData* m_builderData = nullptr;
    BuilderListModel* m_builderList = nullptr;
    BuilderListSortFilterProxy* m_builderListSortFilterProxy = nullptr;
    AssetProcessor::BuilderInfoPatternsModel* m_builderInfoPatterns = nullptr;
    AssetProcessor::BuilderInfoMetricsModel* m_builderInfoMetrics = nullptr;
    AssetProcessor::BuilderInfoMetricsSortModel* m_builderInfoMetricsSort = nullptr;
    AssetProcessor::CacheServerData m_cacheServerData;

    AssetProcessor::EnabledRelocationTypesModel* m_enabledRelocationTypesModel = nullptr;

    void SetContextLogDetails(const QMap<QString, QString>& details);
    void ClearContextLogDetails();

    void EditConnection(const QModelIndex& index);
    void OnConnectionContextMenu(const QPoint& point);
    void OnEditConnection(bool checked);
    void OnAddConnection(bool checked);
    void OnRemoveConnection(bool checked);
    void OnSupportClicked(bool checked);
    void OnConnectionSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    QStringListModel m_rejectedAddresses;
    QStringListModel m_allowedListAddresses;

    void OnAllowedListConnectionsListViewClicked();
    void OnRejectedConnectionsListViewClicked();

    void OnAllowedListCheckBoxToggled();

    void OnAddHostNameAllowedListButtonClicked();
    void OnAddIPAllowedListButtonClicked();

    void OnToAllowedListButtonClicked();
    void OnToRejectedListButtonClicked();

    void UpdateJobLogView(QModelIndex selectedIndex);
    void JobSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void JobStatusChanged(AssetProcessor::JobEntry entry, AzToolsFramework::AssetSystem::JobStatus status);
    void JobLogSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void DesktopOpenJobLogs();
    // Switches to the Job tab of the Asset Processor, clears any current searches, scroll to, and select the job at the given index.
    void SelectJobAndMakeVisible(const QModelIndex& index);

    void ResetLoggingPanel();
    void ShowJobViewContextMenu(const QPoint& pos);
    void ShowLogLineContextMenu(const QPoint& pos);
    void ShowJobLogContextMenu(const QPoint& pos);

    void ShowProductAssetContextMenu(const QPoint& pos);
    void ShowSourceAssetContextMenu(const QPoint& pos);
    void ShowIntermediateAssetContextMenu(const QPoint& pos);

    void BuildSourceAssetTreeContextMenu(QMenu& menu, const AssetProcessor::AssetTreeItem& sourceAssetTreeItem);

    // Helper function that retrieves the item selected in outgoing/incoming dependency TreeView
    AssetProcessor::ProductDependencyTreeItem* GetProductAssetFromDependencyTreeView(bool isOutgoing, const QPoint& pos);
    void ShowOutgoingProductDependenciesContextMenu(const QPoint& pos);
    void ShowIncomingProductDependenciesContextMenu(const QPoint& pos);

    void ResetTimers();
    void CheckStartAnalysisTimers();
    void CheckEndAnalysisTimer();
    void CheckStartProcessTimers();
    void CheckEndProcessTimer();
    QString FormatStringTime(qint64 timeMs) const;

    /// Refreshes the filter in the Asset Tab at a set time interval.
    /// TreeView filters can be expensive to refresh every time an item is added, so refreshing on a set schedule
    /// keeps the view up-to-date without causing a performance bottleneck.
    void IntervalAssetTabFilterRefresh();
    /// Fires off one final refresh before invalidating the filter refresh timer.
    void ShutdownAssetTabFilterRefresh();

    void SetupAssetServerTab();
    void AddPatternRow(AZStd::string_view name, AssetBuilderSDK::AssetBuilderPattern::PatternType type, AZStd::string_view pattern, bool enable);
    void AssembleAssetPatterns();
    void CheckAssetServerStates();
    void ResetAssetServerView();
    void SetServerAddress(AZStd::string_view serverAddress);

    void SetupAssetSelectionCaching();

    QElapsedTimer m_scanTimer;
    QElapsedTimer m_analysisTimer;
    QElapsedTimer m_processTimer;
    QElapsedTimer m_filterRefreshTimer;

    qint64 m_scanTime{ 0 };
    qint64 m_analysisTime{ 0 };
    qint64 m_processTime{ 0 };

    AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_sharedDbConnection;

    AssetProcessor::SourceAndScanID m_cachedSourceAssetSelection;
    AZStd::string m_cachedProductAssetSelection;
    QMetaObject::Connection m_connectionForResettingAssetsView;
};

