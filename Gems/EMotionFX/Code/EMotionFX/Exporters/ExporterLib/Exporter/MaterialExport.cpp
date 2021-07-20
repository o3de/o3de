/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
#include <EMotionFX/Source/StandardMaterial.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>

#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/LogManager.h>


namespace ExporterLib
{
    // write the material attribute set
    void SaveMaterialAttributeSet(MCore::Stream* file, EMotionFX::Material* material, uint32 lodLevel, uint32 materialNumber, MCore::Endian::EEndianType targetEndianType)
    {
        AZ_UNUSED(material);
        // write the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_MATERIALATTRIBUTESET;
        chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_MaterialAttributeSet);
        chunkHeader.mVersion        = 1;
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // write the attribute set info header
        EMotionFX::FileFormat::Actor_MaterialAttributeSet setInfo;
        setInfo.mMaterialIndex      = materialNumber;
        setInfo.mLODLevel           = lodLevel;
        ConvertUnsignedInt(&setInfo.mMaterialIndex, targetEndianType);
        ConvertUnsignedInt(&setInfo.mLODLevel, targetEndianType);
        file->Write(&setInfo, sizeof(EMotionFX::FileFormat::Actor_MaterialAttributeSet));

        // Write a former empty attribute set.
        uint8 version = 1;
        file->Write(&version, sizeof(uint8));

