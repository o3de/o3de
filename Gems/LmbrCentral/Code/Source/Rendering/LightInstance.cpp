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
#include "LmbrCentral_precompiled.h"

#include <AzCore/Math/Quaternion.h>
#include <MathConversion.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <I3DEngine.h>

#include "LightInstance.h"
#include "LightComponent.h"
#include "LensFlareComponent.h"

namespace
{
    const float kDefaultLightFrustumAngle = 45.f;
    const uint8 kDefaultLensOpticsFrustumAngle = 255;

    void UpdateLightFlag(bool enable, int mask, unsigned int& flags)
    {
        if (enable)
        {
            flags |= mask;
        }
        else
        {
            flags &= ~mask;
        }
    }

    AZ::u32 CreateLightId(AZ::EntityId entityId)
    {
        // Renderer requires a non-zero value, but doesn't care what it actually means.
        // This is used only for debug coloring and tie-breaking sorts.
        // Use the timestamp half of our entity Id.
        const AZ::u32 lowerEntityId = static_cast<AZ::u32>(static_cast<AZ::u64>(entityId) >> 32);
        return lowerEntityId;
    }

    const char* GetEntityName(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
        return (entity ? entity->GetName().c_str() : "<unknown>");
    }

    void LightConfigToLightParams(const LmbrCentral::LightConfiguration& configuration, AZ::EntityId entityId, int configSpec, CDLight& lightParams)
    {
        // The light cannot be ambient if it is an environment probe
        bool ambient = configuration.m_ambient;
        if (configuration.m_lightType == LmbrCentral::LightConfiguration::LightType::Probe)
        {
            ambient = false;
        }

        lightParams.SetPosition(Vec3_Zero);
        lightParams.m_fLightFrustumAngle = kDefaultLightFrustumAngle;
        lightParams.m_Flags = 0;
        lightParams.m_LensOpticsFrustumAngle = kDefaultLensOpticsFrustumAngle;
        Vec3 color = AZColorToLYVec3(configuration.m_color);
        color *= configuration.m_diffuseMultiplier;
        lightParams.SetLightColor(ColorF(color.x, color.y, color.z, 1.f));
        lightParams.SetSpecularMult(configuration.m_specMultiplier);

        lightParams.m_nLightStyle = configuration.m_animIndex; // "light style" is really a light animation curve index
        lightParams.SetAnimSpeed(configuration.m_animSpeed); // maps [0, 4] to [0, 255]
        lightParams.m_nLightPhase = static_cast<AZ::u8>(configuration.m_animPhase * 255.f); // maps [0, 1] to [0, 255]

        lightParams.m_nEntityId = CreateLightId(entityId);

        UpdateLightFlag(!configuration.m_useVisAreas, DLF_IGNORES_VISAREAS, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_indoorOnly, DLF_INDOOR_ONLY, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_affectsThisAreaOnly, DLF_THIS_AREA_ONLY, lightParams.m_Flags);
        UpdateLightFlag(ambient, DLF_AMBIENT, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_deferred, DLF_DEFERRED_LIGHT, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_volumetricFog, DLF_VOLUMETRIC_FOG, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_volumetricFogOnly, DLF_VOLUMETRIC_FOG_ONLY, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_castTerrainShadows, DLF_CAST_TERRAIN_SHADOWS, lightParams.m_Flags);

        if (static_cast<AZ::u32>(configSpec) >= static_cast<AZ::u32>(configuration.m_castShadowsSpec))
        {
            lightParams.m_Flags |= DLF_CASTSHADOW_MAPS;
        }

        //Probe won't cast shadow. Enable this flag wrongly for probe could lead to wrong bbox. 
        if (configuration.m_lightType == LmbrCentral::LightConfiguration::LightType::Probe)
        {
            lightParams.m_Flags &= ~DLF_CASTSHADOW_MAPS;
        }

        lightParams.m_fShadowBias = configuration.m_shadowBias;
        lightParams.m_fShadowSlopeBias = configuration.m_shadowSlopeBias;
        lightParams.m_fShadowResolutionScale = configuration.m_shadowResScale;
        lightParams.m_fShadowUpdateMinRadius = configuration.m_shadowUpdateMinRadius;
        lightParams.m_nShadowUpdateRatio = max((uint16)1, (uint16)(configuration.m_shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));

        switch (configuration.m_lightType)
        {
        case LmbrCentral::LightConfiguration::LightType::Point:
        {
            lightParams.m_fRadius = configuration.m_pointMaxDistance;
            lightParams.m_fAttenuationBulbSize = configuration.m_pointAttenuationBulbSize;
            lightParams.m_Flags |= DLF_POINT;
        }
        break;

        case LmbrCentral::LightConfiguration::LightType::Area:
        {
            lightParams.m_fRadius = configuration.m_areaMaxDistance;
            lightParams.m_fAreaWidth = configuration.m_areaWidth;
            lightParams.m_fAreaHeight = configuration.m_areaHeight;
            lightParams.m_fLightFrustumAngle = configuration.m_areaFOV;
            lightParams.m_Flags |= DLF_AREA_LIGHT;
        }
        break;

        case LmbrCentral::LightConfiguration::LightType::Projector:
        {
            lightParams.m_fRadius = configuration.m_projectorRange;
            lightParams.m_fLightFrustumAngle = configuration.m_projectorFOV * 0.5f;
            lightParams.m_fProjectorNearPlane = configuration.m_projectorNearPlane;
            lightParams.m_fAttenuationBulbSize = configuration.m_projectorAttenuationBulbSize;
            lightParams.m_Flags |= DLF_PROJECT;

            const char* texturePath = configuration.m_projectorTexture.GetAssetPath().c_str();
            const int flags = FT_DONT_STREAM;

            lightParams.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(texturePath, flags);

            if (!lightParams.m_pLightImage || !lightParams.m_pLightImage->IsTextureLoaded())
            {
                GetISystem()->Warning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, 0, texturePath,
                    "Light projector texture not found: %s", texturePath);
                lightParams.m_pLightImage = gEnv->pRenderer->EF_LoadTexture("Textures/defaults/red.dds", flags);
            }
        }
        break;

        case LmbrCentral::LightConfiguration::LightType::Probe:
        {
            lightParams.m_ProbeExtents.x = configuration.m_probeArea.GetX() / 2.0f;
            lightParams.m_ProbeExtents.y = configuration.m_probeArea.GetY() / 2.0f;
            lightParams.m_ProbeExtents.z = configuration.m_probeArea.GetZ() / 2.0f;
            lightParams.m_fRadius = lightParams.m_ProbeExtents.len();
            lightParams.m_nSortPriority = configuration.m_probeSortPriority;
            lightParams.m_fProbeAttenuation = configuration.m_probeFade;

            //projection
            if (configuration.m_isBoxProjected)
            {
                //enable box projection
                lightParams.m_Flags |= DLF_BOX_PROJECTED_CM;
            }
            lightParams.m_fBoxHeight = configuration.m_boxHeight;
            lightParams.m_fBoxLength = configuration.m_boxLength;
            lightParams.m_fBoxWidth = configuration.m_boxWidth;
            lightParams.SetFalloffMax(configuration.m_attenFalloffMax);

            lightParams.ReleaseCubemaps();

            const AZStd::string& specularMap = configuration.m_probeCubemap.GetAssetPath();

            if (!specularMap.empty())
            {
                AZStd::string diffuseMap = specularMap;
                AZStd::string::size_type dotPos = specularMap.find_last_of('.');
                if (dotPos != AZStd::string::npos)
                {
                    diffuseMap.insert(dotPos, "_diff");
                }

                lightParams.SetSpecularCubemap(gEnv->pRenderer->EF_LoadCubemapTexture(specularMap.c_str(), FT_DONT_STREAM));
                lightParams.SetDiffuseCubemap(gEnv->pRenderer->EF_LoadCubemapTexture(diffuseMap.c_str(), FT_DONT_STREAM));

                if (lightParams.GetDiffuseCubemap() && lightParams.GetSpecularCubemap())
                {
                    lightParams.m_Flags |= DLF_DEFERRED_CUBEMAPS;
                }
                else
                {
                    if (!lightParams.GetSpecularCubemap())
                    {
                        const char* entityName = GetEntityName(entityId);
                        AZ_Warning("Light", false,
                            "Failed to load specular cubemap \"%s\" for light \"%s\".",
                            specularMap.c_str(),
                            entityName);
                    }

                    if (!lightParams.GetDiffuseCubemap())
                    {
                        const char* entityName = GetEntityName(entityId);
                        AZ_Warning("Light", false,
                            "Failed to load diffuse cubemap \"%s\" for light \"%s\".",
                            diffuseMap.c_str(),
                            entityName);
                    }

                    lightParams.m_Flags &= ~DLF_DEFERRED_CUBEMAPS;
                    lightParams.m_Flags |= DLF_POINT;
                    lightParams.ReleaseCubemaps();
                }
            }
            else
            {
                // Disable the light
                lightParams.m_Flags = DLF_FAKE;
            }
        }
        break;
        }
    }

    void LensFlareConfigToLightParams(const LmbrCentral::LensFlareConfiguration& configuration, [[maybe_unused]] AZ::EntityId entityId, int, CDLight& lightParams)
    {
        lightParams.m_Flags |= DLF_FAKE; // As long as deferred lights are separate components disable the actual deferred light

        ColorF color = AZVec3ToLYVec3(configuration.m_tint);
        color.a = configuration.m_tintAlpha / 255.0f;
        lightParams.SetLightColor(ColorF(1.f, 1.f, 1.f, 1.f));  //Set the light color to white. We can tint the lens flare directly

        if (configuration.m_syncAnimWithLight)
        {
            lightParams.m_nLightStyle = static_cast<AZ::u8>(configuration.m_syncAnimIndex); // This is really a light animation curve ID
            lightParams.SetAnimSpeed(configuration.m_syncAnimSpeed); // maps [0, 4] to [0, 255]
            lightParams.m_nLightPhase = static_cast<AZ::u8>(configuration.m_syncAnimPhase * 255.f); // maps [0, 1] to [0, 255]
        }
        else if (configuration.m_attachToSun)
        {
            gEnv->p3DEngine->SetSunAnimIndex(static_cast<AZ::u8>(configuration.m_animIndex)); // This is really a light animation curve ID
            gEnv->p3DEngine->SetSunAnimSpeed(configuration.m_animSpeed); // maps [0, 4] to [0, 255]
            gEnv->p3DEngine->SetSunAnimPhase(static_cast<AZ::u8>(configuration.m_animPhase * 255.f)); // maps [0, 1] to [0, 255]
        }
        else
        {
            lightParams.m_nLightStyle = static_cast<AZ::u8>(configuration.m_animIndex); // This is really a light animation curve ID
            lightParams.SetAnimSpeed(configuration.m_animSpeed); // maps [0, 4] to [0, 255]
            lightParams.m_nLightPhase = static_cast<AZ::u8>(configuration.m_animPhase * 255.f); // maps [0, 1] to [0, 255]
        }

        lightParams.m_LensOpticsFrustumAngle = static_cast<AZ::u8>(configuration.m_lensFlareFrustumAngle / 360.f * 255.f);

        UpdateLightFlag(configuration.m_affectsThisAreaOnly, DLF_THIS_AREA_ONLY, lightParams.m_Flags);
        UpdateLightFlag(!configuration.m_useVisAreas, DLF_IGNORES_VISAREAS, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_indoorOnly, DLF_INDOOR_ONLY, lightParams.m_Flags);
        UpdateLightFlag(configuration.m_attachToSun, DLF_ATTACH_TO_SUN, lightParams.m_Flags);

        if (!configuration.m_lensFlare.empty())
        {
            int lensOpticsID = 0;
            if (gEnv->pOpticsManager->Load(configuration.m_lensFlare.c_str(), lensOpticsID))
            {
                IOpticsElementBase* flare = gEnv->pOpticsManager->GetOptics(lensOpticsID);
                lightParams.SetLensOpticsElement(flare);
                lightParams.SetOpticsParams({ configuration.m_brightness, configuration.m_size, { color.r, color.g, color.b, color.a }, true });
            }
            else
            {
                AZ_Warning("LensFlare", false, "Failed to load lens flare \"%s\" for entity \"%s\".", configuration.m_lensFlare.c_str(), GetEntityName(entityId));
            }
        }
    }
}

