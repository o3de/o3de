/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzToolsFramework_precompiled.h"
#include "EditorEntityIdContainer.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>

#include <QString>

namespace AzToolsFramework
{
    const QString& EditorEntityIdContainer::GetMimeType()
    {
        static QString mimeType = QStringLiteral("editor/entityidlist");
        return mimeType;
    }

    void EditorEntityIdContainer::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorEntityIdContainer>()
                ->Field("m_entityIds", &EditorEntityIdContainer::m_entityIds)
                ->Version(1);
        }
    }

    bool EditorEntityIdContainer::ToBuffer(AZStd::vector<char>& buffer)
    {
        buffer.clear();
        AZ::IO::ByteContainerStream<AZStd::vector<char> > ms(&buffer);
        return AZ::Utils::SaveObjectToStream(ms, AZ::DataStream::ST_BINARY, this);
    }

    bool EditorEntityIdContainer::FromBuffer(const char* data, AZStd::size_t size)
    {
        AZ::IO::MemoryStream ms(data, size);

        EditorEntityIdContainer* pContainer = AZ::Utils::LoadObjectFromStream<EditorEntityIdContainer>(ms, nullptr);
        if (pContainer)
        {
            m_entityIds = AZStd::move(pContainer->m_entityIds);
            delete pContainer;
            return true;
        }

        return false;
    }

    bool EditorEntityIdContainer::FromBuffer(const AZStd::vector<char>& buffer)
    {
        return FromBuffer(buffer.data(), buffer.size());
    }
}
