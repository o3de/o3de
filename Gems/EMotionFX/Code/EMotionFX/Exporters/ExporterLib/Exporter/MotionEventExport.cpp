/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
