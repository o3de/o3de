/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/Network/SocketConnection.h>

#include <AzQtComponents/Components/StyledBusyLabel.h>

namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
    }

    namespace AssetBrowser
    {
        class AssetBrowserModel;
        class SourceAssetBrowserEntry;
        class FolderAssetBrowserEntry;
        class RootAssetBrowserEntry;
        class AssetEntryChangeset;
        class AssetBrowserEntityInspectorWidget;

        //! AssetBrowserComponent caches database entries
        /*!
            Database entries are cached so that they can be quickly accessed by asset browser views.
            Additionally this class watches for any changes to the database and updates the views if such changes happen
        */
        class AssetBrowserComponent
            : public AZ::Component
            , public AssetBrowserComponentRequestBus::Handler
            , public AssetDatabaseLocationNotificationBus::Handler
            , public AzFramework::AssetCatalogEventBus::Handler
            , public AZ::SystemTickBus::Handler
            , public AssetSystemBus::Handler
            , public AssetBrowserInteractionNotificationBus::Handler
            , private AssetBrowserFileCreationNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(AssetBrowserComponent, "{4BC5F93F-2F9E-412E-B00A-396C68CFB5FB}")

            AssetBrowserComponent();
            virtual ~AssetBrowserComponent();

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            //////////////////////////////////////////////////////////////////////////
            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            //////////////////////////////////////////////////////////////////////////
            // AssetDatabaseLocationNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void OnDatabaseInitialized() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentRequestBus
            //////////////////////////////////////////////////////////////////////////
            AssetBrowserModel* GetAssetBrowserModel() override;
            bool AreEntriesReady() override;
            void PickAssets(AssetSelectionModel& selection, QWidget* parent) override;
            AzQtComponents::StyledBusyLabel* GetStyledBusyLabel() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetCatalogEventBus
            //////////////////////////////////////////////////////////////////////////
            void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
            void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;

            //////////////////////////////////////////////////////////////////////////
            // SystemTickBus
            //////////////////////////////////////////////////////////////////////////
            void OnSystemTick() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetSystemBus
            //////////////////////////////////////////////////////////////////////////
            void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUuid) override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserInteractionNotificationBus
            SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
            //////////////////////////////////////////////////////////////////////////

            void AddFile(const AZ::s64& fileId);
            void RemoveFile(const AZ::s64& fileId);

            void PopulateAssets();
            void UpdateAssets();

        private:
            AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> m_databaseConnection;
            AZStd::shared_ptr<RootAssetBrowserEntry> m_rootEntry;
            AZStd::binary_semaphore m_updateWait;
            AZStd::thread m_thread;

            //! wait until database is ready
            bool m_dbReady;
            //! have entries been populated yet
            bool m_entriesReady = false;
            //! is query waiting for more update requests
            AZStd::atomic_bool m_waitingForMore;
            //! should the query thread stop
            AZStd::atomic_bool m_disposed;
            AZStd::atomic_bool m_isResetting;
            AZStd::atomic_bool m_changesApplied;

            AZStd::unique_ptr<AssetBrowserModel> m_assetBrowserModel;
            AZStd::shared_ptr<AssetEntryChangeset> m_changeset;

            AzFramework::SocketConnection::TMessageCallbackHandle m_cbHandle = 0;

            AzQtComponents::StyledBusyLabel* m_styledBusyLabel;

            //! Notify to start the query thread
            void NotifyUpdateThread();

            void HandleFileInfoNotification(const void* buffer, unsigned int bufferSize);

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserFileCreationNotificationBus
            void HandleAssetCreatedInEditor(const AZStd::string_view assetPath, const AZ::Crc32& creatorBusId /*= AZ::Crc32()*/, const bool initialFilenameChange) override;
            //////////////////////////////////////////////////////////////////////////
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
