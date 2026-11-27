/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasFileObject.h"
#include "UiPrefabInstance.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Utils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasFileObject::~UiCanvasFileObject()
{
    for (AZ::Entity* entity : m_childEntities)
    {
        delete entity;
    }
    m_childEntities.clear();
    delete m_canvasEntity;
    m_canvasEntity = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasFileObject* UiCanvasFileObject::LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc)
{
    UiCanvasFileObject* fileObject =
        AZ::Utils::LoadObjectFromStream<UiCanvasFileObject>(stream, nullptr, filterDesc);
    return fileObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasFileObject::SaveCanvasToStream(AZ::IO::GenericStream& stream, UiCanvasFileObject* canvasFileObject)
{
    AZ::Utils::SaveObjectToStream<UiCanvasFileObject>(stream, AZ::DataStream::ST_XML, canvasFileObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasFileObject::LoadCanvasEntitiesFromStream(
    AZ::IO::GenericStream& stream,
    AZStd::vector<AZ::Entity*>& childEntities,
    AZStd::vector<UiPrefabInstance>& prefabInstances)
{
    AZ::Entity* canvasEntity = nullptr;

    UiCanvasFileObject* fileObject = AZ::Utils::LoadObjectFromStream<UiCanvasFileObject>(stream);
    if (fileObject && fileObject->m_canvasEntity)
    {
        canvasEntity = fileObject->m_canvasEntity;
        childEntities = AZStd::move(fileObject->m_childEntities);
        prefabInstances = AZStd::move(fileObject->m_prefabInstances);

        // Prevent the file object destructor from deleting these
        fileObject->m_canvasEntity = nullptr;
        fileObject->m_childEntities.clear();
    }

    delete fileObject;

    return canvasEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasFileObject::Reflect(AZ::ReflectContext* context)
{
    UiPrefabInstance::Reflect(context);

    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCanvasFileObject>()
            ->Version(4)
            ->Field("CanvasEntity", &UiCanvasFileObject::m_canvasEntity)
            ->Field("ChildEntities", &UiCanvasFileObject::m_childEntities)
            ->Field("PrefabInstances", &UiCanvasFileObject::m_prefabInstances);
    }
}
