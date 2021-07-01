/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QMimeData>
#include <QByteArray>
AZ_POP_DISABLE_WARNING

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>

namespace GraphCanvas
{
    class QtMimeUtils
    {
    public:
    
        template<typename Type>
        static void WriteTypeToMimeData(QMimeData* mimeData, const char* dataName, const Type& dataType)
        {
            AZStd::vector<char> azEncoded;            
            AZ::IO::ByteContainerStream<AZStd::vector<char>> ms(&azEncoded);
            
            if (AZ::Utils::SaveObjectToStream(ms, AZ::DataStream::ST_BINARY, &dataType))
            {
                QByteArray qtEncoded;
                qtEncoded.resize((int)azEncoded.size());
                memcpy(qtEncoded.data(), azEncoded.data(), azEncoded.size());
                mimeData->setData(dataName, qtEncoded);
            }
        }
    
        template<typename Type>
        static Type ExtractTypeFromMimeData(const QMimeData* mimeData, const char* dataName)
        {
            QByteArray encodedData = mimeData->data(dataName);

            AZ::IO::MemoryStream ms(encodedData.constData(), encodedData.size());

            Type* dataType = AZ::Utils::LoadObjectFromStream<Type>(ms, nullptr);

            if (dataType)
            {
                return (*dataType);
            }
            
            return Type{};
        }
    };
}