namespace LmbrCentral
{
    LightInstance::LightInstance()
        : m_renderLight(nullptr)
    {
    }

    LightInstance::~LightInstance()
    {
        DestroyRenderLight();
    }

    void LightInstance::SetEntity(AZ::EntityId entityId)
    {
        if (m_entityId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
        }

        m_entityId = entityId;

        if (m_entityId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        }
    }

    void LightInstance::OnTransformChanged(const AZ::Transform&, const AZ::Transform& world)
    {
        if (m_renderLight)
        {
            CDLight* lightProperties = &m_renderLight->GetLightProperties();
            Matrix34 worldMatrix = AZTransformToLYTransform(world);
            worldMatrix.OrthonormalizeFast(); // Lights do not support scale.
            lightProperties->SetPosition(worldMatrix.GetTranslation());
            lightProperties->SetMatrix(worldMatrix);
            m_renderLight->SetMatrix(worldMatrix);
        }
    }

    bool LightInstance::TurnOn()
    {
        if (!m_renderLight)
        {
            return false;
        }

        m_renderLight->SetRndFlags(ERF_HIDDEN, false);
        return true;
    }

    bool LightInstance::TurnOff()
    {
        if (!m_renderLight)
        {
            return false;
        }

        m_renderLight->SetRndFlags(ERF_HIDDEN, true);
        return true;
    }

