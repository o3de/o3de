/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <MCore/Source/MemoryFile.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/MotionFileFormat.h>

namespace MCore
{
    class CommandManager;
}

namespace EMotionFX
{
    class Actor;
    class Node;
    class NodeGroup;
    class MorphTarget;
    class AnimGraph;
    class MotionEventTable;
    class Motion;
    class MotionSet;
}


namespace ExporterLib
{
    // windows platform endian type
    #define EXPLIB_PLATFORM_ENDIAN MCore::Endian::ENDIAN_LITTLE

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helpers
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // helpers
    void CopyVector2(EMotionFX::FileFormat::FileVector2& to, const AZ::Vector2& from);
    void CopyVector(EMotionFX::FileFormat::FileVector3& to, const AZ::PackedVector3f& from);
    void CopyQuaternion(EMotionFX::FileFormat::FileQuaternion& to, const AZ::Quaternion& from);
    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const AZ::Quaternion& from);
    void Copy16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion& to, const MCore::Compressed16BitQuaternion& from);

    // endian conversion
    void ConvertUnsignedInt(uint32* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertUnsignedInt(uint64* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertInt(int* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertUnsignedShort(uint16* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFloat(float* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileChunk(EMotionFX::FileFormat::FileChunk* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileColor(EMotionFX::FileFormat::FileColor* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileVector2(EMotionFX::FileFormat::FileVector2* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileVector3(EMotionFX::FileFormat::FileVector3* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFile16BitVector3(EMotionFX::FileFormat::File16BitVector3* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileQuaternion(EMotionFX::FileFormat::FileQuaternion* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFile16BitQuaternion(EMotionFX::FileFormat::File16BitQuaternion* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileMotionEvent(EMotionFX::FileFormat::FileMotionEvent* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertFileMotionEventTable(EMotionFX::FileFormat::FileMotionEventTrack* value, MCore::Endian::EEndianType targetEndianType);
    void ConvertVector3(AZ::PackedVector3f* value, MCore::Endian::EEndianType targetEndianType);

    uint32 GetFileHighVersion();
    uint32 GetFileLowVersion();
    const char* GetCompilationDate();

    /**
     * Save the given string to a file. It will first save an integer determining the number of characters which follow and
     * then the actual string data.
     * @param textToSave The string to save.
     * @param file The file stream to save the string to.
     */
    void SaveString(const AZStd::string& textToSave, MCore::Stream* file, MCore::Endian::EEndianType targetEndianType);
    void SaveAzString(const AZStd::string& textToSave, MCore::Stream* file, MCore::Endian::EEndianType targetEndianType);

    /**
     * Return the size in bytes the string chunk will have.
     * @param text The string to check the chunk size for.
     * @return The size the string chunk will have in bytes.
     */
    uint32 GetStringChunkSize(const AZStd::string& text);
    size_t GetAzStringChunkSize(const AZStd::string& text);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Actors
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // nodes
    void SaveNodes(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);
    void SaveNodeGroup(MCore::Stream* file, EMotionFX::NodeGroup* nodeGroup, MCore::Endian::EEndianType targetEndianType);
    void SaveNodeGroups(MCore::Stream* file, const AZStd::vector<EMotionFX::NodeGroup*>& nodeGroups, MCore::Endian::EEndianType targetEndianType);
    void SaveNodeGroups(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);
    void SaveNodeMotionSources(MCore::Stream* file, EMotionFX::Actor* actor, AZStd::vector<EMotionFX::Actor::NodeMirrorInfo>* mirrorInfo, MCore::Endian::EEndianType targetEndianType);
    void SaveAttachmentNodes(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);
    void SaveAttachmentNodes(MCore::Stream* file, EMotionFX::Actor* actor, const AZStd::vector<uint16>& attachmentNodes, MCore::Endian::EEndianType targetEndianType);

    // morph targets
    void SaveMorphTarget(MCore::Stream* file, EMotionFX::Actor* actor, EMotionFX::MorphTarget* inputMorphTarget, size_t lodLevel, MCore::Endian::EEndianType targetEndianType);
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, size_t lodLevel, MCore::Endian::EEndianType targetEndianType);
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType);

    // actors
    const char* GetActorExtension(bool includingDot = true);
    void SaveActorHeader(MCore::Stream* file, MCore::Endian::EEndianType targetEndianType);
    void SaveActorFileInfo(MCore::Stream* file, uint64 numLODLevels, uint64 motionExtractionNodeIndex, uint64 retargetRootNodeIndex, const char* sourceApp, const char* orgFileName, const char* actorName, MCore::Distance::EUnitType unitType, MCore::Endian::EEndianType targetEndianType, bool optimizeSkeleton);
    void SaveActor(MCore::MemoryFile* file, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType, const AZStd::optional<AZ::Data::AssetId> meshAssetId = AZStd::nullopt);
    bool SaveActor(AZStd::string& filename, const EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType, const AZStd::optional<AZ::Data::AssetId> meshAssetId = AZStd::nullopt);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Motions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    const char* GetMotionExtension(bool includingDot = true);
    void SaveMotionEvents(MCore::Stream* file, EMotionFX::MotionEventTable* motionEventTable, MCore::Endian::EEndianType targetEndianType);
    void SaveMotionHeader(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);
    void SaveMotionFileInfo(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);
    void SaveMotion(MCore::Stream* file, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);
    void SaveMotion(AZStd::string& filename, EMotionFX::Motion* motion, MCore::Endian::EEndianType targetEndianType);
} // namespace ExporterLib