        uint32 numAttributes = 0;
        ConvertUnsignedInt(&numAttributes, targetEndianType);
        file->Write(&numAttributes, sizeof(uint32));
    }


    // save the given material for the given LOD level
    void SaveMaterial(MCore::Stream* file, EMotionFX::Material* material, uint32 lodLevel, uint32 materialNumber, MCore::Endian::EEndianType targetEndianType)
    {
        //----------------------------------------
        // Generic EMotionFX::Material
        //----------------------------------------
        if (material->GetType() == EMotionFX::Material::TYPE_ID)
        {
            // chunk header
            EMotionFX::FileFormat::FileChunk chunkHeader;
            chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_GENERICMATERIAL;
            chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_GenericMaterial) + GetStringChunkSize(material->GetName());
            chunkHeader.mVersion        = 1;

            EMotionFX::FileFormat::Actor_GenericMaterial materialChunk;
            materialChunk.mLOD  = lodLevel;

            ConvertFileChunk(&chunkHeader, targetEndianType);
            ConvertUnsignedInt(&materialChunk.mLOD, targetEndianType);

            // write header and material
            file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
            file->Write(&materialChunk, sizeof(EMotionFX::FileFormat::Actor_GenericMaterial));

            // followed by:
            SaveString(material->GetName(), file, targetEndianType);

            MCore::LogDetailedInfo("- Generic material:");
            MCore::LogDetailedInfo("    + Name: '%s'", material->GetName());
            MCore::LogDetailedInfo("    + LOD: %d", lodLevel);
        }

        //----------------------------------------
        // Standard EMotionFX::Material
        //----------------------------------------
        if (material->GetType() == EMotionFX::StandardMaterial::TYPE_ID)
        {
            // typecast to a standard material
            EMotionFX::StandardMaterial* standardMaterial = (EMotionFX::StandardMaterial*)material;

            // chunk header
            EMotionFX::FileFormat::FileChunk chunkHeader;
            chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_STDMATERIAL;
            chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_StandardMaterial) + GetStringChunkSize(standardMaterial->GetName());
            chunkHeader.mVersion        = 1;

            const uint32 numLayers = standardMaterial->GetNumLayers();
            for (uint32 i = 0; i < numLayers; ++i)
            {
                chunkHeader.mSizeInBytes += sizeof(EMotionFX::FileFormat::Actor_StandardMaterialLayer);
                chunkHeader.mSizeInBytes += GetStringChunkSize(standardMaterial->GetLayer(i)->GetFileName());
            }

            EMotionFX::FileFormat::Actor_StandardMaterial materialChunk;
            CopyColor(standardMaterial->GetAmbient(),  materialChunk.mAmbient);
            CopyColor(standardMaterial->GetDiffuse(),  materialChunk.mDiffuse);
            CopyColor(standardMaterial->GetSpecular(), materialChunk.mSpecular);
            CopyColor(standardMaterial->GetEmissive(), materialChunk.mEmissive);
            materialChunk.mDoubleSided      = standardMaterial->GetDoubleSided();
            materialChunk.mIOR              = standardMaterial->GetIOR();
            materialChunk.mOpacity          = standardMaterial->GetOpacity();
            materialChunk.mShine            = standardMaterial->GetShine();
            materialChunk.mShineStrength    = standardMaterial->GetShineStrength();
            materialChunk.mTransparencyType = 'F';//standardMaterial->GetTransparencyType();
            materialChunk.mWireFrame        = standardMaterial->GetWireFrame();
            materialChunk.mNumLayers        = static_cast<uint8>(standardMaterial->GetNumLayers());
            materialChunk.mLOD              = lodLevel;

            // add it to the log file
            MCore::LogDetailedInfo("- Standard material:");
            MCore::LogDetailedInfo("    + Name: '%s'", standardMaterial->GetName());
            MCore::LogDetailedInfo("    + LOD: %d", lodLevel);
            MCore::LogDetailedInfo("    + Ambient:  r=%f g=%f b=%f", materialChunk.mAmbient.mR, materialChunk.mAmbient.mG, materialChunk.mAmbient.mB);
            MCore::LogDetailedInfo("    + Diffuse:  r=%f g=%f b=%f", materialChunk.mDiffuse.mR, materialChunk.mDiffuse.mG, materialChunk.mDiffuse.mB);
            MCore::LogDetailedInfo("    + Specular: r=%f g=%f b=%f", materialChunk.mSpecular.mR, materialChunk.mSpecular.mG, materialChunk.mSpecular.mB);
            MCore::LogDetailedInfo("    + Emissive: r=%f g=%f b=%f", materialChunk.mEmissive.mR, materialChunk.mEmissive.mG, materialChunk.mEmissive.mB);
            MCore::LogDetailedInfo("    + Shine: %f", materialChunk.mShine);
            MCore::LogDetailedInfo("    + ShineStrength: %f", materialChunk.mShineStrength);
            MCore::LogDetailedInfo("    + Opacity: %f", materialChunk.mOpacity);
            MCore::LogDetailedInfo("    + IndexOfRefraction: %f", materialChunk.mIOR);
            MCore::LogDetailedInfo("    + DoubleSided: %i", (int)materialChunk.mDoubleSided);
            MCore::LogDetailedInfo("    + WireFrame: %i", (int)materialChunk.mWireFrame);
            MCore::LogDetailedInfo("    + TransparencyType: %c", (char)materialChunk.mTransparencyType);
            MCore::LogDetailedInfo("    + NumLayers: %i", materialChunk.mNumLayers);

            // endian conversion
            ConvertFileChunk(&chunkHeader, targetEndianType);
            ConvertFileColor(&materialChunk.mAmbient, targetEndianType);
            ConvertFileColor(&materialChunk.mDiffuse, targetEndianType);
            ConvertFileColor(&materialChunk.mSpecular, targetEndianType);
            ConvertFileColor(&materialChunk.mEmissive, targetEndianType);
            ConvertFloat(&materialChunk.mIOR, targetEndianType);
            ConvertFloat(&materialChunk.mOpacity, targetEndianType);
            ConvertFloat(&materialChunk.mShine, targetEndianType);
            ConvertFloat(&materialChunk.mShineStrength, targetEndianType);
            ConvertUnsignedInt(&materialChunk.mLOD, targetEndianType);

            // write header and material
            file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
            file->Write(&materialChunk, sizeof(EMotionFX::FileFormat::Actor_StandardMaterial));

            // followed by:
            SaveString(standardMaterial->GetName(), file, targetEndianType);

            // save all material layers
            for (uint32 i = 0; i < numLayers; ++i)
            {
                // get the layer
                EMotionFX::StandardMaterialLayer* layer = standardMaterial->GetLayer(i);

                EMotionFX::FileFormat::Actor_StandardMaterialLayer materialChunkLayer;
                materialChunkLayer.mAmount          = layer->GetAmount();
                materialChunkLayer.mMapType         = static_cast<uint8>(layer->GetType());
                materialChunkLayer.mMaterialNumber  = (uint16)materialNumber;
                materialChunkLayer.mRotationRadians = layer->GetRotationRadians();
                materialChunkLayer.mUOffset         = layer->GetUOffset();
                materialChunkLayer.mVOffset         = layer->GetVOffset();
                materialChunkLayer.mUTiling         = layer->GetUTiling();
                materialChunkLayer.mVTiling         = layer->GetVTiling();
                materialChunkLayer.mBlendMode       = layer->GetBlendMode();

                // add to log file
                MCore::LogDetailedInfo("    - Material layer #%d:", i);
                MCore::LogDetailedInfo("       + Name: '%s' (MatNr=%i)", layer->GetFileName(), materialNumber);
                MCore::LogDetailedInfo("       + Amount: %f", materialChunkLayer.mAmount);
                MCore::LogDetailedInfo("       + Type: %i", (int)materialChunkLayer.mMapType);
                MCore::LogDetailedInfo("       + BlendMode: %i", (int)materialChunkLayer.mBlendMode);
                MCore::LogDetailedInfo("       + MaterialNumber: %i", (uint32)materialChunkLayer.mMaterialNumber);
                MCore::LogDetailedInfo("       + UOffset: %f", materialChunkLayer.mUOffset);
                MCore::LogDetailedInfo("       + VOffset: %f", materialChunkLayer.mVOffset);
                MCore::LogDetailedInfo("       + UTiling: %f", materialChunkLayer.mUTiling);
                MCore::LogDetailedInfo("       + VTiling: %f", materialChunkLayer.mVTiling);
                MCore::LogDetailedInfo("       + RotationRadians: %f", materialChunkLayer.mRotationRadians);

                // endian conversion
                ConvertFloat(&materialChunkLayer.mAmount, targetEndianType);
                ConvertUnsignedShort(&materialChunkLayer.mMaterialNumber, targetEndianType);
                ConvertFloat(&materialChunkLayer.mRotationRadians, targetEndianType);
                ConvertFloat(&materialChunkLayer.mUOffset, targetEndianType);
                ConvertFloat(&materialChunkLayer.mVOffset, targetEndianType);
                ConvertFloat(&materialChunkLayer.mUTiling, targetEndianType);
                ConvertFloat(&materialChunkLayer.mVTiling, targetEndianType);

                // write header and material layer
                file->Write(&materialChunkLayer, sizeof(EMotionFX::FileFormat::Actor_StandardMaterialLayer));
                SaveString(layer->GetFileName(), file, targetEndianType);
            }
        }
    }


    // save the given materials
    void SaveMaterials(MCore::Stream* file, MCore::Array<EMotionFX::Material*>& materials, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of materials
        const uint32 numMaterials = materials.GetLength();

        // chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_MATERIALINFO;
        chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_MaterialInfo);
        chunkHeader.mVersion        = 1;

        // convert endian and write to file
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        EMotionFX::FileFormat::Actor_MaterialInfo materialInfoChunk;
        materialInfoChunk.mLOD                      = lodLevel;
        materialInfoChunk.mNumTotalMaterials        = numMaterials;
        materialInfoChunk.mNumStandardMaterials     = 0;
        materialInfoChunk.mNumFXMaterials           = 0;
        materialInfoChunk.mNumGenericMaterials      = 0;

        for (uint32 i = 0; i < numMaterials; i++)
        {
            if (materials[i]->GetType() == EMotionFX::Material::TYPE_ID)
            {
                materialInfoChunk.mNumGenericMaterials++;
            }

            if (materials[i]->GetType() == EMotionFX::StandardMaterial::TYPE_ID)
            {
                materialInfoChunk.mNumStandardMaterials++;
            }
        }

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Materials (%d)", numMaterials);
        MCore::LogDetailedInfo("============================================================");

        MCORE_ASSERT(materialInfoChunk.mNumTotalMaterials == materialInfoChunk.mNumStandardMaterials + materialInfoChunk.mNumFXMaterials + materialInfoChunk.mNumGenericMaterials);

        // convert endian and write to disk
        ConvertUnsignedInt(&materialInfoChunk.mNumTotalMaterials, targetEndianType);
        ConvertUnsignedInt(&materialInfoChunk.mNumStandardMaterials, targetEndianType);
        ConvertUnsignedInt(&materialInfoChunk.mNumFXMaterials, targetEndianType);
        ConvertUnsignedInt(&materialInfoChunk.mNumGenericMaterials, targetEndianType);
        ConvertUnsignedInt(&materialInfoChunk.mLOD, targetEndianType);
        file->Write(&materialInfoChunk, sizeof(EMotionFX::FileFormat::Actor_MaterialInfo));

        // export all materials
        for (uint32 i = 0; i < numMaterials; i++)
        {
            SaveMaterial(file, materials[i], lodLevel, i, targetEndianType);
        }

        // save all material attribute sets
        for (uint32 i = 0; i < numMaterials; i++)
        {
            SaveMaterialAttributeSet(file, materials[i], lodLevel, i, targetEndianType);
        }
    }


    // save out all materials for a given LOD level
    void SaveMaterials(MCore::Stream* file, EMotionFX::Actor* actor, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of materials in the given lod level
        const uint32 numMaterials = actor->GetNumMaterials(lodLevel);

        // create our materials array and reserve some elements
        MCore::Array<EMotionFX::Material*> materials;
        materials.Reserve(numMaterials);

        // iterate through the materials
        for (uint32 j = 0; j < numMaterials; j++)
        {
            // get the base material
            EMotionFX::Material* baseMaterial = actor->GetMaterial(lodLevel, j);
            materials.Add(baseMaterial);
        }

        // save the materials
        SaveMaterials(file, materials, lodLevel, targetEndianType);
    }


    // save all materials for all LOD levels
    void SaveMaterials(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of LOD levels and iterate through them
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            SaveMaterials(file, actor, i, targetEndianType);
        }
    }
} // namespace ExporterLib
