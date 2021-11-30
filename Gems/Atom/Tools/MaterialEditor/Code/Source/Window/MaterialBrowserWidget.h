/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
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
        class CompositeFilter;
        class AssetBrowserEntry;
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
    }
}

namespace Ui
{
    class MaterialBrowserWidget;
}

namespace MaterialEditor
{
    //! Provides a tree view of all available materials and other assets exposed by the MaterialEditor.
    class MaterialBrowserWidget
        : public QWidget
        , protected AZ::TickBus::Handler
        , protected AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        MaterialBrowserWidget(QWidget* parent = nullptr);
        ~MaterialBrowserWidget();

    private:
        AzToolsFramework::AssetBrowser::FilterConstType CreateFilter() const;
        void OpenSelectedEntries();

        // AtomToolsDocumentNotificationBus::Handler implementation
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void OpenOptionsMenu();

        QScopedPointer<Ui::MaterialBrowserWidget> m_ui;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel = nullptr;

        //! if new asset is being created with this path it will automatically be selected
        AZStd::string m_pathToSelect;

        QByteArray m_materialBrowserState;
    };
} // namespace MaterialEditor
