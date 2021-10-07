/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        m_layerTypeId    = LAYERTYPE_UNKNOWN;
        m_fileNameId     = MCORE_INVALIDINDEX32;
        m_blendMode      = LAYERBLENDMODE_NONE;
        m_amount         = 1.0f;
        m_uOffset        = 0.0f;
        m_vOffset        = 0.0f;
        m_uTiling        = 1.0f;
        m_vTiling        = 1.0f;
        m_rotationRadians = 0.0f;
    }


    // extra constructor
    StandardMaterialLayer::StandardMaterialLayer(uint32 layerType, const char* fileName, float amount)
        : BaseObject()
    {
        m_layerTypeId    = layerType;
        m_amount         = amount;
        m_uOffset        = 0.0f;
        m_vOffset        = 0.0f;
        m_uTiling        = 1.0f;
        m_vTiling        = 1.0f;
        m_rotationRadians = 0.0f;
        m_blendMode      = LAYERBLENDMODE_NONE;

        // calculate the ID
        m_fileNameId = MCore::GetStringIdPool().GenerateIdForString(fileName);
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
        m_layerTypeId    = layer->m_layerTypeId;
        m_fileNameId     = layer->m_fileNameId;
        m_blendMode      = layer->m_blendMode;
        m_amount         = layer->m_amount;
        m_uOffset        = layer->m_uOffset;
        m_vOffset        = layer->m_vOffset;
        m_uTiling        = layer->m_uTiling;
        m_vTiling        = layer->m_vTiling;
        m_rotationRadians = layer->m_rotationRadians;
    }


    // return the layer type string
    const char* StandardMaterialLayer::GetTypeString() const
    {
        switch (m_layerTypeId)
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
        switch (m_blendMode)
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
        return m_uOffset;
    }


    float StandardMaterialLayer::GetVOffset() const
    {
        return m_vOffset;
    }


    float StandardMaterialLayer::GetUTiling() const
    {
        return m_uTiling;
    }


    float StandardMaterialLayer::GetVTiling() const
    {
        return m_vTiling;
    }


    float StandardMaterialLayer::GetRotationRadians() const
    {
        return m_rotationRadians;
    }


    void StandardMaterialLayer::SetUOffset(float uOffset)
    {
        m_uOffset = uOffset;
    }


    void StandardMaterialLayer::SetVOffset(float vOffset)
    {
        m_vOffset = vOffset;
    }


    void StandardMaterialLayer::SetUTiling(float uTiling)
    {
        m_uTiling = uTiling;
    }


    void StandardMaterialLayer::SetVTiling(float vTiling)
    {
        m_vTiling = vTiling;
    }


    void StandardMaterialLayer::SetRotationRadians(float rotationRadians)
    {
        m_rotationRadians = rotationRadians;
    }


    const char* StandardMaterialLayer::GetFileName() const
    {
        return MCore::GetStringIdPool().GetName(m_fileNameId).c_str();
    }


    const AZStd::string& StandardMaterialLayer::GetFileNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_fileNameId);
    }


    void StandardMaterialLayer::SetFileName(const char* fileName)
    {
        // calculate the new ID
        m_fileNameId = MCore::GetStringIdPool().GenerateIdForString(fileName);
    }


    void StandardMaterialLayer::SetAmount(float amount)
    {
        m_amount = amount;
    }


    float StandardMaterialLayer::GetAmount() const
    {
        return m_amount;
    }


    uint32 StandardMaterialLayer::GetType() const
    {
        return m_layerTypeId;
    }


    void StandardMaterialLayer::SetType(uint32 typeID)
    {
        m_layerTypeId = typeID;
    }


    void StandardMaterialLayer::SetBlendMode(unsigned char layerBlendMode)
    {
        m_blendMode = layerBlendMode;
    }


    unsigned char StandardMaterialLayer::GetBlendMode() const
    {
        return m_blendMode;
    }


    //-------------------------------------------------------------------------------------------------
    // StandardMaterial
    //-------------------------------------------------------------------------------------------------

    // constructor
    StandardMaterial::StandardMaterial(const char* name)
        : Material(name)
    {
        m_ambient        = MCore::RGBAColor(0.2f, 0.2f, 0.2f);
        m_diffuse        = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        m_specular       = MCore::RGBAColor(1.0f, 1.0f, 1.0f);
        m_emissive       = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        m_shine          = 100.0f;
        m_shineStrength  = 1.0f;
        m_opacity        = 1.0f;
        m_ior            = 1.5f;
        m_doubleSided    = true;
        m_wireFrame      = false;
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
        standardMaterial->m_ambient          = m_ambient;
        standardMaterial->m_diffuse          = m_diffuse;
        standardMaterial->m_specular         = m_specular;
        standardMaterial->m_emissive         = m_emissive;
        standardMaterial->m_shine            = m_shine;
        standardMaterial->m_shineStrength    = m_shineStrength;
        standardMaterial->m_opacity          = m_opacity;
        standardMaterial->m_ior              = m_ior;
        standardMaterial->m_doubleSided      = m_doubleSided;
        standardMaterial->m_wireFrame        = m_wireFrame;

        // copy the layers
        const size_t numLayers = m_layers.size();
        standardMaterial->m_layers.resize(numLayers);
        for (size_t i = 0; i < numLayers; ++i)
        {
            standardMaterial->m_layers[i] = StandardMaterialLayer::Create();
            standardMaterial->m_layers[i]->InitFrom(m_layers[i]);
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
            if (const auto it = AZStd::find(begin(m_layers), end(m_layers), layer); it != end(m_layers))
            {
                m_layers.erase(it);
            }
        }
    }


    void StandardMaterial::SetAmbient(const MCore::RGBAColor& ambient)
    {
        m_ambient = ambient;
    }


    void StandardMaterial::SetDiffuse(const MCore::RGBAColor& diffuse)
    {
        m_diffuse = diffuse;
    }


    void StandardMaterial::SetSpecular(const MCore::RGBAColor& specular)
    {
        m_specular = specular;
    }


    void StandardMaterial::SetEmissive(const MCore::RGBAColor& emissive)
    {
        m_emissive = emissive;
    }


    void StandardMaterial::SetShine(float shine)
    {
        m_shine = shine;
    }


    void StandardMaterial::SetShineStrength(float shineStrength)
    {
        m_shineStrength = shineStrength;
    }


    void StandardMaterial::SetOpacity(float opacity)
    {
        m_opacity = opacity;
    }


    void StandardMaterial::SetIOR(float ior)
    {
        m_ior = ior;
    }


    void StandardMaterial::SetDoubleSided(bool doubleSided)
    {
        m_doubleSided = doubleSided;
    }


    void StandardMaterial::SetWireFrame(bool wireFrame)
    {
        m_wireFrame = wireFrame;
    }


    const MCore::RGBAColor& StandardMaterial::GetAmbient() const
    {
        return m_ambient;
    }


    const MCore::RGBAColor& StandardMaterial::GetDiffuse() const
    {
        return m_diffuse;
    }


    const MCore::RGBAColor& StandardMaterial::GetSpecular() const
    {
        return m_specular;
    }


    const MCore::RGBAColor& StandardMaterial::GetEmissive() const
    {
        return m_emissive;
    }


    float StandardMaterial::GetShine() const
    {
        return m_shine;
    }


    float StandardMaterial::GetShineStrength() const
    {
        return m_shineStrength;
    }


    float StandardMaterial::GetOpacity() const
    {
        return m_opacity;
    }


    float StandardMaterial::GetIOR() const
    {
        return m_ior;
    }


    bool StandardMaterial::GetDoubleSided() const
    {
        return m_doubleSided;
    }


    bool StandardMaterial::GetWireFrame() const
    {
        return m_wireFrame;
    }


    StandardMaterialLayer* StandardMaterial::AddLayer(StandardMaterialLayer* layer)
    {
        m_layers.emplace_back(layer);
        return layer;
    }


    size_t StandardMaterial::GetNumLayers() const
    {
        return m_layers.size();
    }


    StandardMaterialLayer* StandardMaterial::GetLayer(size_t nr)
    {
        MCORE_ASSERT(nr < m_layers.size());
        return m_layers[nr];
    }


    void StandardMaterial::RemoveLayer(size_t nr, bool delFromMem)
    {
        MCORE_ASSERT(nr < m_layers.size());
        if (delFromMem)
        {
            m_layers[nr]->Destroy();
        }

        m_layers.erase(AZStd::next(begin(m_layers), nr));
    }


    void StandardMaterial::RemoveAllLayers()
    {
        for (StandardMaterialLayer* layer : m_layers)
        {
            layer->Destroy();
        }

        m_layers.clear();
    }


    size_t StandardMaterial::FindLayer(uint32 layerType) const
    {
        // search through all layers
        const auto foundLayer = AZStd::find_if(begin(m_layers), end(m_layers), [layerType](const StandardMaterialLayer* layer)
        {
            return layer->GetType() == layerType;
        });
        return foundLayer != end(m_layers) ? AZStd::distance(begin(m_layers), foundLayer) : InvalidIndex;
    }


    void StandardMaterial::ReserveLayers(size_t numLayers)
    {
        m_layers.reserve(numLayers);
    }
} // namespace EMotionFX
