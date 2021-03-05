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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>

struct IMaterial;
class ITexture;

namespace Terrain
{
    struct MacroMaterial
    {
        // Textures
        _smart_ptr<ITexture> m_macroColorMap = nullptr;
        _smart_ptr<ITexture> m_macroGlossMap = nullptr;
        _smart_ptr<ITexture> m_macroNormalMap = nullptr;

        // Material Params
        AZ::Color m_macroColorMapColor;
        float m_macroGlossMapScale = 1.0f;
        float m_macroNormalMapScale = 1.0f;
        float m_macroSpecReflectance = 0.03f;

        void Clear()
        {
            m_macroColorMap = nullptr;
            m_macroGlossMap = nullptr;
            m_macroNormalMap = nullptr;

            m_macroColorMapColor = AZ::Color(1.0f);
            m_macroGlossMapScale = 1.0f;
            m_macroNormalMapScale = 1.0f;
            m_macroSpecReflectance = 0.03f;
        }
    };

    struct TerrainMaterialLayer
    {
        _smart_ptr<IMaterial> m_material = nullptr;
        _smart_ptr<ITexture> m_splatTexture = nullptr;

        TerrainMaterialLayer(_smart_ptr<IMaterial> material, _smart_ptr<ITexture> splatTexture)
            : m_material(material)
            , m_splatTexture(splatTexture)
        {
        }
    };

    struct RegionMaterials
    {
        MacroMaterial m_macroMaterial;

        AZStd::vector<TerrainMaterialLayer> m_materialLayers;
        _smart_ptr<IMaterial> m_defaultMaterial = nullptr;

        RegionMaterials& operator=(const RegionMaterials& rhs) = default;

        void Clear()
        {
            m_materialLayers.clear();
            m_defaultMaterial = nullptr;
            m_macroMaterial.Clear();
        }
    };

    const AZ::u32 kMaxRegionsPerTerrainMaterialRequest = 16;
    typedef AZStd::pair<int, int> RegionIndex;
    typedef AZStd::fixed_vector<RegionIndex, kMaxRegionsPerTerrainMaterialRequest> RegionIndexVector;
    typedef AZStd::fixed_vector<RegionMaterials, kMaxRegionsPerTerrainMaterialRequest> RegionMaterialVector;

    enum class RequestResult
    {
        NoAssetsForRegion,
        Loading,
        Success
    };

    class WorldMaterialRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void LoadWorld(const AZStd::string& worldName, int regionSize) = 0;

        virtual RequestResult RequestRegionMaterials(const RegionIndexVector& regions, RegionMaterialVector& outRegionMaterials) = 0;

        // Parameters:
        //  int tileX, tileY : Region tile indices
        // Returns:
        //  bool : If true, region material data is loaded and exists. outMacroMaterial will be modified with the respective macro material data
        //         If false, no region material data has been loaded or exists for the given tile (may still have invalid material layers).
        virtual RequestResult GetMacroMaterial(int tileX, int tileY, MacroMaterial& outMacroMaterial) = 0;

        virtual void GetTerrainPOMParameters(float& pomHeightBias, float& pomDisplacement, float& selfShadowStrength) = 0;

        //Get the surface type at a given position. If not loaded yet, returns "loadingMaterial".
        virtual AZStd::string_view GetSurfaceTypeAtPosition(AZ::Vector2 position) = 0;
    };
    using WorldMaterialRequestBus = AZ::EBus<WorldMaterialRequests>;
} // namespace Terrain

