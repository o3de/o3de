/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef COMPONENT_ASSET_MIME_DATA_CONTAINER_H
#define COMPONENT_ASSET_MIME_DATA_CONTAINER_H

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
    class ComponentAssetMimeData
    {
    public:
        virtual ~ComponentAssetMimeData() { }

        AZ_RTTI(ComponentAssetMimeData, "{39C0AF31-9CC2-4E98-0E27-8025D15F4DF5}");
        AZ_CLASS_ALLOCATOR(ComponentAssetMimeData, AZ::SystemAllocator);

        ComponentAssetMimeData()
        {
            m_classId = AZ::Uuid::CreateNull();
        }

        ComponentAssetMimeData(AZ::Uuid classId, AZ::Data::AssetId assetId)
            : m_assetId(assetId)
            , m_classId(classId)
        {
        }

        AZ::Data::AssetId   m_assetId;
        AZ::Uuid            m_classId;

        static void Reflect(AZ::ReflectContext* context);
    };

    /// A mime container used for an asset and a component type to assign that asset into.
    /// Used for creating new components directly from assets.
    class ComponentAssetMimeDataContainer
    {
    public:
        virtual ~ComponentAssetMimeDataContainer() { }

        AZ_RTTI(ComponentAssetMimeDataContainer, "{7744B99F-2FE9-49AF-DEB2-1562BDA04238}");
        AZ_CLASS_ALLOCATOR(ComponentAssetMimeDataContainer, AZ::SystemAllocator);

        AZStd::vector< ComponentAssetMimeData > m_assets;

        /// Create a new ComponentAssetMimeData and add it to the internal vector.
        void AddComponentAsset(const AZ::Uuid& classId, const AZ::Data::AssetId& assetId);

        /// Add mime data of this type to the specified QMimeData.
        void AddToMimeData(QMimeData* mimeData) const;
        /// Retrieve mime data of this type from the specified QMimeData. Return true if successful.
        bool FromMimeData(const QMimeData* mimeData);

        static void Reflect(AZ::ReflectContext* context);

        static QString GetMimeType() { return "editor/componentasset"; }
    };
}

#endif // EDITOR_ASSET_ID_CONTAINER_H
