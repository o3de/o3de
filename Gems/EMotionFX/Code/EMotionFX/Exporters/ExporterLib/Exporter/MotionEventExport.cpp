/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/FileFunc/FileFunc.h>
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

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't save motion events. Can't get serialize context from component application.");
            return;
        }

        AZ::JsonSerializerSettings settings;
        settings.m_serializeContext = context;
        rapidjson::Document jsonDocument;
        auto jsonResult = AZ::JsonSerialization::Store(jsonDocument, jsonDocument.GetAllocator(), *motionEventTable, settings);
        if (jsonResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error("EMotionFX", false, "JSON serialization failed: %s", jsonResult.ToString("").c_str());
            return;
        }
        
        AZStd::string serializedMotionEventTable;
        auto writeToStringOutcome = AzFramework::FileFunc::WriteJsonToString(jsonDocument, serializedMotionEventTable);
        if (!writeToStringOutcome.IsSuccess())
        {
            AZ_Error("EMotionFX", false, "WriteJsonToString failed: %s", writeToStringOutcome.GetError().c_str());
            return;
        }

        const size_t serializedTableSizeInBytes = serializedMotionEventTable.size();

        // the motion event table chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID = EMotionFX::FileFormat::SHARED_CHUNK_MOTIONEVENTTABLE;
        chunkHeader.mVersion = 3;

        chunkHeader.mSizeInBytes = static_cast<uint32>(serializedTableSizeInBytes + sizeof(EMotionFX::FileFormat::FileMotionEventTableSerialized));

        EMotionFX::FileFormat::FileMotionEventTableSerialized tableHeader;
        tableHeader.m_size = serializedTableSizeInBytes;

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);

        // save the chunk header and the chunk
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&tableHeader, sizeof(EMotionFX::FileFormat::FileMotionEventTableSerialized));
        file->Write(serializedMotionEventTable.c_str(), serializedTableSizeInBytes);
    }
} // namespace ExporterLib
