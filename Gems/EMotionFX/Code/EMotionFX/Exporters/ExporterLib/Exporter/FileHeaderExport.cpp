/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"

#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <MCore/Source/LogManager.h>


namespace ExporterLib
{
    void SaveActorHeader(MCore::Stream* file, MCore::Endian::EEndianType targetEndianType)
    {
        // the header information
        EMotionFX::FileFormat::Actor_Header header;
        memset(&header, 0, sizeof(EMotionFX::FileFormat::Actor_Header));
        header.m_fourcc[0] = 'A';
        header.m_fourcc[1] = 'C';
        header.m_fourcc[2] = 'T';
        header.m_fourcc[3] = 'R';
        header.m_hiVersion   = static_cast<uint8>(GetFileHighVersion());
        header.m_loVersion   = static_cast<uint8>(GetFileLowVersion());
        header.m_endianType  = static_cast<uint8>(targetEndianType);

        // write the header to the stream
        file->Write(&header, sizeof(EMotionFX::FileFormat::Actor_Header));
    }


    void SaveActorFileInfo(MCore::Stream* file,
        uint64 numLODLevels,
        uint64 motionExtractionNodeIndex,
        uint64 retargetRootNodeIndex,
        const char* sourceApp,
        const char* orgFileName,
        const char* actorName,
        MCore::Distance::EUnitType unitType,
        MCore::Endian::EEndianType targetEndianType,
        bool optimizeSkeleton)
    {
        // chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId        = EMotionFX::FileFormat::ACTOR_CHUNK_INFO;

        chunkHeader.m_sizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_Info3);
        chunkHeader.m_sizeInBytes    += GetStringChunkSize(sourceApp);
        chunkHeader.m_sizeInBytes    += GetStringChunkSize(orgFileName);
        chunkHeader.m_sizeInBytes    += GetStringChunkSize(GetCompilationDate());
        chunkHeader.m_sizeInBytes    += GetStringChunkSize(actorName);

        chunkHeader.m_version        = 3;

        EMotionFX::FileFormat::Actor_Info3 infoChunk;
        memset(&infoChunk, 0, sizeof(EMotionFX::FileFormat::Actor_Info3));
        infoChunk.m_numLoDs                      = aznumeric_caster(numLODLevels);
        infoChunk.m_motionExtractionNodeIndex    = aznumeric_caster(motionExtractionNodeIndex);
        infoChunk.m_retargetRootNodeIndex        = aznumeric_caster(retargetRootNodeIndex);
        infoChunk.m_exporterHighVersion          = static_cast<uint8>(EMotionFX::GetEMotionFX().GetHighVersion());
        infoChunk.m_exporterLowVersion           = static_cast<uint8>(EMotionFX::GetEMotionFX().GetLowVersion());
        infoChunk.m_unitType                     = static_cast<uint8>(unitType);
        infoChunk.m_optimizeSkeleton             = optimizeSkeleton ? 1 : 0;

        // print repositioning node information
        MCore::LogDetailedInfo("- File Info");
        MCore::LogDetailedInfo("   + Actor Name: '%s'", actorName);
        MCore::LogDetailedInfo("   + Source Application: '%s'", sourceApp);
        MCore::LogDetailedInfo("   + Original File: '%s'", orgFileName);
        MCore::LogDetailedInfo("   + Exporter Version: v%d.%d", infoChunk.m_exporterHighVersion, infoChunk.m_exporterLowVersion);
        MCore::LogDetailedInfo("   + Exporter Compilation Date: '%s'", GetCompilationDate());
        MCore::LogDetailedInfo("   + Num LODs = %d", infoChunk.m_numLoDs);
        MCore::LogDetailedInfo("   + Motion extraction node index = %d", infoChunk.m_motionExtractionNodeIndex);
        MCore::LogDetailedInfo("   + Retarget root node index = %d", infoChunk.m_retargetRootNodeIndex);

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);
        ConvertUnsignedInt(&infoChunk.m_motionExtractionNodeIndex, targetEndianType);
        ConvertUnsignedInt(&infoChunk.m_retargetRootNodeIndex, targetEndianType);
        ConvertUnsignedInt(&infoChunk.m_numLoDs, targetEndianType);

        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&infoChunk, sizeof(EMotionFX::FileFormat::Actor_Info3));

        SaveString(sourceApp, file, targetEndianType);
        SaveString(orgFileName, file, targetEndianType);
        // Save an empty string as the compilation date (Don't need that information anymore).
        SaveString("", file, targetEndianType);
        SaveString(actorName, file, targetEndianType);
    }


    void SaveMotionHeader(MCore::Stream* file, [[maybe_unused]] EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // the header information
        EMotionFX::FileFormat::Motion_Header header;
        memset(&header, 0, sizeof(EMotionFX::FileFormat::Motion_Header));

        header.m_fourcc[0]   = 'M';
        header.m_fourcc[1]   = 'O';
        header.m_fourcc[2]   = 'T';
        header.m_fourcc[3] = ' ';

        header.m_hiVersion   = static_cast<uint8>(GetFileHighVersion());
        header.m_loVersion   = static_cast<uint8>(GetFileLowVersion());
        header.m_endianType  = static_cast<uint8>(targetEndianType);

        // write the header to the stream
        file->Write(&header, sizeof(EMotionFX::FileFormat::Motion_Header));
    }


    void SaveMotionFileInfo(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType)
    {
        // chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId      = EMotionFX::FileFormat::MOTION_CHUNK_INFO;
        chunkHeader.m_sizeInBytes  = sizeof(EMotionFX::FileFormat::Motion_Info3);
        chunkHeader.m_version      = 3;

        EMotionFX::FileFormat::Motion_Info3 infoChunk;
        memset(&infoChunk, 0, sizeof(EMotionFX::FileFormat::Motion_Info3));

        infoChunk.m_motionExtractionFlags    = motion->GetMotionExtractionFlags();
        infoChunk.m_motionExtractionNodeIndex= MCORE_INVALIDINDEX32; // not used anymore
        infoChunk.m_unitType                 = static_cast<uint8>(motion->GetUnitType());
        infoChunk.m_isAdditive               = motion->GetMotionData()->IsAdditive() ? 1 : 0;

        MCore::LogDetailedInfo("- File Info");
        MCore::LogDetailedInfo("   + Exporter Compilation Date    = '%s'", GetCompilationDate());
        MCore::LogDetailedInfo("   + Motion Extraction Flags      = 0x%x [capZ=%d]", infoChunk.m_motionExtractionFlags, (infoChunk.m_motionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);
        ConvertUnsignedInt(&infoChunk.m_motionExtractionFlags, targetEndianType);
        ConvertUnsignedInt(&infoChunk.m_motionExtractionNodeIndex, targetEndianType);

        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&infoChunk, sizeof(EMotionFX::FileFormat::Motion_Info2));
    }
} // namespace ExporterLib
