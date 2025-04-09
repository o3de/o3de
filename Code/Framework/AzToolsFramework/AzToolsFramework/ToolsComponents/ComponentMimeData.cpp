/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentMimeData.h"
#include <QMimeData>
#include <QDataStream>
#include <QClipboard>
#include <QApplication>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    QString ComponentTypeMimeData::GetMimeType()
    {
        return "application/x-amazon-o3de-editorcomponenttypes";
    }

    AZStd::unique_ptr<QMimeData> ComponentTypeMimeData::Create(const ClassDataContainer& container)
    {
        QByteArray dataArray;
        AZStd::unique_ptr<QMimeData> mimeData = std::make_unique<QMimeData>();
        QDataStream stream(&dataArray, QIODevice::WriteOnly);

        // Count how many valid items we have so we don't stream out nullptrs
        quint64 numItems = 0;
        for (auto item : container)
        {
            AZ_Assert(item, "null class type provided as input to ComponentMimeData::Create");
            if (item)
            {
                ++numItems;
            }
        }
        
        stream << numItems;
        for (auto item : container)
        {
            if (!item)
            {
                continue;
            }
            quint64 classDataPtr = reinterpret_cast<quint64>(item);
            stream << classDataPtr;
        }

        mimeData->setData(GetMimeType(), dataArray);

        return mimeData;
    }

    bool ComponentTypeMimeData::Get(const QMimeData* mimeData, ClassDataContainer& container)
    {
        if (mimeData && mimeData->hasFormat(GetMimeType()))
        {
            container.clear();

            QByteArray arrayData = mimeData->data(GetMimeType());

            QDataStream stream(&arrayData, QIODevice::ReadOnly);

            quint64 numItems = 0;
            stream >> numItems;

            for (size_t i = 0; i < numItems; ++i)
            {
                quint64 ptrAddr;
                stream >> ptrAddr;
                auto classData = reinterpret_cast<ClassDataType>(ptrAddr);
                container.push_back(classData);
            }

            return !container.empty();
        }
        
        return false;
    }

    void ComponentMimeData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ComponentMimeData>()
                ->Field("Components", &ComponentMimeData::m_components);
        }
    }

    QString ComponentMimeData::GetMimeType()
    {
        return "application/x-amazon-o3de-editorcomponentdata";
    }

    AZStd::unique_ptr<QMimeData> ComponentMimeData::Create(AZStd::span<AZ::Component* const> components)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Assert(context, "No serialize context");
            return nullptr;
        }
        // Extract the type data, put it in the mime data for early checking later
        ComponentTypeMimeData::ClassDataContainer componentClassTypes;
        for (AZ::Component* component : components)
        {
            AZ_Assert(component, "null component provided as input to ComponentMimeData::Create");
            if (!component)
            {
                continue;
            }
            // Get the underlying component type if wrapped with a generic component wrapper
            auto classData = context->FindClassData(GetComponentTypeId(component));
            if (classData)
            {
                componentClassTypes.push_back(classData);
            }
        }

        // Pack the components into a helper class for serialization
        ComponentMimeData componentMimeData;
        componentMimeData.m_components.assign(components.begin(), components.end());

        // Save the helper class into a buffer to pack into mime data
        AZStd::vector<char> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> byteStream(&buffer);
        if (!AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_XML, &componentMimeData))
        {
            return nullptr;
        }

        QByteArray dataArray(buffer.data(), static_cast<int>(sizeof(char) * buffer.size()));

        AZStd::unique_ptr<QMimeData> mimeData = ComponentTypeMimeData::Create(componentClassTypes);
        if (mimeData)
        {
            mimeData->setData(GetMimeType(), dataArray);
        }
        return mimeData;
    }

    void ComponentMimeData::GetComponentDataFromMimeData(const QMimeData* mimeData, ComponentDataContainer& componentData)
    {
        if (!mimeData || !mimeData->hasFormat(GetMimeType()))
        {
            return;
        }

        QByteArray arrayData = mimeData->data(GetMimeType());
        AZ::IO::MemoryStream memoryStream(arrayData.constData(), arrayData.size());

        ComponentMimeData* componentMimeData = AZ::Utils::LoadObjectFromStream<ComponentMimeData>(memoryStream);
        if (!componentMimeData)
        {
            return;
        }

        componentData.clear();
        componentData = AZStd::move(componentMimeData->m_components);
        delete componentMimeData;
    }

    const QMimeData* ComponentMimeData::GetComponentMimeDataFromClipboard()
    {
        // Do we have stuff on the clipboard?
        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();

        if (mimeData && mimeData->hasFormat(GetMimeType()))
        {
            return mimeData;
        }
        return nullptr;
    }

    void ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::unique_ptr<QMimeData> mimeData)
    {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mimeData.release());
    }
}
