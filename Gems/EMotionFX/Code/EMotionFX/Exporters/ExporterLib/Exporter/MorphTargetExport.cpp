/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"

#include <AzCore/Serialization/Locale.h>

#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/MorphSetup.h>


namespace ExporterLib
{
    // save the given morph target
    void SaveMorphTarget(MCore::Stream* file, EMotionFX::Actor* actor, EMotionFX::MorphTarget* inputMorphTarget, size_t lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);
        MCORE_ASSERT(inputMorphTarget);
        MCORE_ASSERT(inputMorphTarget->GetType() == EMotionFX::MorphTargetStandard::TYPE_ID);
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        const size_t numTransformations = morphTarget->GetNumTransformations();

        // copy over the information to the chunk
        EMotionFX::FileFormat::Actor_MorphTarget morphTargetChunk;
        morphTargetChunk.m_lod                   = aznumeric_caster(lodLevel);
        morphTargetChunk.m_numTransformations    = aznumeric_caster(numTransformations);
        morphTargetChunk.m_rangeMin              = morphTarget->GetRangeMin();
        morphTargetChunk.m_rangeMax              = morphTarget->GetRangeMax();
        morphTargetChunk.m_phonemeSets           = morphTarget->GetPhonemeSets();

        // log it
        MCore::LogDetailedInfo(" - Morph Target: Name='%s'",  morphTarget->GetName());
        MCore::LogDetailedInfo("    + LOD Level = %d", lodLevel);
        MCore::LogDetailedInfo("    + RangeMin = %f", morphTarget->GetRangeMin());
        MCore::LogDetailedInfo("    + RangeMax = %f", morphTarget->GetRangeMax());
        MCore::LogDetailedInfo("    + NumTransformations = %d", numTransformations);
        MCore::LogDetailedInfo("    + PhonemesSets: %s", EMotionFX::MorphTarget::GetPhonemeSetString((EMotionFX::MorphTarget::EPhonemeSet)morphTarget->GetPhonemeSets()).c_str());

        // convert endian
        ConvertFloat(&morphTargetChunk.m_rangeMin, targetEndianType);
        ConvertFloat(&morphTargetChunk.m_rangeMax, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.m_lod, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.m_numTransformations, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.m_phonemeSets, targetEndianType);

