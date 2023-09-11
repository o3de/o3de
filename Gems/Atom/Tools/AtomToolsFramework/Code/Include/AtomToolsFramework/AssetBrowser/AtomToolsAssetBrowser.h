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
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

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

        void SetFilterState(const AZStd::string& category, const AZStd::string& displayName, bool enabled);
        void SetOpenHandler(AZStd::function<void(const AZStd::string&)> openHandler);

        void SelectEntries(const AZStd::string& absolutePath);
        void OpenSelectedEntries();

    protected:
        AzToolsFramework::AssetBrowser::FilterConstType CreateFilter() const;
        void UpdateFilter();
        void UpdatePreview();
        void TogglePreview();

        // AZ::SystemTickBus::Handler
        void OnSystemTick() override;

        QScopedPointer<Ui::AtomToolsAssetBrowser> m_ui;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel = nullptr;

        //! If an asset is opened with this path it will automatically be selected
        AZStd::string m_pathToSelect;

        QByteArray m_browserState;

        AZStd::function<void(const AZStd::string&)> m_openHandler;

        AZ::SettingsRegistryInterface::NotifyEventHandler m_settingsNotifyEventHandler;
    };
} // namespace AtomToolsFramework
