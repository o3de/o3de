--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {
        "subsurfaceScattering.enableSubsurfaceScattering", 
        "subsurfaceScattering.influenceMap", 
        "subsurfaceScattering.useInfluenceMap", 
        "subsurfaceScattering.transmissionMode", 
        "subsurfaceScattering.thicknessMap", 
        "subsurfaceScattering.useThicknessMap", 
        }
end

function GetShaderOptionDependencies()
    return {
        "o_subsurfaceScattering_useTexture", 
        "o_transmission_useTexture", 
        }
end
 
TransmissionMode_None = 0
TransmissionMode_ThickObject = 1
TransmissionMode_ThinObject = 2

function UpdateUseTextureState(context, subsurfaceScatteringEnabled, textureMapPropertyName, useTexturePropertyName, shaderOptionName) 
    local textureMap = context:GetMaterialPropertyValue_Image(textureMapPropertyName)
    local useTextureMap = context:GetMaterialPropertyValue_bool(useTexturePropertyName)
    context:SetShaderOptionValue_bool(shaderOptionName, subsurfaceScatteringEnabled and useTextureMap and textureMap ~= nil)
end

function Process(context)
    local ssEnabled = context:GetMaterialPropertyValue_bool("subsurfaceScattering.enableSubsurfaceScattering")
    local transmissionEnabled = TransmissionMode_None ~= context:GetMaterialPropertyValue_enum("subsurfaceScattering.transmissionMode")
    UpdateUseTextureState(context, ssEnabled, "subsurfaceScattering.influenceMap", "subsurfaceScattering.useInfluenceMap", "o_subsurfaceScattering_useTexture")
    UpdateUseTextureState(context, transmissionEnabled, "subsurfaceScattering.thicknessMap", "subsurfaceScattering.useThicknessMap", "o_transmission_useTexture")
end

function UpdateTextureDependentPropertyVisibility(context, featureEnabled, textureMapPropertyName, useTexturePropertyName, uvPropertyName)

    if(not featureEnabled) then
        context:SetMaterialPropertyVisibility(useTexturePropertyName, MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Hidden)
    else
        local textureMap = context:GetMaterialPropertyValue_Image(textureMapPropertyName)
        local useTextureMap = context:GetMaterialPropertyValue_bool(useTexturePropertyName)

        if(textureMap == nil) then
            context:SetMaterialPropertyVisibility(useTexturePropertyName, MaterialPropertyVisibility_Hidden)
            context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Hidden)
        elseif(not useTextureMap) then
            context:SetMaterialPropertyVisibility(useTexturePropertyName, MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Disabled)
        else
            context:SetMaterialPropertyVisibility(useTexturePropertyName, MaterialPropertyVisibility_Enabled)
            context:SetMaterialPropertyVisibility(uvPropertyName, MaterialPropertyVisibility_Enabled)
        end
    end

end

function ProcessEditor(context)
    
    -- Update visibility for subsurface scattering...

    local ssEnabled = context:GetMaterialPropertyValue_bool("subsurfaceScattering.enableSubsurfaceScattering")
    
    local commonSsVisibility
    if(ssEnabled) then
        commonSsVisibility = MaterialPropertyVisibility_Enabled
    else
        commonSsVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("subsurfaceScattering.subsurfaceScatterFactor", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.influenceMap", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.useInfluenceMap", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.influenceMapUv", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.scatterColor", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.scatterDistance", commonSsVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.quality", commonSsVisibility)

    UpdateTextureDependentPropertyVisibility(context, ssEnabled, "subsurfaceScattering.influenceMap", "subsurfaceScattering.useInfluenceMap", "subsurfaceScattering.influenceMapUv")
    
    -- Update visibility for transmission...

    local transmissionEnabled = TransmissionMode_None ~= context:GetMaterialPropertyValue_enum("subsurfaceScattering.transmissionMode")
    
    local commonTrasmissionVisibility
    if(transmissionEnabled) then
        commonTrasmissionVisibility = MaterialPropertyVisibility_Enabled
    else
        commonTrasmissionVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("subsurfaceScattering.thickness", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.thicknessMap", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.transmissionTint", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.transmissionPower", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.transmissionDistortion", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.transmissionAttenuation", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("subsurfaceScattering.transmissionScale", commonTrasmissionVisibility)
    
    UpdateTextureDependentPropertyVisibility(context, transmissionEnabled, "subsurfaceScattering.thicknessMap", "subsurfaceScattering.useThicknessMap", "subsurfaceScattering.thicknessMapUv")

end