        // write the bones expression part
        file->Write(&morphTargetChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTarget));

        // save the mesh expression part name
        SaveString(morphTarget->GetName(), file, targetEndianType);

        // create and write the transformations
        for (size_t i = 0; i < numTransformations; i++)
        {
            EMotionFX::MorphTargetStandard::Transformation transform    = morphTarget->GetTransformation(i);
            EMotionFX::Node* node                                       = actor->GetSkeleton()->GetNode(transform.m_nodeIndex);
            if (node == nullptr)
            {
                MCore::LogError("Can't get node '%i'. File is corrupt!", transform.m_nodeIndex);
                continue;
            }

            // create and fill the transformation
            EMotionFX::FileFormat::Actor_MorphTargetTransform transformChunk;

            transformChunk.m_nodeIndex = aznumeric_caster(transform.m_nodeIndex);
            CopyVector(transformChunk.m_position, AZ::PackedVector3f(transform.m_position));
            CopyVector(transformChunk.m_scale, AZ::PackedVector3f(transform.m_scale));
            CopyQuaternion(transformChunk.m_rotation, transform.m_rotation);
            CopyQuaternion(transformChunk.m_scaleRotation, transform.m_scaleRotation);

            MCore::LogDetailedInfo("    - EMotionFX::Transform #%i: Node='%s' NodeNr=#%i", i, node->GetName(), node->GetNodeIndex());
            MCore::LogDetailedInfo("       + Pos:      %f, %f, %f", transformChunk.m_position.m_x, transformChunk.m_position.m_y, transformChunk.m_position.m_z);
            MCore::LogDetailedInfo("       + Rotation: %f, %f, %f %f", transformChunk.m_rotation.m_x, transformChunk.m_rotation.m_y, transformChunk.m_rotation.m_z, transformChunk.m_rotation.m_w);
            MCore::LogDetailedInfo("       + Scale:    %f, %f, %f", transformChunk.m_scale.m_x, transformChunk.m_scale.m_y, transformChunk.m_scale.m_z);
            MCore::LogDetailedInfo("       + ScaleRot: %f, %f, %f %f", transformChunk.m_scaleRotation.m_x, transformChunk.m_scaleRotation.m_y, transformChunk.m_scaleRotation.m_z, transformChunk.m_scaleRotation.m_w);

            // convert endian and coordinate system
            ConvertUnsignedInt(&transformChunk.m_nodeIndex, targetEndianType);
            ConvertFileVector3(&transformChunk.m_position, targetEndianType);
            ConvertFileVector3(&transformChunk.m_scale, targetEndianType);
            ConvertFileQuaternion(&transformChunk.m_rotation, targetEndianType);
            ConvertFileQuaternion(&transformChunk.m_scaleRotation, targetEndianType);

            // write the transformation
            file->Write(&transformChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTargetTransform));
        }
    }


    // get the size of the chunk for the given morph target
    size_t GetMorphTargetChunkSize(EMotionFX::MorphTarget* inputMorphTarget)
    {
        MCORE_ASSERT(inputMorphTarget->GetType() == EMotionFX::MorphTargetStandard::TYPE_ID);
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        size_t totalSize = 0;
        totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTarget);
        totalSize += GetStringChunkSize(morphTarget->GetName());
        totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTargetTransform) * morphTarget->GetNumTransformations();

        return totalSize;
    }


    // get the size of the chunk for the complete morph setup
    size_t GetMorphSetupChunkSize(EMotionFX::MorphSetup* morphSetup)
    {
        // get the number of morph targets
        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();

        // calculate the size of the chunk
        size_t totalSize = sizeof(EMotionFX::FileFormat::Actor_MorphTargets);
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            totalSize += GetMorphTargetChunkSize(morphSetup->GetMorphTarget(i));
        }

        return totalSize;
    }

    size_t GetNumSavedMorphTargets(EMotionFX::MorphSetup* morphSetup)
    {
        return morphSetup->GetNumMorphTargets();
    }

    // save all morph targets for a given LOD level
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, size_t lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);

        EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
        if (morphSetup == nullptr)
        {
            return;
        }

        // get the number of morph targets we need to save to the file and check if there are any at all
        const size_t numSavedMorphTargets = GetNumSavedMorphTargets(morphSetup);
        if (numSavedMorphTargets <= 0)
        {
            MCore::LogInfo("No morph targets to be saved in morph setup. Skipping writing morph targets.");
            return;
        }

        // get the number of morph targets
        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();

        // check if all morph targets have a valid name and rename them in case they are empty
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);

            // check if the name of the morph target is valid
            if (morphTarget->GetNameString().empty())
            {
                // rename the morph target
                AZStd::string morphTargetName;
                morphTargetName = AZStd::string::format("Morph Target %zu", MCore::GetIDGenerator().GenerateID());
                MCore::LogWarning("The morph target has an empty name. The morph target will be automatically renamed to '%s'.", morphTargetName.c_str());
                morphTarget->SetName(morphTargetName.c_str());
            }
        }

        // fill in the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.m_chunkId        = EMotionFX::FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS;
        chunkHeader.m_sizeInBytes    = aznumeric_caster(GetMorphSetupChunkSize(morphSetup));
        chunkHeader.m_version        = 2;

        // endian convert the chunk and write it to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // fill in the chunk header
        EMotionFX::FileFormat::Actor_MorphTargets morphTargetsChunk;
        morphTargetsChunk.m_numMorphTargets  = aznumeric_caster(numSavedMorphTargets);
        morphTargetsChunk.m_lod              = aznumeric_caster(lodLevel);

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Morph Targets (%i, LOD=%d)", morphTargetsChunk.m_numMorphTargets, morphTargetsChunk.m_lod);
        MCore::LogDetailedInfo("============================================================");

        // endian convert the chunk and write it to the file
        ConvertUnsignedInt(&morphTargetsChunk.m_numMorphTargets, targetEndianType);
        ConvertUnsignedInt(&morphTargetsChunk.m_lod, targetEndianType);
        file->Write(&morphTargetsChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTargets));

        // save morph targets
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            SaveMorphTarget(file, actor, morphSetup->GetMorphTarget(i), lodLevel, targetEndianType);
        }
    }

    // save all morph targets for all LOD levels
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of LOD levels and save the morph targets for each
        const size_t numLODLevels = actor->GetNumLODLevels();
        for (size_t i = 0; i < numLODLevels; ++i)
        {
            SaveMorphTargets(file, actor, i, targetEndianType);
        }
    }
} // namespace ExporterLib
