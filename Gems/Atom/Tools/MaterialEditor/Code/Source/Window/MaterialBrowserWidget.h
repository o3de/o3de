/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <Atom/Document/MaterialDocumentNotificationBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
#include <QByteArray>
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
        , public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
        , public MaterialDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        MaterialBrowserWidget(QWidget* parent = nullptr);
        ~MaterialBrowserWidget();

    private:
        AzToolsFramework::AssetBrowser::FilterConstType CreateFilter() const;
        void OpenSelectedEntries();

        QScopedPointer<Ui::MaterialBrowserWidget> m_ui;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel = nullptr;

        //! if new asset is being created with this path it will automatically be selected
        AZStd::string m_pathToSelect;

        // AssetBrowserModelNotificationBus::Handler implementation
        void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;

        // MaterialDocumentNotificationBus::Handler implementation
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        void OpenOptionsMenu();

        QByteArray m_materialBrowserState;
    };
} // namespace MaterialEditor
