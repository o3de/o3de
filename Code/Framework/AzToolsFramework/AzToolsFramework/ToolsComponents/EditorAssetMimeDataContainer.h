/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef EDITOR_ASSET_ID_CONTAINER_H
#define EDITOR_ASSET_ID_CONTAINER_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

#include <QtCore/QString>

namespace AZ
{
    struct ClassDataReflection;
    class ReflectContext;
}

class QMimeData;

namespace AzToolsFramework
{
    class EditorAssetMimeData
    {
    public:
        virtual ~EditorAssetMimeData() { }

        AZ_RTTI(EditorAssetMimeData, "{844742CD-7D34-4ED0-B798-396A6C0530BF}");
        AZ_CLASS_ALLOCATOR(EditorAssetMimeData, AZ::SystemAllocator);

        EditorAssetMimeData()
        {
        }

        EditorAssetMimeData(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType)
            : m_assetId(assetId)
            , m_assetType(assetType)
        {
        }

        AZ::Data::AssetId   m_assetId;
        AZ::Data::AssetType m_assetType;

        static void Reflect(AZ::ReflectContext* context);
    };

    /// Mime data for copying assets into property fields via drag/drop.
    /// The type is used for validation before accepting drops.
    class EditorAssetMimeDataContainer
    {
    public:
        virtual ~EditorAssetMimeDataContainer() { }

        AZ_RTTI(EditorAssetMimeDataContainer, "{BC72D334-EFF9-40F0-B615-48186E01BDD6}");
        AZ_CLASS_ALLOCATOR(EditorAssetMimeDataContainer, AZ::SystemAllocator);

        AZStd::vector< EditorAssetMimeData > m_assets;

        /// Create a new EditorAssetMimeData and add it to the internal vector.
        void AddEditorAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType);

        /// Add mime data of this type to the specified QMimeData.
        void AddToMimeData(QMimeData* mimeData) const;
        /// Retrieve mime data of this type from the specified QMimeData. Return true if successful.
        bool FromMimeData(const QMimeData* mimeData);

        // utility functions to serialize/deserialize.
        bool ToBuffer(AZStd::vector<char>& buffer);
        bool FromBuffer(const AZStd::vector<char>& buffer);
        bool FromBuffer(const char* data, AZStd::size_t size);

        static void Reflect(AZ::ReflectContext* context);

        static QString GetMimeType() { return "editor/assetinformation"; }
    };
}

#endif // EDITOR_ASSET_ID_CONTAINER_H
