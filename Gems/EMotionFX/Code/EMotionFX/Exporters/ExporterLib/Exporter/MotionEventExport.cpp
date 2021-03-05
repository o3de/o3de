/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Exporter.h"
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/Motion.h>
#include <MCore/Source/LogManager.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>

namespace ExporterLib
{
    void SaveMotionEvents(MCore::Stream* file, EMotionFX::MotionEventTable* motionEventTable, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of motion event tracks and check if we have to save
        // any at all
        if (motionEventTable->GetNumTracks() == 0)
        {
            return;
        }

        AZ::Outcome<AZStd::string> serializedMotionEventTable = MCore::ReflectionSerializer::Serialize(motionEventTable);
        if (!serializedMotionEventTable.IsSuccess())
        {
            return;
        }
        const size_t serializedTableSizeInBytes = serializedMotionEventTable.GetValue().size();

        // the motion event table chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE;
        chunkHeader.mVersion = 2;

        chunkHeader.mSizeInBytes = static_cast<uint32>(serializedTableSizeInBytes + sizeof(EMotionFX::FileFormat::FileMotionEventTableSerialized));

        EMotionFX::FileFormat::FileMotionEventTableSerialized tableHeader;
        tableHeader.m_size = serializedTableSizeInBytes;

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);

        // save the chunk header and the chunk
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&tableHeader, sizeof(EMotionFX::FileFormat::FileMotionEventTableSerialized));
        file->Write(serializedMotionEventTable.GetValue().c_str(), serializedTableSizeInBytes);
    }
} // namespace ExporterLib
