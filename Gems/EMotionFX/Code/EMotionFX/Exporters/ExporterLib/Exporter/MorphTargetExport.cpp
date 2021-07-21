/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
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
    void SaveMorphTarget(MCore::Stream* file, EMotionFX::Actor* actor, EMotionFX::MorphTarget* inputMorphTarget, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);
        MCORE_ASSERT(inputMorphTarget);
        MCORE_ASSERT(inputMorphTarget->GetType() == EMotionFX::MorphTargetStandard::TYPE_ID);
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        const uint32 numTransformations = morphTarget->GetNumTransformations();

        // copy over the information to the chunk
        EMotionFX::FileFormat::Actor_MorphTarget morphTargetChunk;
        morphTargetChunk.mLOD                   = lodLevel;
        morphTargetChunk.mNumTransformations    = numTransformations;
        morphTargetChunk.mRangeMin              = morphTarget->GetRangeMin();
        morphTargetChunk.mRangeMax              = morphTarget->GetRangeMax();
        morphTargetChunk.mPhonemeSets           = morphTarget->GetPhonemeSets();

        // log it
        MCore::LogDetailedInfo(" - Morph Target: Name='%s'",  morphTarget->GetName());
        MCore::LogDetailedInfo("    + LOD Level = %d", lodLevel);
        MCore::LogDetailedInfo("    + RangeMin = %f", morphTarget->GetRangeMin());
        MCore::LogDetailedInfo("    + RangeMax = %f", morphTarget->GetRangeMax());
        MCore::LogDetailedInfo("    + NumTransformations = %d", numTransformations);
        MCore::LogDetailedInfo("    + PhonemesSets: %s", EMotionFX::MorphTarget::GetPhonemeSetString((EMotionFX::MorphTarget::EPhonemeSet)morphTarget->GetPhonemeSets()).c_str());

        // convert endian
        ConvertFloat(&morphTargetChunk.mRangeMin, targetEndianType);
        ConvertFloat(&morphTargetChunk.mRangeMax, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mLOD, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mNumTransformations, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mPhonemeSets, targetEndianType);

        // write the bones expression part
        file->Write(&morphTargetChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTarget));

        // save the mesh expression part name
        SaveString(morphTarget->GetName(), file, targetEndianType);

        // create and write the transformations
        for (uint32 i = 0; i < numTransformations; i++)
        {
            EMotionFX::MorphTargetStandard::Transformation transform    = morphTarget->GetTransformation(i);
            EMotionFX::Node* node                                       = actor->GetSkeleton()->GetNode(transform.mNodeIndex);
            if (node == nullptr)
            {
                MCore::LogError("Can't get node '%i'. File is corrupt!", transform.mNodeIndex);
                continue;
            }

            // create and fill the transformation
            EMotionFX::FileFormat::Actor_MorphTargetTransform transformChunk;

            transformChunk.mNodeIndex = transform.mNodeIndex;
            CopyVector(transformChunk.mPosition, AZ::PackedVector3f(transform.mPosition));
            CopyVector(transformChunk.mScale, AZ::PackedVector3f(transform.mScale));
            CopyQuaternion(transformChunk.mRotation, transform.mRotation);
            CopyQuaternion(transformChunk.mScaleRotation, transform.mScaleRotation);

            MCore::LogDetailedInfo("    - EMotionFX::Transform #%i: Node='%s' NodeNr=#%i", i, node->GetName(), node->GetNodeIndex());
            MCore::LogDetailedInfo("       + Pos:      %f, %f, %f", transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
            MCore::LogDetailedInfo("       + Rotation: %f, %f, %f %f", transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
            MCore::LogDetailedInfo("       + Scale:    %f, %f, %f", transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
            MCore::LogDetailedInfo("       + ScaleRot: %f, %f, %f %f", transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

            // convert endian and coordinate system
            ConvertUnsignedInt(&transformChunk.mNodeIndex, targetEndianType);
            ConvertFileVector3(&transformChunk.mPosition, targetEndianType);
            ConvertFileVector3(&transformChunk.mScale, targetEndianType);
            ConvertFileQuaternion(&transformChunk.mRotation, targetEndianType);
            ConvertFileQuaternion(&transformChunk.mScaleRotation, targetEndianType);

            // write the transformation
            file->Write(&transformChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTargetTransform));
        }
    }


    // get the size of the chunk for the given morph target
    uint32 GetMorphTargetChunkSize(EMotionFX::MorphTarget* inputMorphTarget)
    {
        MCORE_ASSERT(inputMorphTarget->GetType() == EMotionFX::MorphTargetStandard::TYPE_ID);
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        uint32 totalSize = 0;
        totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTarget);
        totalSize += GetStringChunkSize(morphTarget->GetName());
        totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTargetTransform) * morphTarget->GetNumTransformations();

        return totalSize;
    }


    // get the size of the chunk for the complete morph setup
    uint32 GetMorphSetupChunkSize(EMotionFX::MorphSetup* morphSetup)
    {
        // get the number of morph targets
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();

        // calculate the size of the chunk
        uint32 totalSize = sizeof(EMotionFX::FileFormat::Actor_MorphTargets);
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            totalSize += GetMorphTargetChunkSize(morphSetup->GetMorphTarget(i));
        }

        return totalSize;
    }

    uint32 GetNumSavedMorphTargets(EMotionFX::MorphSetup* morphSetup)
    {
        return morphSetup->GetNumMorphTargets();
    }

    // save all morph targets for a given LOD level
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        uint32 i;
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);

        EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
        if (morphSetup == nullptr)
        {
            return;
        }

        // get the number of morph targets we need to save to the file and check if there are any at all
        const uint32 numSavedMorphTargets = GetNumSavedMorphTargets(morphSetup);
        if (numSavedMorphTargets <= 0)
        {
            MCore::LogInfo("No morph targets to be saved in morph setup. Skipping writing morph targets.");
            return;
        }

        // get the number of morph targets
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();

        // check if all morph targets have a valid name and rename them in case they are empty
        for (i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);

            // check if the name of the morph target is valid
            if (morphTarget->GetNameString().empty())
            {
                // rename the morph target
                AZStd::string morphTargetName;
                morphTargetName = AZStd::string::format("Morph Target %d", MCore::GetIDGenerator().GenerateID());
                MCore::LogWarning("The morph target has an empty name. The morph target will be automatically renamed to '%s'.", morphTargetName.c_str());
                morphTarget->SetName(morphTargetName.c_str());
            }
        }

        // fill in the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_STDPMORPHTARGETS;
        chunkHeader.mSizeInBytes    = GetMorphSetupChunkSize(morphSetup);
        chunkHeader.mVersion        = 2;

        // endian convert the chunk and write it to the file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // fill in the chunk header
        EMotionFX::FileFormat::Actor_MorphTargets morphTargetsChunk;
        morphTargetsChunk.mNumMorphTargets  = numSavedMorphTargets;
        morphTargetsChunk.mLOD              = lodLevel;

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Morph Targets (%i, LOD=%d)", morphTargetsChunk.mNumMorphTargets, morphTargetsChunk.mLOD);
        MCore::LogDetailedInfo("============================================================");

        // endian convert the chunk and write it to the file
        ConvertUnsignedInt(&morphTargetsChunk.mNumMorphTargets, targetEndianType);
        ConvertUnsignedInt(&morphTargetsChunk.mLOD, targetEndianType);
        file->Write(&morphTargetsChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTargets));

        // save morph targets
        for (i = 0; i < numMorphTargets; ++i)
        {
            SaveMorphTarget(file, actor, morphSetup->GetMorphTarget(i), lodLevel, targetEndianType);
        }
    }

    // save all morph targets for all LOD levels
    void SaveMorphTargets(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of LOD levels and save the morph targets for each
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            SaveMorphTargets(file, actor, i, targetEndianType);
        }
    }
} // namespace ExporterLib
