/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "EditorAssetMimeDataContainer.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <QtCore/QMimeData>

namespace AzToolsFramework
{
    void EditorAssetMimeData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAssetMimeData>()
                ->Field("m_assetId", &EditorAssetMimeData::m_assetId)
                ->Field("m_assetType", &EditorAssetMimeData::m_assetType)
                ->Version(1);
        }
    }

    void EditorAssetMimeDataContainer::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::EditorAssetMimeData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorAssetMimeDataContainer>()
                ->Field("m_assets", &EditorAssetMimeDataContainer::m_assets)
                ->Version(2);
        }
    }

    bool EditorAssetMimeDataContainer::ToBuffer(AZStd::vector<char>& buffer)
    {
        buffer.clear();
        AZ::IO::ByteContainerStream<AZStd::vector<char> > ms(&buffer);
        return AZ::Utils::SaveObjectToStream(ms, AZ::DataStream::ST_BINARY, this);
    }

    bool EditorAssetMimeDataContainer::FromBuffer(const char* data, AZStd::size_t size)
    {
        AZ::IO::MemoryStream ms(data, size);

        EditorAssetMimeDataContainer* pContainer = AZ::Utils::LoadObjectFromStream<EditorAssetMimeDataContainer>(ms, nullptr);
        if (pContainer)
        {
            m_assets = AZStd::move(pContainer->m_assets);
            delete pContainer;
            return true;
        }
        return false;
    }

    bool EditorAssetMimeDataContainer::FromBuffer(const AZStd::vector<char>& buffer)
    {
        return FromBuffer(buffer.data(), buffer.size());
    }

    void EditorAssetMimeDataContainer::AddEditorAsset(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType)
    {
        EditorAssetMimeData newAsset(assetId, assetType);
        m_assets.push_back(newAsset);
    }

    void EditorAssetMimeDataContainer::AddToMimeData(QMimeData* mimeData) const
    {
        if (mimeData != nullptr)
        {
            AZStd::vector<char> buffer;

            AZ::IO::ByteContainerStream<AZStd::vector<char> > byteStream(&buffer);
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, this);

            QByteArray dataArray(buffer.data(), static_cast<int>(sizeof(char) * buffer.size()));
            mimeData->setData(GetMimeType(), dataArray);
        }
    }

    bool EditorAssetMimeDataContainer::FromMimeData(const QMimeData* mimeData)
    {
        if (mimeData != nullptr && mimeData->hasFormat(GetMimeType()))
        {
            QByteArray arrayData = mimeData->data(GetMimeType());
            AZ::IO::MemoryStream ms(arrayData.constData(), arrayData.size());

            EditorAssetMimeDataContainer* pContainer = AZ::Utils::LoadObjectFromStream<EditorAssetMimeDataContainer>(ms, nullptr);
            if (pContainer)
            {
                m_assets = AZStd::move(pContainer->m_assets);
                delete pContainer;
                return true;
            }
        }
        return false;
    }
}
