/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/TickBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Search/SearchWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QByteArray>
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
    }
} // namespace AzToolsFramework

namespace Ui
{
    class AtomToolsAssetBrowser;
}

namespace AtomToolsFramework
{
    //! Extends the standard asset browser with custom filters and multiselect behavior
    class AtomToolsAssetBrowser
        : public QWidget
        , protected AZ::SystemTickBus::Handler
    {
        Q_OBJECT
    public:
        AtomToolsAssetBrowser(QWidget* parent = nullptr);
        ~AtomToolsAssetBrowser();

        //! @return the asset browser top level search widget 
        AzToolsFramework::AssetBrowser::SearchWidget* GetSearchWidget();

        //! FileTypeFilter contains a set of extensions used to compare and filter file paths add a flag that tells if the filter is
        //! enabled.
        struct FileTypeFilter
        {
            AZStd::string m_name;
            AZStd::unordered_set<AZStd::string> m_extensions;
            bool m_enabled = false;
        };

        using FileTypeFilterVec = AZStd::vector<FileTypeFilter>;

        //! Set up ext based file filters for the asset browser. The filters can be set using the options menu on the search bar. This is
        //! intended to be used instead of asset type filters because some tools primarily work with source files that do not have
        //! registered asset type info.
        void SetFileTypeFilters(const FileTypeFilterVec& fileTypeFilters);

        //! Register a custom handler for opening different file types.
        void SetOpenHandler(AZStd::function<void(const AZStd::string&)> openHandler);

        //! Select the asset browser entries matching the absolute path.
        void SelectEntries(const AZStd::string& absolutePath);

        //! Open the currently selected asset browser entries using the registered file openers.
        void OpenSelectedEntries();

    protected:
        AzToolsFramework::AssetBrowser::FilterConstType CreateFilter() const;
        void UpdateFilter();
        void UpdatePreview();
        void TogglePreview();

        void InitOptionsMenu();
        void InitSettingsHandler();
        void InitSettings();
        void SaveSettings();

        void UpdateFileTypeFilters();

        // AZ::SystemTickBus::Handler
        void OnSystemTick() override;

        QScopedPointer<Ui::AtomToolsAssetBrowser> m_ui;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel = nullptr;

        //! If an asset is opened with this path it will automatically be selected
        AZStd::string m_pathToSelect;

        QByteArray m_browserState;

        AZStd::function<void(const AZStd::string&)> m_openHandler;

        AZ::SettingsRegistryInterface::NotifyEventHandler m_settingsNotifyEventHandler;

        FileTypeFilterVec m_fileTypeFilters;

        bool m_fileTypeFiltersEnabled = false;

        bool m_showEmptyFolders = false;

        QMenu* m_optionsMenu = nullptr;
    };
} // namespace AtomToolsFramework
