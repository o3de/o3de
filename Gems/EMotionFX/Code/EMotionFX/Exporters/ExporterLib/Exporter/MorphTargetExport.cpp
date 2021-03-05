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
        uint32 i, j;

        // down cast the morph target
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        // get the number of deform datas, transformations
        const uint32 numDeformDatas     = morphTarget->GetNumDeformDatas();
        const uint32 numTransformations = morphTarget->GetNumTransformations();

        // copy over the information to the chunk
        EMotionFX::FileFormat::Actor_MorphTarget morphTargetChunk;
        morphTargetChunk.mLOD                   = lodLevel;
        morphTargetChunk.mNumMeshDeformDeltas   = numDeformDatas;
        morphTargetChunk.mNumTransformations    = numTransformations;
        morphTargetChunk.mRangeMin              = morphTarget->GetRangeMin();
        morphTargetChunk.mRangeMax              = morphTarget->GetRangeMax();
        morphTargetChunk.mPhonemeSets           = morphTarget->GetPhonemeSets();

        // log it
        MCore::LogDetailedInfo(" - Morph Target: Name='%s'",  morphTarget->GetName());
        MCore::LogDetailedInfo("    + LOD Level = %d", lodLevel);
        MCore::LogDetailedInfo("    + RangeMin = %f", morphTarget->GetRangeMin());
        MCore::LogDetailedInfo("    + RangeMax = %f", morphTarget->GetRangeMax());
        MCore::LogDetailedInfo("    + NumDeformDatas = %d", numDeformDatas);
        MCore::LogDetailedInfo("    + NumTransformations = %d", numTransformations);
        MCore::LogDetailedInfo("    + PhonemesSets: %s", EMotionFX::MorphTarget::GetPhonemeSetString((EMotionFX::MorphTarget::EPhonemeSet)morphTarget->GetPhonemeSets()).c_str());

        // convert endian
        ConvertFloat(&morphTargetChunk.mRangeMin, targetEndianType);
        ConvertFloat(&morphTargetChunk.mRangeMax, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mLOD, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mNumMeshDeformDeltas, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mNumTransformations, targetEndianType);
        ConvertUnsignedInt(&morphTargetChunk.mPhonemeSets, targetEndianType);

        // write the bones expression part
        file->Write(&morphTargetChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTarget));

        // save the mesh expression part name
        SaveString(morphTarget->GetName(), file, targetEndianType);


        // write the deform datas
        for (i = 0; i < numDeformDatas; ++i)
        {
            // write the deform data struct
            EMotionFX::MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(i);

            EMotionFX::FileFormat::Actor_MorphTargetMeshDeltas2 meshDeformDataChunk;

            meshDeformDataChunk.mNodeIndex  = deformData->mNodeIndex;
            meshDeformDataChunk.mNumVertices = deformData->mNumVerts;
            meshDeformDataChunk.mMinValue   = deformData->mMinValue;
            meshDeformDataChunk.mMaxValue   = deformData->mMaxValue;

            MCore::LogDetailedInfo("    + Deform data #%i:Node='%s' NodeNr=#%i", i, actor->GetSkeleton()->GetNode(deformData->mNodeIndex)->GetName(), deformData->mNodeIndex);
            MCore::LogDetailedInfo("       - NumVertices = %d", meshDeformDataChunk.mNumVertices);
            MCore::LogDetailedInfo("       - MinValue    = %f", meshDeformDataChunk.mMinValue);
            MCore::LogDetailedInfo("       - MaxValue    = %f", meshDeformDataChunk.mMaxValue);

            // convert endian
            ConvertFloat(&meshDeformDataChunk.mMinValue, targetEndianType);
            ConvertFloat(&meshDeformDataChunk.mMaxValue, targetEndianType);
            ConvertUnsignedInt(&meshDeformDataChunk.mNumVertices, targetEndianType);
            ConvertUnsignedInt(&meshDeformDataChunk.mNodeIndex, targetEndianType);

            file->Write(&meshDeformDataChunk, sizeof(EMotionFX::FileFormat::Actor_MorphTargetMeshDeltas2));


            // write the position deltas
            EMotionFX::FileFormat::File16BitVector3 deltaP;
            uint32 a;
            for (a = 0; a < deformData->mNumVerts; ++a)
            {
                deltaP.mX = deformData->mDeltas[a].mPosition.mX;
                deltaP.mY = deformData->mDeltas[a].mPosition.mY;
                deltaP.mZ = deformData->mDeltas[a].mPosition.mZ;
                ConvertUnsignedShort(&deltaP.mX, targetEndianType);
                ConvertUnsignedShort(&deltaP.mY, targetEndianType);
                ConvertUnsignedShort(&deltaP.mZ, targetEndianType);
                file->Write(&deltaP, sizeof(EMotionFX::FileFormat::File16BitVector3));
            }

            // write the normal deltas
            EMotionFX::FileFormat::File8BitVector3 deltaN;
            for (a = 0; a < deformData->mNumVerts; ++a)
            {
                deltaN.mX = deformData->mDeltas[a].mNormal.mX;
                deltaN.mY = deformData->mDeltas[a].mNormal.mY;
                deltaN.mZ = deformData->mDeltas[a].mNormal.mZ;
                file->Write(&deltaN, sizeof(EMotionFX::FileFormat::File8BitVector3));
            }

            // write the tangent deltas
            EMotionFX::FileFormat::File8BitVector3 deltaT;
            for (a = 0; a < deformData->mNumVerts; ++a)
            {
                deltaT.mX = deformData->mDeltas[a].mTangent.mX;
                deltaT.mY = deformData->mDeltas[a].mTangent.mY;
                deltaT.mZ = deformData->mDeltas[a].mTangent.mZ;
                file->Write(&deltaT, sizeof(EMotionFX::FileFormat::File8BitVector3));
            }

            // write the bitangent deltas
            EMotionFX::FileFormat::File8BitVector3 deltaBT;
            for (a = 0; a < deformData->mNumVerts; ++a)
            {
                deltaBT.mX = deformData->mDeltas[a].mBitangent.mX;
                deltaBT.mY = deformData->mDeltas[a].mBitangent.mY;
                deltaBT.mZ = deformData->mDeltas[a].mBitangent.mZ;
                file->Write(&deltaBT, sizeof(EMotionFX::FileFormat::File8BitVector3));
            }

            // write the vertex numbers
            uint32 vtx;
            for (a = 0; a < deformData->mNumVerts; ++a)
            {
                vtx = deformData->mDeltas[a].mVertexNr;

                // convert endian
                ConvertUnsignedInt(&vtx, targetEndianType);

                file->Write(&vtx, sizeof(uint32));
            }
        }


        // create and write the transformations
        for (j = 0; j < numTransformations; j++)
        {
            EMotionFX::MorphTargetStandard::Transformation transform    = morphTarget->GetTransformation(j);
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

            MCore::LogDetailedInfo("    - EMotionFX::Transform #%i: Node='%s' NodeNr=#%i", j, node->GetName(), node->GetNodeIndex());
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

        // down cast the morph target
        EMotionFX::MorphTargetStandard* morphTarget = (EMotionFX::MorphTargetStandard*)inputMorphTarget;

        const uint32 numDeformDatas     = morphTarget->GetNumDeformDatas();
        const uint32 numTransformations = morphTarget->GetNumTransformations();

        uint32 totalSize = 0;
        totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTarget);
        totalSize += GetStringChunkSize(morphTarget->GetName());

        // size of the deform data
        uint32 i;
        for (i = 0; i < numDeformDatas; ++i)
        {
            EMotionFX::MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(i);

            totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTargetMeshDeltas2);
            totalSize += deformData->mNumVerts * sizeof(EMotionFX::FileFormat::File16BitVector3); // positions
            totalSize += deformData->mNumVerts * sizeof(EMotionFX::FileFormat::File8BitVector3);  // normals
            totalSize += deformData->mNumVerts * sizeof(EMotionFX::FileFormat::File8BitVector3);  // tangents
            totalSize += deformData->mNumVerts * sizeof(EMotionFX::FileFormat::File8BitVector3);  // bitangents
            totalSize += deformData->mNumVerts * sizeof(uint32); // vertex indices
        }

        // size of the transformations
        for (i = 0; i < numTransformations; ++i)
        {
            totalSize += sizeof(EMotionFX::FileFormat::Actor_MorphTargetTransform);
        }

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


    bool AddMorphTarget(EMotionFX::Actor* actor, MCore::MemoryFile* file, const AZStd::string& morphTargetName, uint32 captureMode, uint32 phonemeSets, float rangeMin, float rangeMax, uint32 geomLODLevel)
    {
        MCORE_ASSERT(actor && file);

        // check if the memory file actually contains any data
        if (file->GetFileSize() <= 0)
        {
            MCore::LogWarning("Memory file does not contain any data. Skipping morph target '%s'.", morphTargetName.c_str());
            return false;
        }

        // get the morph setup and create a new one if it doesn't exist yet
        EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(geomLODLevel);
        if (morphSetup == nullptr)
        {
            morphSetup = EMotionFX::MorphSetup::Create();
        }

        // bind the morph setup
        actor->SetMorphSetup(geomLODLevel, morphSetup);

        // seek to the beginning of the memory file and try to load the actor
        file->Seek(0);
        EMotionFX::Importer::ActorSettings loadSettings;
        loadSettings.mAutoGenTangents = false;
        loadSettings.mUnitTypeConvert = false;

        // the morph target actor will be automatically destructed when the morph target will be created
        AZStd::unique_ptr<EMotionFX::Actor> morphTargetActor = EMotionFX::GetImporter().LoadActor(file, &loadSettings);
        if (!morphTargetActor)
        {
            MCore::LogError("Could not load morph target '%s'.", morphTargetName.c_str());
            return false;
        }

        // set the morph target name as actor name
        morphTargetActor->SetName(morphTargetName.c_str());

        // check if a morph target with the same name already exists
        if (morphSetup->FindMorphTargetByName(morphTargetName.c_str()))
        {
            MCore::LogError("Error while adding morph target. There already is a morph target named '%s'. This will lead to view errors. Please check for duplicated morph target names.", morphTargetName.c_str());
            return false;
        }

        // create the morph target based on the given capture mode
        EMotionFX::MorphTargetStandard* morphTarget = nullptr;
        switch (captureMode)
        {
        case 0: // transforms
            morphTarget = EMotionFX::MorphTargetStandard::Create(true, false, actor, morphTargetActor.get(), morphTargetName.c_str());
            break;

        case 1: // mesh deforms
            morphTarget = EMotionFX::MorphTargetStandard::Create(false, true, actor, morphTargetActor.get(), morphTargetName.c_str());
            break;

        case 2: // both
            morphTarget = EMotionFX::MorphTargetStandard::Create(true, true, actor, morphTargetActor.get(), morphTargetName.c_str());
            break;

        default:
            MCore::LogError("Capture mode undefined for '%s'", morphTargetName.c_str());
            return false;
        }

        // set the attributes
        morphTarget->SetPhonemeSets((EMotionFX::MorphTarget::EPhonemeSet)phonemeSets);
        morphTarget->SetRangeMin(rangeMin);
        morphTarget->SetRangeMax(rangeMax);

        // add the morph target to the morph setup
        morphSetup->AddMorphTarget(morphTarget);

        return true;
    }
} // namespace ExporterLib