    bool LightInstance::IsOn() const
    {
        if (!m_renderLight)
        {
            return false;
        }

        return 0 == (m_renderLight->GetRndFlags() & ERF_HIDDEN);
    }

    void LightInstance::CreateRenderLight(const LightConfiguration& configuration)
    {
        CreateRenderLightInternal(configuration, LightConfigToLightParams);

#if defined(FEATURE_SVO_GI)
        if (m_renderLight)
        {
            m_renderLight->SetDesiredVoxelGIMode(configuration.m_voxelGIMode);
        }
#endif
    }

    void LightInstance::CreateRenderLight(const LensFlareConfiguration& configuration)
    {
        CreateRenderLightInternal(configuration, LensFlareConfigToLightParams);
    }

    void LightInstance::UpdateRenderLight(const LightConfiguration& configuration)
    {
        // ERF_HIDDEN is used to turn on/off the light, so cache it here and set it back later
        bool isHidden = m_renderLight ? (m_renderLight->GetRndFlags() & ERF_HIDDEN) == ERF_HIDDEN : false;
        DestroyRenderLight();
        CreateRenderLight(configuration);
        if (m_renderLight)
        {
            m_renderLight->SetRndFlags(ERF_HIDDEN, isHidden);
        }
    }

    void LightInstance::UpdateRenderLight(const LensFlareConfiguration& configuration)
    {
        // ERF_HIDDEN is used to turn on/off the light, so cache it here and set it back later
        bool isHidden = m_renderLight ? (m_renderLight->GetRndFlags() & ERF_HIDDEN) == ERF_HIDDEN : false;
        DestroyRenderLight();
        CreateRenderLight(configuration);
        if (m_renderLight)
        {
            m_renderLight->SetRndFlags(ERF_HIDDEN, isHidden);
        }
    }

