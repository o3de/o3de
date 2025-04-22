/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        /**
        * AssetEntryChange represents an atomic change performed on AssetBrowser model
        */
        class AssetEntryChange
        {
        public:
            AZ_RTTI(AssetEntryChange, "{C8D33C55-1AB9-4599-B1DD-32403E117813}");

            AssetEntryChange() = default;
            virtual ~AssetEntryChange() = default;
            virtual bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) = 0;
        };

        class AddScanFolderChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(AddScanFolderChange, "{CA1C5AC8-127B-4422-9431-030209A07614}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(AddScanFolderChange, AZ::SystemAllocator);

            AddScanFolderChange(const AssetDatabase::ScanFolderDatabaseEntry& scanFolder)
                : m_scanFolder(scanFolder) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                rootEntry->AddScanFolder(m_scanFolder);
                return true;
            }
        private:
            AssetDatabase::ScanFolderDatabaseEntry m_scanFolder;
        };

        class AddFileChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(AddFileChange, "{65D8CFBB-4CD9-4E4F-9A08-0F4D25CA1326}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(AddFileChange, AZ::SystemAllocator);

            AddFileChange(const AssetDatabase::FileDatabaseEntry& file)
                : m_file(file) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                rootEntry->AddFile(m_file);
                return true;
            }
        private:
            AssetDatabase::FileDatabaseEntry m_file;
        };

        class RemoveFileChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(RemoveFileChange, "{83758FA6-65F2-493E-8D27-C73D0AF0FB94}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(RemoveFileChange, AZ::SystemAllocator);

            RemoveFileChange(const AZ::s64& fileId)
                : m_fileId(fileId) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                return rootEntry->RemoveFile(m_fileId);
            }
        private:
            AZ::s64 m_fileId;
        };

        class AddSourceChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(AddSourceChange, "{E9BE4E9B-85DE-4217-96F2-4098A88F1FDE}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(AddSourceChange, AZ::SystemAllocator);

            AddSourceChange(const SourceWithFileID& source)
                : m_source(source) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                return rootEntry->AddSource(m_source);
            }
        private:
            SourceWithFileID m_source;
        };

        class RemoveSourceChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(RemoveSourceChange, "{D87AB1B8-6F72-4E2F-B7C1-A01BD62D8F1D}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(RemoveSourceChange, AZ::SystemAllocator);

            RemoveSourceChange(const AZ::Uuid& sourceUuid)
                : m_sourceUuid(sourceUuid) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                rootEntry->RemoveSource(m_sourceUuid);
                return true;
            }
        private:
            AZ::Uuid m_sourceUuid;
        };

        class AddProductChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(AddProductChange, "{7127E694-4CD4-480F-A88E-B5C726B65DFE}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(AddProductChange, AZ::SystemAllocator);

            AddProductChange(const ProductWithUuid& product)
                : m_product(product) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                return rootEntry->AddProduct(m_product);
            }

            AZ::Uuid GetUuid() const { return m_product.first; }
            const AZ::Data::AssetId GetAssetId() const { return AZ::Data::AssetId(GetUuid(), m_product.second.m_subID); }
        private:
            ProductWithUuid m_product;
        };

        class RemoveProductChange
            : public AssetEntryChange
        {
        public:
            AZ_RTTI(RemoveProductChange, "{7DDC4900-8842-4221-8CB7-C167DEB82BE8}", AssetEntryChange);
            AZ_CLASS_ALLOCATOR(RemoveProductChange, AZ::SystemAllocator);

            RemoveProductChange(const AZ::Data::AssetId& assetId)
                : m_assetId(assetId) {}

            bool Apply(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry) override
            {
                rootEntry->RemoveProduct(m_assetId);
                return true;
            }
        private:
            AZ::Data::AssetId m_assetId;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
