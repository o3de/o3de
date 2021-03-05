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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include "LightInstance.h"
#include "LightComponent.h"

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////
    class BehaviorLightComponentNotificationBusHandler : public LightComponentNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorLightComponentNotificationBusHandler, "{969C5B17-10D1-41DB-8123-6664FA64B4E9}", AZ::SystemAllocator,
            LightTurnedOn, LightTurnedOff);

        // Sent when the light is turned on.
        void LightTurnedOn() override
        {
            Call(FN_LightTurnedOn);
        }

        // Sent when the light is turned off.
        void LightTurnedOff() override
        {
            Call(FN_LightTurnedOff);
        }
    };

    void LightComponent::Reflect(AZ::ReflectContext* context)
    {
        LightConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LightComponent, AZ::Component>()
                ->Version(1)
                ->Field("LightConfiguration", &LightComponent::m_configuration);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<LightComponent>()->RequestBus("LightComponentRequestBus");
            ;
            auto probeFadeDefaultValue(behaviorContext->MakeDefaultValue(1.0f));

            behaviorContext->EBus<LightComponentRequestBus>("Light", "LightComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Event("SetState", &LightComponentRequestBus::Events::SetLightState, "SetLightState", { { { "State", "1=On, 0=Off" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Set the light state")
                ->Event("TurnOn", &LightComponentRequestBus::Events::TurnOnLight, "TurnOnLight")
                ->Event("TurnOff", &LightComponentRequestBus::Events::TurnOffLight, "TurnOffLight")
                ->Event("Toggle", &LightComponentRequestBus::Events::ToggleLight, "ToggleLight")
                // General Settings Modifiers
                ->Event("GetVisible", &LightComponentRequestBus::Events::GetVisible)
                ->Event("SetVisible", &LightComponentRequestBus::Events::SetVisible, { { { "IsVisible", "" } } })
                ->VirtualProperty("Visible", "GetVisible", "SetVisible")
                ->Event("GetColor", &LightComponentRequestBus::Events::GetColor)
                ->Event("SetColor", &LightComponentRequestBus::Events::SetColor, { { { "Color", "The color to set" } } })
                ->VirtualProperty("Color", "GetColor", "SetColor")
                ->Event("GetDiffuseMultiplier", &LightComponentRequestBus::Events::GetDiffuseMultiplier)
                ->Event("SetDiffuseMultiplier", &LightComponentRequestBus::Events::SetDiffuseMultiplier, { { { "Multiplier", "The multiplier" } } })
                ->VirtualProperty("DiffuseMultiplier", "GetDiffuseMultiplier", "SetDiffuseMultiplier")
                ->Event("GetSpecularMultiplier", &LightComponentRequestBus::Events::GetSpecularMultiplier)
                ->Event("SetSpecularMultiplier", &LightComponentRequestBus::Events::SetSpecularMultiplier, { { { "Multiplier", "The multiplier" } } })
                ->VirtualProperty("SpecularMultiplier", "GetSpecularMultiplier", "SetSpecularMultiplier")
                ->Event("GetAmbient", &LightComponentRequestBus::Events::GetAmbient)
                ->Event("SetAmbient", &LightComponentRequestBus::Events::SetAmbient, { { { "IsAmbient", "" } } })
                ->VirtualProperty("Ambient", "GetAmbient", "SetAmbient")
                // Point Light Modifiers
                ->Event("GetPointMaxDistance", &LightComponentRequestBus::Events::GetPointMaxDistance)
                ->Event("SetPointMaxDistance", &LightComponentRequestBus::Events::SetPointMaxDistance, { { { "Distance", "The max point distance" } } })
                ->VirtualProperty("PointMaxDistance", "GetPointMaxDistance", "SetPointMaxDistance")
                ->Event("GetPointAttenuationBulbSize", &LightComponentRequestBus::Events::GetPointAttenuationBulbSize)
                ->Event("SetPointAttenuationBulbSize", &LightComponentRequestBus::Events::SetPointAttenuationBulbSize, { { { "BulbSize", "The size of the bulb" } } })
                ->VirtualProperty("PointAttenuationBulbSize", "GetPointAttenuationBulbSize", "SetPointAttenuationBulbSize")
                // Area Light Modifiers
                ->Event("GetAreaMaxDistance", &LightComponentRequestBus::Events::GetAreaMaxDistance)
                ->Event("SetAreaMaxDistance", &LightComponentRequestBus::Events::SetAreaMaxDistance, { { { "Distance", "The max point distance" } } })
                ->VirtualProperty("AreaMaxDistance", "GetAreaMaxDistance", "SetAreaMaxDistance")
                ->Event("GetAreaWidth", &LightComponentRequestBus::Events::GetAreaWidth)
                ->Event("SetAreaWidth", &LightComponentRequestBus::Events::SetAreaWidth, { { { "Width", "Area Width" } } })
                ->VirtualProperty("AreaWidth", "GetAreaWidth", "SetAreaWidth")
                ->Event("GetAreaHeight", &LightComponentRequestBus::Events::GetAreaHeight)
                ->Event("SetAreaHeight", &LightComponentRequestBus::Events::SetAreaHeight, { { { "Height", "Area Height" } } })
                ->VirtualProperty("AreaHeight", "GetAreaHeight", "SetAreaHeight")
                ->Event("GetAreaFOV", &LightComponentRequestBus::Events::GetAreaFOV)
                ->Event("SetAreaFOV", &LightComponentRequestBus::Events::SetAreaFOV, { { { "FOV", "Field of View" } } })
                ->VirtualProperty("AreaFOV", "GetAreaFOV", "SetAreaFOV")
                // Projector Light Modifiers
                ->Event("GetProjectorMaxDistance", &LightComponentRequestBus::Events::GetProjectorMaxDistance)
                ->Event("SetProjectorMaxDistance", &LightComponentRequestBus::Events::SetProjectorMaxDistance, { { { "Distance", "Projector distance" } } })
                ->VirtualProperty("ProjectorMaxDistance", "GetProjectorMaxDistance", "SetProjectorMaxDistance")
                ->Event("GetProjectorAttenuationBulbSize", &LightComponentRequestBus::Events::GetProjectorAttenuationBulbSize)
                ->Event("SetProjectorAttenuationBulbSize", &LightComponentRequestBus::Events::SetProjectorAttenuationBulbSize, { { { "BulbSize", "The size of the bulb" } } })
                ->VirtualProperty("ProjectorAttenuationBulbSize", "GetProjectorAttenuationBulbSize", "SetProjectorAttenuationBulbSize")
                ->Event("GetProjectorFOV", &LightComponentRequestBus::Events::GetProjectorFOV)
                ->Event("SetProjectorFOV", &LightComponentRequestBus::Events::SetProjectorFOV, { { { "FOV", "Field of View" } } })
                ->VirtualProperty("ProjectorFOV", "GetProjectorFOV", "SetProjectorFOV")
                ->Event("GetProjectorNearPlane", &LightComponentRequestBus::Events::GetProjectorNearPlane)
                ->Event("SetProjectorNearPlane", &LightComponentRequestBus::Events::SetProjectorNearPlane, { { { "Plane", "Plane distance" } } })
                ->VirtualProperty("ProjectorNearPlane", "GetProjectorNearPlane", "SetProjectorNearPlane")
                // Environment Probe Light Modifiers
                ->Event("GetProbeAreaDimensions", &LightComponentRequestBus::Events::GetProbeAreaDimensions)
                ->Event("SetProbeAreaDimensions", &LightComponentRequestBus::Events::SetProbeAreaDimensions, { { { "Dimension", "The X,Y and Z extents" } } })
                ->VirtualProperty("ProbeAreaDimensions", "GetProbeAreaDimensions", "SetProbeAreaDimensions")
                ->Event("GetProbeSortPriority", &LightComponentRequestBus::Events::GetProbeSortPriority)
                ->Event("SetProbeSortPriority", &LightComponentRequestBus::Events::SetProbeSortPriority, { { { "Priority", "" } } })
                ->VirtualProperty("ProbeSortPriority", "GetProbeSortPriority", "SetProbeSortPriority")
                ->Event("GetProbeBoxProjected", &LightComponentRequestBus::Events::GetProbeBoxProjected)
                ->Event("SetProbeBoxProjected", &LightComponentRequestBus::Events::SetProbeBoxProjected, { { { "IsProjected", "TRUE will project the box, False otherwise." } } })
                ->VirtualProperty("ProbeBoxProjected", "GetProbeBoxProjected", "SetProbeBoxProjected")
                ->Event("GetProbeBoxHeight", &LightComponentRequestBus::Events::GetProbeBoxHeight)
                ->Event("SetProbeBoxHeight", &LightComponentRequestBus::Events::SetProbeBoxHeight, { { { "Height", "Box Height" } } })
                ->VirtualProperty("ProbeBoxHeight", "GetProbeBoxHeight", "SetProbeBoxHeight")
                ->Event("GetProbeBoxLength", &LightComponentRequestBus::Events::GetProbeBoxLength)
                ->Event("SetProbeBoxLength", &LightComponentRequestBus::Events::SetProbeBoxLength, { { { "Length", "Box Length" } } })
                ->VirtualProperty("ProbeBoxLength", "GetProbeBoxLength", "SetProbeBoxLength")
                ->Event("GetProbeBoxWidth", &LightComponentRequestBus::Events::GetProbeBoxWidth)
                ->Event("SetProbeBoxWidth", &LightComponentRequestBus::Events::SetProbeBoxWidth, { { { "Width", "Box Width" } } })
                ->VirtualProperty("ProbeBoxWidth", "GetProbeBoxWidth", "SetProbeBoxWidth")
                ->Event("GetProbeAttenuationFalloff", &LightComponentRequestBus::Events::GetProbeAttenuationFalloff)
                ->Event("SetProbeAttenuationFalloff", &LightComponentRequestBus::Events::SetProbeAttenuationFalloff, { { { "Falloff", "Smoothness of the falloff around the probe's bounds" } } })
                ->VirtualProperty("ProbeAttenuationFalloff", "GetProbeAttenuationFalloff", "SetProbeAttenuationFalloff")
                ->Event("GetProbeFade", &LightComponentRequestBus::Events::GetProbeFade)
                ->Event("SetProbeFade", &LightComponentRequestBus::Events::SetProbeFade, { { { "Fade", "Multiplier for fading out a probe [0-1]", probeFadeDefaultValue } } })
                ->VirtualProperty("ProbeFade", "GetProbeFade", "SetProbeFade")
                ;

            behaviorContext->EBus<LightComponentNotificationBus>("LightNotification", "LightComponentNotificationBus", "Notifications for the Light Components")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Handler<BehaviorLightComponentNotificationBusHandler>();
        }
    }

    void LightConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LightConfiguration>()
                ->Version(8, &VersionConverter)
                ->Field("LightType", &LightConfiguration::m_lightType)
                ->Field("Visible", &LightConfiguration::m_visible)
                ->Field("OnInitially", &LightConfiguration::m_onInitially)
                ->Field("Color", &LightConfiguration::m_color)
                ->Field("DiffuseMultiplier", &LightConfiguration::m_diffuseMultiplier)
                ->Field("SpecMultiplier", &LightConfiguration::m_specMultiplier)
                ->Field("Ambient", &LightConfiguration::m_ambient)
                ->Field("PointMaxDistance", &LightConfiguration::m_pointMaxDistance)
                ->Field("PointAttenuationBulbSize", &LightConfiguration::m_pointAttenuationBulbSize)
                ->Field("AreaWidth", &LightConfiguration::m_areaWidth)
                ->Field("AreaHeight", &LightConfiguration::m_areaHeight)
                ->Field("AreaMaxDistance", &LightConfiguration::m_areaMaxDistance)
                ->Field("AreaFOV", &LightConfiguration::m_areaFOV)
                ->Field("ProjectorDistance", &LightConfiguration::m_projectorRange)
                ->Field("ProjectorAttenuationBulbSize", &LightConfiguration::m_projectorAttenuationBulbSize)
                ->Field("ProjectorFOV", &LightConfiguration::m_projectorFOV)
                ->Field("ProjectorNearPlane", &LightConfiguration::m_projectorNearPlane)
                ->Field("ProjectorTexture", &LightConfiguration::m_projectorTexture)
                ->Field("ProjectorMaterial", &LightConfiguration::m_material)
                ->Field("Area X,Y,Z", &LightConfiguration::m_probeArea)
                ->Field("SortPriority", &LightConfiguration::m_probeSortPriority)
                ->Field("CubemapResolution", &LightConfiguration::m_probeCubemapResolution)
                ->Field("CubemapTexture", &LightConfiguration::m_probeCubemap)
                ->Field("BoxProject", &LightConfiguration::m_isBoxProjected)
                ->Field("BoxHeight", &LightConfiguration::m_boxHeight)
                ->Field("BoxLength", &LightConfiguration::m_boxLength)
                ->Field("BoxWidth", &LightConfiguration::m_boxWidth)
                ->Field("AttenuationFalloffMax", &LightConfiguration::m_attenFalloffMax)
                ->Field("ViewDistanceMultiplier", &LightConfiguration::m_viewDistMultiplier)
                ->Field("MinimumSpec", &LightConfiguration::m_minSpec)
                ->Field("CastShadowsSpec", &LightConfiguration::m_castShadowsSpec)
                ->Field("VoxelGIMode", &LightConfiguration::m_voxelGIMode)
                ->Field("UseVisAreas", &LightConfiguration::m_useVisAreas)
                ->Field("IndoorOnly", &LightConfiguration::m_indoorOnly)
                ->Field("AffectsThisAreaOnly", &LightConfiguration::m_affectsThisAreaOnly)
                ->Field("VolumetricFogOnly", &LightConfiguration::m_volumetricFogOnly)
                ->Field("VolumetricFog", &LightConfiguration::m_volumetricFog)
                ->Field("Deferred", &LightConfiguration::m_deferred)
                ->Field("TerrainShadows", &LightConfiguration::m_castTerrainShadows)
                ->Field("ShadowBias", &LightConfiguration::m_shadowBias)
                ->Field("ShadowResScale", &LightConfiguration::m_shadowResScale)
                ->Field("ShadowSlopeBias", &LightConfiguration::m_shadowSlopeBias)
                ->Field("ShadowUpdateMinRadius", &LightConfiguration::m_shadowUpdateMinRadius)
                ->Field("ShadowUpdateRatio", &LightConfiguration::m_shadowUpdateRatio)
                ->Field("AnimIndex", &LightConfiguration::m_animIndex)
                ->Field("AnimSpeed", &LightConfiguration::m_animSpeed)
                ->Field("AnimPhase", &LightConfiguration::m_animPhase)
                ->Field("CubemapId", &LightConfiguration::m_cubemapId);
        }
    }

    // Private Static 
    bool LightConfiguration::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Need to rename OmniRadius to MaxDistance
        // - remove ShadowBlurStrength
        if (classElement.GetVersion() <= 1)
        {
            int radiusIndex = classElement.FindElement(AZ_CRC("OmniRadius", 0x3fbb253e));

            if (radiusIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& radius = classElement.GetSubElement(radiusIndex);
            radius.SetName("MaxDistance");

            classElement.RemoveElementByName(AZ_CRC("ShadowBlurStrength", 0x70ac8e34));
        }

        // conversion from version 2:
        // - Replace AreaSize with AreaWidth and AreaHeights
        if (classElement.GetVersion() <= 2)
        {
            int areaIndex = classElement.FindElement(AZ_CRC("AreaSize", 0x287b852c));

            if (areaIndex < 0)
            {
                return false;
            }

            //Get the size
            AZ::SerializeContext::DataElementNode& area = classElement.GetSubElement(areaIndex);
            AZ::Vector3 size;

            area.GetData<AZ::Vector3>(size);

            //Create width and height
            int areaWidthIndex = classElement.AddElement<float>(context, "AreaWidth");
            int areaHeightIndex = classElement.AddElement<float>(context, "AreaHeight");

            AZ::SerializeContext::DataElementNode& areaWidth = classElement.GetSubElement(areaWidthIndex);
            AZ::SerializeContext::DataElementNode& areaHeight = classElement.GetSubElement(areaHeightIndex);

            //Set width and height data to x and y from the old size
            areaWidth.SetData<float>(context, size.GetX());
            areaHeight.SetData<float>(context, size.GetY());
            classElement.RemoveElement(areaIndex);
        }

        // conversion from version 3:
        // - MaxDistance turned into PointMaxDistance and AreaMaxDistance
        // - AttenuationBulbSize turned into PointAttenuationBulbSize and ProjectorAttenuationBulbSize
        // - Apply old area size to new probe area
        if (classElement.GetVersion() <= 3)
        {
            int maxDistanceIndex = classElement.FindElement(AZ_CRC("MaxDistance", 0xfc2921b8));
            int attenBulbIndex = classElement.FindElement(AZ_CRC("AttenuationBulbSize", 0x3ca0f5c0));
            int areaWidthIndex = classElement.FindElement(AZ_CRC("AreaWidth", 0x137a1a2b));
            int areaHeightIndex = classElement.FindElement(AZ_CRC("AreaHeight", 0xf2bf4149));

            if (maxDistanceIndex < 0 || attenBulbIndex < 0 || areaWidthIndex < 0 || areaHeightIndex < 0)
            {
                return false;
            }

            //Get some values
            AZ::SerializeContext::DataElementNode& maxDistance = classElement.GetSubElement(maxDistanceIndex);
            AZ::SerializeContext::DataElementNode& attenBulbSize = classElement.GetSubElement(attenBulbIndex);
            AZ::SerializeContext::DataElementNode& areaWidth = classElement.GetSubElement(areaWidthIndex);
            AZ::SerializeContext::DataElementNode& areaHeight = classElement.GetSubElement(attenBulbIndex);

            float maxDistVal, attenBulbVal, areaWidthVal, areaHeightVal;

            maxDistance.GetData<float>(maxDistVal);
            attenBulbSize.GetData<float>(attenBulbVal);
            areaWidth.GetData<float>(areaWidthVal);
            areaHeight.GetData<float>(areaHeightVal);

            //Create AreaMaxDistance, PointMaxDistance, PointAttenuationBulbSize and ProjectorAttenuationBulbSize
            int areaMaxDistIndex = classElement.AddElement<float>(context, "AreaMaxDistance");
            int pointMaxDistIndex = classElement.AddElement<float>(context, "PointMaxDistance");
            int pointAttenBulbIndex = classElement.AddElement<float>(context, "PointAttenuationBulbSize");
            int projectorAttenBulbIndex = classElement.AddElement<float>(context, "ProjectorAttenuationBulbSize");
            int areaXYZIndex = classElement.AddElement<AZ::Vector3>(context, "Area X,Y,Z");

            AZ::SerializeContext::DataElementNode& areaMaxDist = classElement.GetSubElement(areaMaxDistIndex);
            AZ::SerializeContext::DataElementNode& pointMaxDist = classElement.GetSubElement(pointMaxDistIndex);
            AZ::SerializeContext::DataElementNode& pointAttenBulb = classElement.GetSubElement(pointAttenBulbIndex);
            AZ::SerializeContext::DataElementNode& projectorAttenBulb = classElement.GetSubElement(projectorAttenBulbIndex);
            AZ::SerializeContext::DataElementNode& areaXYZ = classElement.GetSubElement(areaXYZIndex);

            //Set width and height data to x and y from the old size
            areaMaxDist.SetData<float>(context, maxDistVal);
            pointMaxDist.SetData<float>(context, maxDistVal);
            pointAttenBulb.SetData<float>(context, attenBulbVal);
            projectorAttenBulb.SetData<float>(context, attenBulbVal);

            //Set new area XYZ values based on existing area values
            areaXYZ.SetData<AZ::Vector3>(context, AZ::Vector3(maxDistVal, areaWidthVal, areaHeightVal));

            //Remove old MaxDistance and AttenuationBulbSize
            classElement.RemoveElement(maxDistanceIndex);
            classElement.RemoveElement(attenBulbIndex);
        }

        // conversion from version 4:
        // m_color: AZ::Vector4 -> AZ::Color
        if (classElement.GetVersion() <= 4)
        {
            int colorIndex = classElement.FindElement(AZ_CRC("Color", 0x665648e9));
            if (colorIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& color = classElement.GetSubElement(colorIndex);
            AZ::Vector4 colorVec;
            color.GetData<AZ::Vector4>(colorVec);
            AZ::Color colorVal(colorVec.GetX(), colorVec.GetY(), colorVec.GetZ(), colorVec.GetW());
            color.Convert<AZ::Color>(context);
            color.SetData(context, colorVal);
        }

        // conversion from version 6 to version 7:
        // - Need to rename IgnoreVisAreas to UseVisAreas
        // UseVisAreas is the Inverse of IgnoreVisAreas
        if (classElement.GetVersion() <= 6)
        {
            int ignoreVisAreasIndex = classElement.FindElement(AZ_CRC("IgnoreVisAreas", 0x01823201));

            if (ignoreVisAreasIndex < 0)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& useVisAreasNode = classElement.GetSubElement(ignoreVisAreasIndex);
            useVisAreasNode.SetName("UseVisAreas");

            bool ignoreVisAreas = true;
            if (!useVisAreasNode.GetData<bool>(ignoreVisAreas))
            {
                return false;
            }

            if (!useVisAreasNode.SetData<bool>(context, !ignoreVisAreas))
            {
                return false;
            }
        }

        if (classElement.GetVersion() <= 7)
        {
            AZ::SerializeContext::DataElementNode* cubemapTexture = classElement.FindSubElement(AZ_CRC("CubemapTexture", 0xbf6d8df4));

            if (!cubemapTexture)
            {
                return false;
            }

            AZStd::string cubemapPath;

            if (!cubemapTexture->GetData<AZStd::string>(cubemapPath))
            {
                return false;
            }

            AzFramework::SimpleAssetReference<TextureAsset> cubemapSimpleAsset;

            cubemapSimpleAsset.SetAssetPath(cubemapPath.c_str());

            if (!classElement.RemoveElementByName(AZ_CRC("CubemapTexture", 0xbf6d8df4)))
            {
                return false;
            }

            if (!classElement.AddElementWithData<AzFramework::SimpleAssetReference<TextureAsset>>(context, "CubemapTexture", cubemapSimpleAsset))
            {
                return false;
            }
        }

        return true;
    }

    bool LightComponent::TurnOnLight()
    {
        bool success = m_light.TurnOn();
        if (success)
        {
            EBUS_EVENT_ID(GetEntityId(), LightComponentNotificationBus, LightTurnedOn);
        }
        return success;
    }

    bool LightComponent::TurnOffLight()
    {
        bool success = m_light.TurnOff();
        if (success)
        {
            EBUS_EVENT_ID(GetEntityId(), LightComponentNotificationBus, LightTurnedOff);
        }
        return success;
    }

    ///////////////////////////////////////////////////////////////////////
    // Modifiers
    void LightComponent::SetVisible(bool isVisible)
    {
        if (isVisible != m_configuration.m_visible)
        {
            m_configuration.m_visible = isVisible;

            if (m_configuration.m_visible)
            {
                if (!TurnOnLight())
                {
                    // unable to turn on light. This happens when trying to turn on a light was was previously not visible. Use UpdateRenderLight to
                    // update the light's visibility (by recreating it) to turn it on.
                    m_light.UpdateRenderLight(m_configuration);
                }
            }
            else
            {
                TurnOffLight();
            }
        }
    }

    bool LightComponent::GetVisible()
    {
        return m_light.IsOn();
    }

    void LightComponent::SetColor(const AZ::Color& newColor)
    {
        if (m_configuration.m_color != newColor)
        {
            m_configuration.m_color = newColor;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    const AZ::Color LightComponent::GetColor()
    {
        return m_configuration.m_color;
    }

    void LightComponent::SetDiffuseMultiplier(float newMultiplier)
    {
        if (m_configuration.m_diffuseMultiplier != newMultiplier)
        {
            m_configuration.m_diffuseMultiplier = newMultiplier;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetDiffuseMultiplier()
    {
        return m_configuration.m_diffuseMultiplier;
    }

    void LightComponent::SetSpecularMultiplier(float newMultiplier)
    {
        if (m_configuration.m_specMultiplier != newMultiplier)
        {
            m_configuration.m_specMultiplier = newMultiplier;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    float LightComponent::GetSpecularMultiplier()
    {
        return m_configuration.m_specMultiplier;
    }

    void LightComponent::SetAmbient(bool isAmbient)
    {
        if (isAmbient != m_configuration.m_ambient)
        {
            m_configuration.m_ambient = isAmbient;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    bool LightComponent::GetAmbient()
    {
        return m_configuration.m_ambient;
    }

    void LightComponent::SetPointMaxDistance(float newMaxDistance)
    {
        if (newMaxDistance != m_configuration.m_pointMaxDistance)
        {
            m_configuration.m_pointMaxDistance = newMaxDistance;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetPointMaxDistance()
    {
        return m_configuration.m_pointMaxDistance;
    }

    void LightComponent::SetPointAttenuationBulbSize(float newAttenuationBulbSize)
    {
        if (newAttenuationBulbSize != m_configuration.m_pointAttenuationBulbSize)
        {
            m_configuration.m_pointAttenuationBulbSize = newAttenuationBulbSize;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetPointAttenuationBulbSize()
    {
        return m_configuration.m_pointAttenuationBulbSize;
    }

    void LightComponent::SetAreaMaxDistance(float newMaxDistance)
    {
        if (newMaxDistance != m_configuration.m_areaMaxDistance)
        {
            m_configuration.m_areaMaxDistance = newMaxDistance;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetAreaMaxDistance()
    {
        return m_configuration.m_areaMaxDistance;
    }

    void LightComponent::SetAreaWidth(float newWidth)
    {
        if (newWidth != m_configuration.m_areaWidth)
        {
            m_configuration.m_areaWidth = newWidth;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetAreaWidth()
    {
        return m_configuration.m_areaWidth;
    }

    void LightComponent::SetAreaHeight(float newHeight)
    {
        if (newHeight != m_configuration.m_areaHeight)
        {
            m_configuration.m_areaHeight = newHeight;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetAreaHeight()
    {
        return m_configuration.m_areaHeight;
    }

    void LightComponent::SetAreaFOV(float newFOV)
    {
        if (newFOV != m_configuration.m_areaFOV)
        {
            m_configuration.m_areaFOV = newFOV;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetAreaFOV()
    {
        return m_configuration.m_areaFOV;
    }

    void LightComponent::SetProjectorMaxDistance(float newMaxDistance)
    {
        if (newMaxDistance != m_configuration.m_projectorRange)
        {
            m_configuration.m_projectorRange = newMaxDistance;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetProjectorMaxDistance()
    {
        return m_configuration.m_projectorRange;
    }

    void LightComponent::SetProjectorAttenuationBulbSize(float newAttenuationBulbSize)
    {
        if (newAttenuationBulbSize != m_configuration.m_projectorAttenuationBulbSize)
        {
            m_configuration.m_projectorAttenuationBulbSize = newAttenuationBulbSize;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetProjectorAttenuationBulbSize()
    {
        return m_configuration.m_projectorAttenuationBulbSize;
    }

    void LightComponent::SetProjectorFOV(float newFOV)
    {
        if (newFOV != m_configuration.m_projectorFOV)
        {
            m_configuration.m_projectorFOV = newFOV;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetProjectorFOV()
    {
        return m_configuration.m_projectorFOV;
    }

    void LightComponent::SetProjectorNearPlane(float newNearPlane)
    {
        if (newNearPlane != m_configuration.m_projectorNearPlane)
        {
            m_configuration.m_projectorNearPlane = newNearPlane;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetProjectorNearPlane()
    {
        return m_configuration.m_projectorNearPlane;
    }

    void LightComponent::SetProbeAreaDimensions(const AZ::Vector3& newDimensions)
    {
        if (newDimensions != m_configuration.m_probeArea)
        {
            m_configuration.m_probeArea = newDimensions;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    const AZ::Vector3 LightComponent::GetProbeAreaDimensions()
    {
        return m_configuration.m_probeArea;
    }

    void LightComponent::SetProbeSortPriority(AZ::u32 newPriority)
    {
        if (newPriority != m_configuration.m_probeSortPriority)
        {
            m_configuration.m_probeSortPriority = newPriority;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    AZ::u32 LightComponent::GetProbeSortPriority()
    {
        return m_configuration.m_probeSortPriority;
    }

    void LightComponent::SetProbeBoxProjected(bool isProbeBoxProjected)
    {
        if (isProbeBoxProjected != m_configuration.m_isBoxProjected)
        {
            m_configuration.m_isBoxProjected = isProbeBoxProjected;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    bool LightComponent::GetProbeBoxProjected()
    {
        return m_configuration.m_isBoxProjected;
    }

    void LightComponent::SetProbeBoxHeight(float newHeight)
    {
        if (newHeight != m_configuration.m_boxHeight)
        {
            m_configuration.m_boxHeight = newHeight;
            m_light.UpdateRenderLight(m_configuration);
        }
    }
    float LightComponent::GetProbeBoxHeight()
    {
        return m_configuration.m_boxHeight;
    }

    void LightComponent::SetProbeBoxLength(float newLength)
    {
        if (newLength != m_configuration.m_boxLength)
        {
            m_configuration.m_boxLength = newLength;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    float LightComponent::GetProbeBoxLength()
    {
        return m_configuration.m_boxLength;
    }

    void LightComponent::SetProbeBoxWidth(float newWidth)
    {
        if (newWidth != m_configuration.m_boxWidth)
        {
            m_configuration.m_boxWidth = newWidth;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    float LightComponent::GetProbeBoxWidth()
    {
        return m_configuration.m_boxWidth;
    }

    void LightComponent::SetProbeAttenuationFalloff(float newAttenuationFalloff)
    {
        if (newAttenuationFalloff != m_configuration.m_attenFalloffMax)
        {
            m_configuration.m_attenFalloffMax = newAttenuationFalloff;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    float LightComponent::GetProbeAttenuationFalloff()
    {
        return m_configuration.m_attenFalloffMax;
    }

    void LightComponent::SetProbeFade(float fade)
    {
        AZ_Warning("Lighting", 0.0f <= fade && fade <= 1.0f, "SetProbeFade value %f out of range. Clamping to [0,1]", fade);
        fade = AZ::GetClamp(fade, 0.0f, 1.0f);

        if (fade != m_configuration.m_probeFade)
        {
            m_configuration.m_probeFade = fade;
            m_light.UpdateRenderLight(m_configuration);
        }
    }

    float LightComponent::GetProbeFade()
    {
        return m_configuration.m_probeFade;
    }

    ///////////////////////////////////////////////////////////
    void LightComponent::ToggleLight()
    {
        if (m_light.IsOn())
        {
            TurnOffLight();
        }
        else
        {
            TurnOnLight();
        }
    }

    void LightComponent::SetLightState(State state)
    {
        switch (state)
        {
        case LightComponentRequests::State::On:
        {
            TurnOnLight();
        }
        break;

        case LightComponentRequests::State::Off:
        {
            TurnOffLight();
        }
        break;
        }
    }

    IRenderNode* LightComponent::GetRenderNode()
    {
        return m_light.GetRenderNode();
    }

    float LightComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    const float LightComponent::s_renderNodeRequestBusOrder = 500.f;

    LightComponent::LightComponent()
    {
    }

    LightComponent::~LightComponent()
    {
    }

    void LightComponent::Init()
    {
    }

    void LightComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        m_light.SetEntity(entityId);
        m_light.CreateRenderLight(m_configuration);

        LightComponentRequestBus::Handler::BusConnect(entityId);
        RenderNodeRequestBus::Handler::BusConnect(entityId);

        if (m_configuration.m_onInitially)
        {
            m_light.TurnOn();
        }
        else
        {
            m_light.TurnOff();
        }
    }

    void LightComponent::Deactivate()
    {
        LightComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
        m_light.DestroyRenderLight();
        m_light.SetEntity(AZ::EntityId());
    }

    LightConfiguration::LightConfiguration()
        : m_lightType(LightType::Point)
        , m_visible(true)
        , m_onInitially(true)
        , m_pointMaxDistance(2.f)
        , m_pointAttenuationBulbSize(0.05f)
        , m_areaMaxDistance(2.f)
        , m_areaWidth(5.f)
        , m_areaHeight(5.f)
        , m_areaFOV(45.0f)
        , m_projectorAttenuationBulbSize(0.05f)
        , m_projectorRange(5.f)
        , m_projectorFOV(90.f)
        , m_projectorNearPlane(0)
        , m_probeSortPriority(0)
        , m_probeArea(20.0f, 20.0f, 20.0f)
        , m_probeCubemapResolution(ResolutionSetting::ResDefault)
        , m_isBoxProjected(false)
        , m_boxWidth(20.0f)
        , m_boxHeight(20.0f)
        , m_boxLength(20.0f)
        , m_attenFalloffMax(0.3f)
        , m_probeFade(1.0f)
        , m_minSpec(EngineSpec::Low)
        , m_viewDistMultiplier(1.f)
        , m_castShadowsSpec(EngineSpec::Never)
        , m_voxelGIMode(IRenderNode::VM_None)
        , m_color(1.f, 1.f, 1.f, 1.f)
        , m_diffuseMultiplier(1.f)
        , m_specMultiplier(1.f)
        , m_affectsThisAreaOnly(true)
        , m_useVisAreas(true)
        , m_volumetricFog(true)
        , m_volumetricFogOnly(false)
        , m_indoorOnly(false)
        , m_ambient(false)
        , m_deferred(true)
        , m_animIndex(0)
        , m_animSpeed(1.f)
        , m_animPhase(0.f)
        , m_castTerrainShadows(false)
        , m_shadowBias(1.f)
        , m_shadowSlopeBias(1.f)
        , m_shadowResScale(1.f)
        , m_shadowUpdateMinRadius(10.f)
        , m_shadowUpdateRatio(1.f)
        , m_cubemapId(AZ::Uuid::Create())
    {
    }
} // namespace LmbrCentral