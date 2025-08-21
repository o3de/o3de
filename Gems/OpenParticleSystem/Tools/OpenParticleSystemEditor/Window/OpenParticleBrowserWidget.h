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
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QByteArray>
#include <QWidget>
#include <QListWidget>
#include <QFileInfo>
#include <QToolButton>
#include <QPainter>
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
        class EntryDelegate;
    }
}

namespace Ui
{
    class ParticleBrowserWidget;
}

namespace OpenParticleSystemEditor
{
    class NodeBrowserTreeDelegate : public AzToolsFramework::AssetBrowser::EntryDelegate
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeBrowserTreeDelegate, AZ::SystemAllocator, 0);

        explicit NodeBrowserTreeDelegate(QWidget* parent = nullptr);
        ~NodeBrowserTreeDelegate() = default;
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        void SetStringFilter(const QString& filter);

    private:
        QString m_filterString;
        QRegExp m_filterRegex;
        int m_paddingOffset = 7;
    };

    //! Provides a tree view of all available materials and other assets exposed by the ParticleEditor.
    class ParticleBrowserWidget
        : public QWidget
        , protected AZ::TickBus::Handler
    {
        Q_OBJECT
    public:
        ParticleBrowserWidget(QWidget* parent = nullptr);
        ~ParticleBrowserWidget();
    private Q_SLOTS:
        void AddNewSlot(const QPoint& pos);
        void OnTextPathChanged(const QString& text);
        void OnSelectedChanged();
        void OnItemDoubleClicked(QListWidgetItem* item);
        void OnItemRename(QLineEdit* lineEdit, QListWidgetItem *item);
        void ShowParticlePath();
        void CreateFolderSlot();
        void CreateNewParticle();

    protected:
        bool eventFilter(QObject* obj, QEvent* e) override;

    private:
        void InitUI();
        void ConnectSignal();
        void AddNewMenu();
        void HideTreeView();
        bool FindModelIndex(const QModelIndex& idxParent, const QString& key, QModelIndex& idxCurrent);
        void InitParticlesList();
        AzToolsFramework::AssetBrowser::FilterConstType CreateFilter() const;
        bool CheckParticleFileName(const QFileInfo& fileInfo, const QString& text);
        bool CheckName(const QString& name, bool bIsFolder = false);
        QString GetCurrentPath();
        void CreateFolderWindow(const QString& currentPath);
        void CreateParticleWindow(const QString& currentPath);
        void CreateMenu(QMenu* menu);
        void AddListItem(const QString& displayName, const QString& fileName, const QString& fullPath, const QString& iconPath,
                         const QString& relativePath = "");
        void InitIcon();
        void ExpandToKey(const QModelIndex& idxParent, const QString& key);
        void LoadAllParticles();
        void LoadParticleFromItem(const QModelIndex& idxParent);

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        QScopedPointer<Ui::ParticleBrowserWidget> m_ui;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel = nullptr;

        //! if new asset is being created with this path it will automatically be selected
        AZStd::string m_pathToSelect;

        QByteArray m_materialBrowserState;
        QStringList m_particlesPath;
        NodeBrowserTreeDelegate* m_treeDelegate;

        enum MouseState
        {
            MOUSE_LEAVE,
            MOUSE_ENTER
        };
        AZStd::unordered_map<QToolButton*, AZStd::vector<AZStd::string>> m_mapIcon; 
    };
} // namespace OpenParticleSystemEditor
