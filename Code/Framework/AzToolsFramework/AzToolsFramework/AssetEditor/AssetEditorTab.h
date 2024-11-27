/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

#include <QTimer>
#include <QWidget>
#endif

namespace AZ
{
    class SerializeContext;
    namespace DocumentPropertyEditor
    {
        class ReflectionAdapter;
    }
} // namespace AZ

namespace Ui
{
    class AssetEditorToolbar;
    class AssetEditorStatusBar;
    class AssetEditorHeader;
} // namespace Ui

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class DocumentPropertyEditor;
    class FilteredDPE;
    namespace AssetEditor
    {
        class AssetEditorWidget;

        /**
         * Provides ability to create, edit, and save reflected assets.
         */
        class AssetEditorTab
            : public QWidget
            , private AZ::Data::AssetBus::MultiHandler
            , private AzFramework::AssetCatalogEventBus::Handler
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetEditorTab, AZ::SystemAllocator);

            using SaveCompleteCallback = AZStd::function<void()>;

            explicit AssetEditorTab(QWidget* parent = nullptr);
            ~AssetEditorTab() override;

            void LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, const QString& assetName);
            void CreateAsset(AZ::Data::AssetType assetType, const QString& assetName, const AZ::Uuid& observerToken);

            const AZ::Data::AssetId& GetAssetId() const;
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            bool IsDirty() const;
            bool WaitingToSave() const;

            const QString& GetAssetName();

            bool TrySaveAsset(const AZStd::function<void()>& savedCallback);
            bool UserRefusedSave();
            void ClearUserRefusedSaveFlag();
        public Q_SLOTS:

            void SaveAsset();
            bool SaveAsDialog();
            bool SaveAssetToPath(AZStd::string_view assetPath);
            void ExpandAll();
            void CollapseAll();

        private Q_SLOTS:
            // note, intentionally not a reference to a QString - we want to make a copy of the string since this could be a queued thread call
            // Also note, QStrings are reference counted copy-on-write, so this is not as expensive as it seems (++incref cost)
            void ApplyStatusText(QString newStatus);

            // For subscribing to document property editor adapter property specific changes
            void OnDocumentPropertyChanged(const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& changeInfo);
        Q_SIGNALS:
            void OnAssetSavedSignal();
            void OnAssetSaveFailedSignal(const AZStd::string& error);
            void OnAssetRevertedSignal();
            void OnAssetRevertFailedSignal(const AZStd::string& error);
            void OnAssetOpenedSignal(const AZ::Data::Asset<AZ::Data::AssetData>& asset);

        protected: // IPropertyEditorNotify
            void AfterPropertyModified(InstanceDataNode* /*node*/) override;
            void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) override;

            void BeforePropertyModified(InstanceDataNode* node) override;

            void SetPropertyEditingActive(InstanceDataNode* /*node*/) override
            {
            }
            void SetPropertyEditingComplete(InstanceDataNode* /*node*/) override
            {
            }
            void SealUndoStack() override{};

            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

        private:
            void DirtyAsset();

            void SetStatusText(const QString& assetStatus);
            void SetupHeader();

            void SaveAssetImpl(const AZStd::function<void()>& savedCallback);

            AZ::Data::AssetId m_sourceAssetId;
            AZ::Data::Asset<AZ::Data::AssetData> m_inMemoryAsset;
            Ui::AssetEditorHeader* m_header;
            ReflectedPropertyEditor* m_propertyEditor;
            AZStd::shared_ptr<AZ::DocumentPropertyEditor::ReflectionAdapter> m_adapter;
            FilteredDPE* m_filteredWidget = nullptr;
            DocumentPropertyEditor* m_dpe;
            AZ::SerializeContext* m_serializeContext = nullptr;

            // Ids can change when an asset goes from in-memory to saved on disk.
            // If there is a failure, the asset will be removed from the catalog.
            // The only reliable mechanism to be certain the asset being added/removed
            // from the catalog is the same one that was added is to compare its file path.
            AZStd::string m_expectedAddedAssetPath;
            AZStd::string m_recentlyAddedAssetPath;

            bool m_dirty = false;
            bool m_useDPE = false;
            bool m_userRefusedSave = false;

            QString m_currentAsset;

            AssetEditorWidget* m_parentEditorWidget = nullptr;

            AZStd::vector<AZ::u8> m_saveData;
            bool m_closeTabAfterSave = false;
            bool m_closeParentAfterSave = false;
            bool m_waitingToSave = false;

            AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeEvent::Handler m_propertyChangeHandler;
            AZ::Crc32 m_savedStateKey;

            void CreateAssetImpl(AZ::Data::AssetType assetType, const QString& assetName);
            bool SaveImpl(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const QString& saveAsPath);
            void UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset);

            void GenerateSaveDataSnapshot();

            QIcon m_warningIcon;

            AZ::Uuid m_assetObserverToken; // Token that can be used to register for specific request for a new asset to be created via the AssetEditor::CreateAsset function. 

        };
    } // namespace AssetEditor
} // namespace AzToolsFramework
