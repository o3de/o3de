/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <MCore/Source/LogManager.h>


namespace ExporterLib
{
    void SaveMotionData(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        EMotionFX::MotionData* motionData = motion->GetMotionData();
        AZ_Assert(motionData, "Expecting a valid motion data pointer");

        const AZStd::string uuidString = motionData->RTTI_GetType().ToString<AZStd::string>();
        const AZStd::string nameString = motionData->RTTI_GetTypeName();

        // Write the chunk header.
        EMotionFX::MotionData::SaveSettings saveSettings;
        saveSettings.m_targetEndianType = targetEndianType;
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId = EMotionFX::FileFormat::MOTION_CHUNK_MOTIONDATA;
        chunkHeader.m_version = 1;
        chunkHeader.m_sizeInBytes = static_cast<AZ::u32>(
            sizeof(EMotionFX::FileFormat::Motion_MotionData) +
            ExporterLib::GetAzStringChunkSize(uuidString) +
            ExporterLib::GetAzStringChunkSize(nameString) +
            motionData->CalcStreamSaveSizeInBytes(saveSettings)
        );
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // Write the motion data info.
        EMotionFX::FileFormat::Motion_MotionData motionDataHeader;
        motionDataHeader.m_sizeInBytes = static_cast<AZ::u32>(motionData->CalcStreamSaveSizeInBytes(saveSettings));
        motionDataHeader.m_dataVersion = motionData->GetStreamSaveVersion();
        ConvertUnsignedInt(&motionDataHeader.m_sizeInBytes, targetEndianType);
        ConvertUnsignedInt(&motionDataHeader.m_dataVersion, targetEndianType);
        file->Write(&motionDataHeader, sizeof(EMotionFX::FileFormat::Motion_MotionData));

        // Write the strings.
        SaveString(uuidString, file, targetEndianType);
        SaveString(nameString, file, targetEndianType);

        // Write the actual motion data.
        if (!motion->GetMotionData()->Save(file, saveSettings))
        {
            AZ_Error("EMotionFX", false, "Failed to save motion data!");
        }
    }

    void SaveRootMotionExtractionData(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        if (!motion->GetRootMotionExtractionData())
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&buffer);
        if (AZ::Utils::SaveObjectToStream<EMotionFX::RootMotionExtractionData>(
                stream, AZ::ObjectStream::ST_BINARY, motion->GetRootMotionExtractionData().get(), serializeContext))
        {
            const AZ::u32 bufferSize = static_cast<AZ::u32>(buffer.size());

            EMotionFX::FileFormat::FileChunk chunkHeader;
            chunkHeader.m_chunkId = EMotionFX::FileFormat::MOTION_CHUNK_ROOTMOTIONEXTRACTION;
            chunkHeader.m_version = 1;
            chunkHeader.m_sizeInBytes = bufferSize + sizeof(AZ::u32);

            ConvertFileChunk(&chunkHeader, targetEndianType);
            file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

            // Write the number of bytes again as inside the chunk processor we don't have access to the file chunk.
            AZ::u32 endianBufferSize = bufferSize;
            ConvertUnsignedInt(&endianBufferSize, targetEndianType);
            file->Write(&endianBufferSize, sizeof(endianBufferSize));

            file->Write(buffer.data(), bufferSize);
        }
        else
        {
            AZ_Error("EMotionFX", false, "Cannot save root motion extraction data. SaveObjectToStream() failed.");
        }
    }

    void SaveMotion(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        SaveMotionHeader(file, motion, targetEndianType);
        SaveMotionFileInfo(file, motion, targetEndianType);
        SaveMotionData(file, motion, targetEndianType);
        SaveMotionEvents(file, motion->GetEventTable(), targetEndianType);
        SaveRootMotionExtractionData(file, motion, targetEndianType);
    }

    void SaveMotion(AZStd::string& filename, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        if (filename.empty())
        {
            AZ_Error("EMotionFX", false, "Cannot save actor. Filename is empty.");
            return;
        }

        if (motion && motion->GetMotionData())
        {
            MCore::MemoryFile memoryFile;
            memoryFile.Open();
            memoryFile.SetPreAllocSize(262144); // 256kb

            // Save the motion to the memory file.
            SaveMotion(&memoryFile, motion, targetEndianType);

            // Make sure the file has the correct extension and write the data from memory to disk.
            AzFramework::StringFunc::Path::ReplaceExtension(filename, GetMotionExtension());
            memoryFile.SaveToDiskFile(filename.c_str());
            memoryFile.Close();
        }
    }
} // namespace ExporterLib
