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

// include the required headers
#include "StandardMaterial.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(StandardMaterialLayer, MaterialAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(StandardMaterial, MaterialAllocator, 0)

    // constructor
    StandardMaterialLayer::StandardMaterialLayer()
        : BaseObject()
    {
        mLayerTypeID    = LAYERTYPE_UNKNOWN;
        mFileNameID     = MCORE_INVALIDINDEX32;
        mBlendMode      = LAYERBLENDMODE_NONE;
        mAmount         = 1.0f;
        mUOffset        = 0.0f;
        mVOffset        = 0.0f;
        mUTiling        = 1.0f;
        mVTiling        = 1.0f;
        mRotationRadians = 0.0f;
    }


    // extra constructor
    StandardMaterialLayer::StandardMaterialLayer(uint32 layerType, const char* fileName, float amount)
        : BaseObject()
    {
        mLayerTypeID    = layerType;
        mAmount         = amount;
        mUOffset        = 0.0f;
        mVOffset        = 0.0f;
        mUTiling        = 1.0f;
        mVTiling        = 1.0f;
        mRotationRadians = 0.0f;
        mBlendMode      = LAYERBLENDMODE_NONE;

        // calculate the ID
        mFileNameID = MCore::GetStringIdPool().GenerateIdForString(fileName);
    }


    // destructor
    StandardMaterialLayer::~StandardMaterialLayer()
    {
    }


    // create
    StandardMaterialLayer* StandardMaterialLayer::Create()
    {
        return aznew StandardMaterialLayer();
    }


    // extended create
    StandardMaterialLayer* StandardMaterialLayer::Create(uint32 layerType, const char* fileName, float amount)
    {
        return aznew StandardMaterialLayer(layerType, fileName, amount);
    }


    // init from another layer
    void StandardMaterialLayer::InitFrom(StandardMaterialLayer* layer)
    {
        mLayerTypeID    = layer->mLayerTypeID;
        mFileNameID     = layer->mFileNameID;
        mBlendMode      = layer->mBlendMode;
        mAmount         = layer->mAmount;
        mUOffset        = layer->mUOffset;
        mVOffset        = layer->mVOffset;
        mUTiling        = layer->mUTiling;
        mVTiling        = layer->mVTiling;
        mRotationRadians = layer->mRotationRadians;
    }


    // return the layer type string
    const char* StandardMaterialLayer::GetTypeString() const
    {
        switch (mLayerTypeID)
        {
        case LAYERTYPE_UNKNOWN:
        {
            return "Unknown";
        }
        case LAYERTYPE_AMBIENT:
        {
            return "Ambient";
        }
        case LAYERTYPE_DIFFUSE:
        {
            return "Diffuse";
        }
        case LAYERTYPE_SPECULAR:
        {
            return "Specular";
        }
        case LAYERTYPE_OPACITY:
        {
            return "Opacity";
        }
        case LAYERTYPE_BUMP:
        {
            return "Bumpmap";
        }
        case LAYERTYPE_SELFILLUM:
        {
            return "Self Illumination";
        }
        case LAYERTYPE_SHINE:
        {
            return "Shininess";
        }
        case LAYERTYPE_SHINESTRENGTH:
        {
            return "Shine Strength";
        }
        case LAYERTYPE_FILTERCOLOR:
        {
            return "Filter Color";
        }
        case LAYERTYPE_REFLECT:
        {
            return "Reflection";
        }
        case LAYERTYPE_REFRACT:
        {
            return "Refraction";
        }
        case LAYERTYPE_ENVIRONMENT:
        {
            return "Environment Map";
        }
        case LAYERTYPE_DISPLACEMENTCOLOR:
        {
            return "Displacement Color Map";
        }
        case LAYERTYPE_DISPLACEMENTFACTOR:
        {
            return "Displacement Factor Map";
        }
        case LAYERTYPE_NORMALMAP:
        {
            return "Normal Map";
        }
        }

        return "Unknown";
    }


    // return the blend mode string
    const char* StandardMaterialLayer::GetBlendModeString() const
    {
        switch (mBlendMode)
        {
        case LAYERBLENDMODE_NONE:
        {
            return "None";
        }
        case LAYERBLENDMODE_OVER:
        {
            return "Over";
        }
        case LAYERBLENDMODE_IN:
        {
            return "In";
        }
        case LAYERBLENDMODE_OUT:
        {
            return "Out";
        }
        case LAYERBLENDMODE_ADD:
        {
            return "Add";
        }
        case LAYERBLENDMODE_SUBSTRACT:
        {
            return "Substract";
        }
        case LAYERBLENDMODE_MULTIPLY:
        {
            return "Multiply";
        }
        case LAYERBLENDMODE_DIFFERENCE:
        {
            return "Difference";
        }
        case LAYERBLENDMODE_LIGHTEN:
        {
            return "Lighten";
        }
        case LAYERBLENDMODE_DARKEN:
        {
            return "Darken";
        }
        case LAYERBLENDMODE_SATURATE:
        {
            return "Saturate";
        }
        case LAYERBLENDMODE_DESATURATE:
        {
            return "Desaturate";
        }
        case LAYERBLENDMODE_ILLUMINATE:
        {
            return "Illuminate";
        }
        }

        return "";
    }


    float StandardMaterialLayer::GetUOffset() const
    {
        return mUOffset;
    }


    float StandardMaterialLayer::GetVOffset() const
    {
        return mVOffset;
    }


    float StandardMaterialLayer::GetUTiling() const
    {
        return mUTiling;
    }


    float StandardMaterialLayer::GetVTiling() const
    {
        return mVTiling;
    }


    float StandardMaterialLayer::GetRotationRadians() const
    {
        return mRotationRadians;
    }


    void StandardMaterialLayer::SetUOffset(float uOffset)
    {
        mUOffset = uOffset;
    }


    void StandardMaterialLayer::SetVOffset(float vOffset)
    {
        mVOffset = vOffset;
    }


    void StandardMaterialLayer::SetUTiling(float uTiling)
    {
        mUTiling = uTiling;
    }


    void StandardMaterialLayer::SetVTiling(float vTiling)
    {
        mVTiling = vTiling;
    }


    void StandardMaterialLayer::SetRotationRadians(float rotationRadians)
    {
        mRotationRadians = rotationRadians;
    }


    const char* StandardMaterialLayer::GetFileName() const
    {
        return MCore::GetStringIdPool().GetName(mFileNameID).c_str();
    }


    const AZStd::string& StandardMaterialLayer::GetFileNameString() const
    {
        return MCore::GetStringIdPool().GetName(mFileNameID);
    }


    void StandardMaterialLayer::SetFileName(const char* fileName)
    {
        // calculate the new ID
        mFileNameID = MCore::GetStringIdPool().GenerateIdForString(fileName);
    }


    void StandardMaterialLayer::SetAmount(float amount)
    {
        mAmount = amount;
    }


    float StandardMaterialLayer::GetAmount() const
    {
        return mAmount;
    }


    uint32 StandardMaterialLayer::GetType() const
    {
        return mLayerTypeID;
    }


    void StandardMaterialLayer::SetType(uint32 typeID)
    {
        mLayerTypeID = typeID;
    }


    void StandardMaterialLayer::SetBlendMode(unsigned char layerBlendMode)
    {
        mBlendMode = layerBlendMode;
    }


    unsigned char StandardMaterialLayer::GetBlendMode() const
    {
        return mBlendMode;
    }


    //-------------------------------------------------------------------------------------------------
    // StandardMaterial
    //-------------------------------------------------------------------------------------------------

    // constructor
    StandardMaterial::StandardMaterial(const char* name)
        : Material(name)
    {
        mAmbient        = MCore::RGBAColor(0.2f, 0.2f, 0.2f);
        mDiffuse        = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        mSpecular       = MCore::RGBAColor(1.0f, 1.0f, 1.0f);
        mEmissive       = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        mShine          = 100.0f;
        mShineStrength  = 1.0f;
        mOpacity        = 1.0f;
        mIOR            = 1.5f;
        mDoubleSided    = true;
        mWireFrame      = false;

        mLayers.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MATERIALS);
    }


    // destructor
    StandardMaterial::~StandardMaterial()
    {
        // get rid of all material layers
        RemoveAllLayers();
    }


    // create
    StandardMaterial* StandardMaterial::Create(const char* name)
    {
        return aznew StandardMaterial(name);
    }


    // clone the standard material
    Material* StandardMaterial::Clone() const
    {
        // create the new material
        Material* clone = aznew StandardMaterial(GetName());

        // type-cast the standard material since it is stored as material base in the smart pointer
        StandardMaterial* standardMaterial = static_cast<StandardMaterial*>(clone);

        // copy the attributes
        standardMaterial->mAmbient          = mAmbient;
        standardMaterial->mDiffuse          = mDiffuse;
        standardMaterial->mSpecular         = mSpecular;
        standardMaterial->mEmissive         = mEmissive;
        standardMaterial->mShine            = mShine;
        standardMaterial->mShineStrength    = mShineStrength;
        standardMaterial->mOpacity          = mOpacity;
        standardMaterial->mIOR              = mIOR;
        standardMaterial->mDoubleSided      = mDoubleSided;
        standardMaterial->mWireFrame        = mWireFrame;

        // copy the layers
        const uint32 numLayers = mLayers.GetLength();
        standardMaterial->mLayers.Resize(numLayers);
        for (uint32 i = 0; i < numLayers; ++i)
        {
            standardMaterial->mLayers[i] = StandardMaterialLayer::Create();
            standardMaterial->mLayers[i]->InitFrom(mLayers[i]);
        }

        // return the result
        return clone;
    }


    // remove a given layer
    void StandardMaterial::RemoveLayer(StandardMaterialLayer* layer, bool delFromMem)
    {
        if (layer)
        {
            if (delFromMem)
            {
                layer->Destroy();
            }
            mLayers.RemoveByValue(layer);
        }
    }


    void StandardMaterial::SetAmbient(const MCore::RGBAColor& ambient)
    {
        mAmbient = ambient;
    }


    void StandardMaterial::SetDiffuse(const MCore::RGBAColor& diffuse)
    {
        mDiffuse = diffuse;
    }


    void StandardMaterial::SetSpecular(const MCore::RGBAColor& specular)
    {
        mSpecular = specular;
    }


    void StandardMaterial::SetEmissive(const MCore::RGBAColor& emissive)
    {
        mEmissive = emissive;
    }


    void StandardMaterial::SetShine(float shine)
    {
        mShine = shine;
    }


    void StandardMaterial::SetShineStrength(float shineStrength)
    {
        mShineStrength = shineStrength;
    }


    void StandardMaterial::SetOpacity(float opacity)
    {
        mOpacity = opacity;
    }


    void StandardMaterial::SetIOR(float ior)
    {
        mIOR = ior;
    }


    void StandardMaterial::SetDoubleSided(bool doubleSided)
    {
        mDoubleSided = doubleSided;
    }


    void StandardMaterial::SetWireFrame(bool wireFrame)
    {
        mWireFrame = wireFrame;
    }


    const MCore::RGBAColor& StandardMaterial::GetAmbient() const
    {
        return mAmbient;
    }


    const MCore::RGBAColor& StandardMaterial::GetDiffuse() const
    {
        return mDiffuse;
    }


    const MCore::RGBAColor& StandardMaterial::GetSpecular() const
    {
        return mSpecular;
    }


    const MCore::RGBAColor& StandardMaterial::GetEmissive() const
    {
        return mEmissive;
    }


    float StandardMaterial::GetShine() const
    {
        return mShine;
    }


    float StandardMaterial::GetShineStrength() const
    {
        return mShineStrength;
    }


    float StandardMaterial::GetOpacity() const
    {
        return mOpacity;
    }


    float StandardMaterial::GetIOR() const
    {
        return mIOR;
    }


    bool StandardMaterial::GetDoubleSided() const
    {
        return mDoubleSided;
    }


    bool StandardMaterial::GetWireFrame() const
    {
        return mWireFrame;
    }


    StandardMaterialLayer* StandardMaterial::AddLayer(StandardMaterialLayer* layer)
    {
        mLayers.Add(layer);
        return layer;
    }


    uint32 StandardMaterial::GetNumLayers() const
    {
        return mLayers.GetLength();
    }


    StandardMaterialLayer* StandardMaterial::GetLayer(uint32 nr)
    {
        MCORE_ASSERT(nr < mLayers.GetLength());
        return mLayers[nr];
    }


    void StandardMaterial::RemoveLayer(uint32 nr, bool delFromMem)
    {
        MCORE_ASSERT(nr < mLayers.GetLength());
        if (delFromMem)
        {
            mLayers[nr]->Destroy();
        }

        mLayers.Remove(nr);
    }


    void StandardMaterial::RemoveAllLayers()
    {
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            mLayers[i]->Destroy();
        }

        mLayers.Clear();
    }


    uint32 StandardMaterial::FindLayer(uint32 layerType) const
    {
        // search through all layers
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mLayers[i]->GetType() == layerType)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void StandardMaterial::ReserveLayers(uint32 numLayers)
    {
        mLayers.Reserve(numLayers);
    }
} // namespace EMotionFX