    void LightInstance::DestroyRenderLight()
    {
        if (m_renderLight)
        {
            m_renderLight->ReleaseNode();
            m_renderLight = nullptr;
        }
    }

    IRenderNode* LightInstance::GetRenderNode()
    {
        return m_renderLight;
    }

    template <typename ConfigurationType, typename ConfigToLightParamsFunc>
    void LightInstance::CreateRenderLightInternal(const ConfigurationType& configuration, ConfigToLightParamsFunc configToLightParams)
    {
        if (m_renderLight || !configuration.m_visible)
        {
            return;
        }

        ICVar* cvarSysSpecLight = gEnv->pConsole->GetCVar("e_LightQuality");
        const int configSpec = cvarSysSpecLight ? cvarSysSpecLight->GetIVal() : gEnv->pSystem->GetConfigSpec(true);

        if (static_cast<AZ::u32>(configSpec) < static_cast<AZ::u32>(configuration.m_minSpec))
        {
            // Light is disabled in the active system spec.
            return;
        }

        CDLight lightParams;
        configToLightParams(configuration, m_entityId, configSpec, lightParams);

        m_renderLight = gEnv->p3DEngine->CreateLightSource();

        if (!configuration.m_material.GetAssetPath().empty())
        {
            m_renderLight->SetMaterial(gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(configuration.m_material.GetAssetPath().c_str()));
        }

        m_renderLight->SetLightProperties(lightParams);
        m_renderLight->SetMinSpec(static_cast<int>(configuration.m_minSpec));
        m_renderLight->SetViewDistanceMultiplier(configuration.m_viewDistMultiplier);

        AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(parentTransform, m_entityId, AZ::TransformBus, GetWorldTM);
        OnTransformChanged(AZ::Transform::CreateIdentity(), parentTransform);
    }
} // namespace LmbrCentral
