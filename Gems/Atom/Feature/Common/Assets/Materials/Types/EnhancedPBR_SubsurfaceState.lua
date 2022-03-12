--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {
        "enableSubsurfaceScattering", 
        "influenceMap", 
        "useInfluenceMap", 
        "transmissionMode", 
        "thicknessMap", 
        "useThicknessMap", 
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
    local ssEnabled = context:GetMaterialPropertyValue_bool("enableSubsurfaceScattering")
    local transmissionEnabled = TransmissionMode_None ~= context:GetMaterialPropertyValue_enum("transmissionMode")
    UpdateUseTextureState(context, ssEnabled, "influenceMap", "useInfluenceMap", "o_subsurfaceScattering_useTexture")
    UpdateUseTextureState(context, transmissionEnabled, "thicknessMap", "useThicknessMap", "o_transmission_useTexture")
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

    local ssEnabled = context:GetMaterialPropertyValue_bool("enableSubsurfaceScattering")
    
    local commonSsVisibility
    if(ssEnabled) then
        commonSsVisibility = MaterialPropertyVisibility_Enabled
    else
        commonSsVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("subsurfaceScatterFactor", commonSsVisibility)
    context:SetMaterialPropertyVisibility("influenceMap", commonSsVisibility)
    context:SetMaterialPropertyVisibility("useInfluenceMap", commonSsVisibility)
    context:SetMaterialPropertyVisibility("influenceMapUv", commonSsVisibility)
    context:SetMaterialPropertyVisibility("scatterColor", commonSsVisibility)
    context:SetMaterialPropertyVisibility("scatterDistance", commonSsVisibility)
    context:SetMaterialPropertyVisibility("quality", commonSsVisibility)

    UpdateTextureDependentPropertyVisibility(context, ssEnabled, "influenceMap", "useInfluenceMap", "influenceMapUv")
    
    -- Update visibility for transmission...

    local thickTransmissionEnabled = TransmissionMode_ThickObject == context:GetMaterialPropertyValue_enum("transmissionMode")
    local thinTransmissionEnabled = TransmissionMode_ThinObject == context:GetMaterialPropertyValue_enum("transmissionMode")

    local commonTrasmissionVisibility = MaterialPropertyVisibility_Hidden
    local thickTransmissionVisibility = MaterialPropertyVisibility_Hidden
    local thinTransmissionVisibility = MaterialPropertyVisibility_Hidden
    if (thickTransmissionEnabled or thinTransmissionEnabled) then
        commonTrasmissionVisibility = MaterialPropertyVisibility_Enabled

            if(thickTransmissionEnabled) then
                thickTransmissionVisibility = MaterialPropertyVisibility_Enabled
            else -- thin transmission enabled
                thinTransmissionVisibility = MaterialPropertyVisibility_Enabled
            end
    end
    
    context:SetMaterialPropertyVisibility("thickness", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("thicknessMap", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionTint", commonTrasmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionPower", thickTransmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionDistortion", thickTransmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionAttenuation", thickTransmissionVisibility)
    context:SetMaterialPropertyVisibility("shrinkFactor", thinTransmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionNdLBias", thinTransmissionVisibility)
    context:SetMaterialPropertyVisibility("distanceAttenuation", thinTransmissionVisibility)
    context:SetMaterialPropertyVisibility("transmissionScale", commonTrasmissionVisibility)
    
    
    UpdateTextureDependentPropertyVisibility(context, thickTransmissionEnabled or thinTransmissionEnabled, "thicknessMap", "useThicknessMap", "thicknessMapUv")

end
