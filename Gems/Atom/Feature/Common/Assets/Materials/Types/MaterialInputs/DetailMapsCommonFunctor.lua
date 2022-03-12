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
        "enableDetailLayer", 
        "blendDetailMask", 
        "enableDetailMaskTexture", 
        "enableBaseColor", 
        "baseColorDetailMap", 
        "enableNormals", 
        "normalDetailMap"
        }
end

function GetShaderOptionDependencies()
    return {
        "o_detail_blendMask_useTexture", 
        "o_detail_baseColor_useTexture", 
        "o_detail_normal_useTexture"
        }
end

function Process(context)
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("enableDetailLayer")
    
    local blendMaskTexture = context:GetMaterialPropertyValue_Image("blendDetailMask")
    local blendMaskTextureEnabled = context:GetMaterialPropertyValue_bool("enableDetailMaskTexture")
    context:SetShaderOptionValue_bool("o_detail_blendMask_useTexture", isFeatureEnabled and blendMaskTextureEnabled and blendMaskTexture ~= nil)

    local baseColorDetailEnabled = context:GetMaterialPropertyValue_bool("enableBaseColor")
    local baseColorDetailTexture = context:GetMaterialPropertyValue_Image("baseColorDetailMap")
    context:SetShaderOptionValue_bool("o_detail_baseColor_useTexture", isFeatureEnabled and baseColorDetailEnabled and baseColorDetailTexture ~= nil)
    
    local normalDetailEnabled = context:GetMaterialPropertyValue_bool("enableNormals")
    local normalDetailTexture = context:GetMaterialPropertyValue_Image("normalDetailMap")
    context:SetShaderOptionValue_bool("o_detail_normal_useTexture", isFeatureEnabled and normalDetailEnabled and normalDetailTexture ~= nil)
end

function ProcessEditor(context)
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("enableDetailLayer")
    
    mainVisibility = MaterialPropertyVisibility_Enabled
    if(not isFeatureEnabled) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    end

    context:SetMaterialPropertyVisibility("blendDetailFactor", mainVisibility)
    context:SetMaterialPropertyVisibility("blendDetailMask", mainVisibility)
    context:SetMaterialPropertyVisibility("enableDetailMaskTexture", mainVisibility)
    context:SetMaterialPropertyVisibility("blendDetailMaskUv", mainVisibility)
    context:SetMaterialPropertyVisibility("textureMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("enableBaseColor", mainVisibility)
    context:SetMaterialPropertyVisibility("baseColorDetailMap", mainVisibility)
    context:SetMaterialPropertyVisibility("baseColorDetailBlend", mainVisibility)
    context:SetMaterialPropertyVisibility("enableNormals", mainVisibility)
    context:SetMaterialPropertyVisibility("normalDetailStrength", mainVisibility)
    context:SetMaterialPropertyVisibility("normalDetailMap", mainVisibility)
    context:SetMaterialPropertyVisibility("normalDetailFlipX", mainVisibility)
    context:SetMaterialPropertyVisibility("normalDetailFlipY", mainVisibility)
        
    context:SetMaterialPropertyVisibility("detailUV.center", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.tileU", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.tileV", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.offsetU", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.offsetV", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.rotateDegrees", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.scale", mainVisibility)
    
    local blendMaskTexture = context:GetMaterialPropertyValue_Image("blendDetailMask")
    if(nil == blendMaskTexture) then
        context:SetMaterialPropertyVisibility("enableDetailMaskTexture", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("blendDetailMaskUv", MaterialPropertyVisibility_Hidden)
    end
    
    local baseColorDetailEnabled = context:GetMaterialPropertyValue_bool("enableBaseColor")
    if(not baseColorDetailEnabled) then
        context:SetMaterialPropertyVisibility("baseColorDetailMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("baseColorDetailBlend", MaterialPropertyVisibility_Hidden)
    end
    
    local normalDetailEnabled = context:GetMaterialPropertyValue_bool("enableNormals")
    if(not normalDetailEnabled) then
        context:SetMaterialPropertyVisibility("normalDetailStrength", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("normalDetailMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("normalDetailFlipX", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("normalDetailFlipY", MaterialPropertyVisibility_Hidden)
    end

end
