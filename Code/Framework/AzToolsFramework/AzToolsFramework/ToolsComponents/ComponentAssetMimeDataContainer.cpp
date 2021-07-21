/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "ComponentAssetMimeDataContainer.h"
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
    void ComponentAssetMimeData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ComponentAssetMimeData>()
                ->Field("m_assetId", &ComponentAssetMimeData::m_assetId)
                ->Field("m_classId", &ComponentAssetMimeData::m_classId)
                ->Version(1);
        }
    }

    void ComponentAssetMimeDataContainer::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentAssetMimeData::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ComponentAssetMimeDataContainer>()
                ->Field("m_assets", &ComponentAssetMimeDataContainer::m_assets)
                ->Version(1);
        }
    }

    void ComponentAssetMimeDataContainer::AddComponentAsset(const AZ::Uuid& classId, const AZ::Data::AssetId& assetId)
    {
        ComponentAssetMimeData newAsset(classId, assetId);
        m_assets.push_back(newAsset);
    }

    void ComponentAssetMimeDataContainer::AddToMimeData(QMimeData* mimeData) const
    {
        if (mimeData != nullptr)
        {
            //  There are a few conversion steps.
            AZStd::vector<char> buffer;

            AZ::IO::ByteContainerStream<AZStd::vector<char> > byteStream(&buffer);
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, this);

            QByteArray dataArray(buffer.data(), static_cast<int>(sizeof(char) * buffer.size()));
            mimeData->setData(GetMimeType(), dataArray);
        }
    }

    bool ComponentAssetMimeDataContainer::FromMimeData(const QMimeData* mimeData)
    {
        if (mimeData != nullptr && mimeData->hasFormat(GetMimeType()))
        {
            QByteArray arrayData = mimeData->data(GetMimeType());
            AZ::IO::MemoryStream ms(arrayData.constData(), arrayData.size());

            ComponentAssetMimeDataContainer* pContainer = AZ::Utils::LoadObjectFromStream<ComponentAssetMimeDataContainer>(ms, nullptr);
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
