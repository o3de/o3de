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
        "detailLayerGroup.enableDetailLayer", 
        "detailLayerGroup.blendDetailMask", 
        "detailLayerGroup.enableDetailMaskTexture", 
        "detailLayerGroup.enableBaseColor", 
        "detailLayerGroup.baseColorDetailMap", 
        "detailLayerGroup.enableNormals", 
        "detailLayerGroup.normalDetailMap"
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
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableDetailLayer")
    
    local blendMaskTexture = context:GetMaterialPropertyValue_Image("detailLayerGroup.blendDetailMask")
    local blendMaskTextureEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableDetailMaskTexture")
    context:SetShaderOptionValue_bool("o_detail_blendMask_useTexture", isFeatureEnabled and blendMaskTextureEnabled and blendMaskTexture ~= nil)

    local baseColorDetailEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableBaseColor")
    local baseColorDetailTexture = context:GetMaterialPropertyValue_Image("detailLayerGroup.baseColorDetailMap")
    context:SetShaderOptionValue_bool("o_detail_baseColor_useTexture", isFeatureEnabled and baseColorDetailEnabled and baseColorDetailTexture ~= nil)
    
    local normalDetailEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableNormals")
    local normalDetailTexture = context:GetMaterialPropertyValue_Image("detailLayerGroup.normalDetailMap")
    context:SetShaderOptionValue_bool("o_detail_normal_useTexture", isFeatureEnabled and normalDetailEnabled and normalDetailTexture ~= nil)
end

function ProcessEditor(context)
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableDetailLayer")
    
    mainVisibility = MaterialPropertyVisibility_Enabled
    if(not isFeatureEnabled) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    end

    context:SetMaterialPropertyVisibility("detailLayerGroup.blendDetailFactor", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.blendDetailMask", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.enableDetailMaskTexture", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.blendDetailMaskUv", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.textureMapUv", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.enableBaseColor", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.baseColorDetailMap", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.baseColorDetailBlend", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.enableNormals", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailStrength", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailMap", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailFlipX", mainVisibility)
    context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailFlipY", mainVisibility)
        
    context:SetMaterialPropertyVisibility("detailUV.center", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.tileU", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.tileV", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.offsetU", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.offsetV", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.rotateDegrees", mainVisibility)
    context:SetMaterialPropertyVisibility("detailUV.scale", mainVisibility)
    
    local blendMaskTexture = context:GetMaterialPropertyValue_Image("detailLayerGroup.blendDetailMask")
    if(nil == blendMaskTexture) then
        context:SetMaterialPropertyVisibility("detailLayerGroup.enableDetailMaskTexture", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("detailLayerGroup.blendDetailMaskUv", MaterialPropertyVisibility_Hidden)
    end
    
    local baseColorDetailEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableBaseColor")
    if(not baseColorDetailEnabled) then
        context:SetMaterialPropertyVisibility("detailLayerGroup.baseColorDetailMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("detailLayerGroup.baseColorDetailBlend", MaterialPropertyVisibility_Hidden)
    end
    
    local normalDetailEnabled = context:GetMaterialPropertyValue_bool("detailLayerGroup.enableNormals")
    if(not normalDetailEnabled) then
        context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailStrength", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailMap", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailFlipX", MaterialPropertyVisibility_Hidden)
        context:SetMaterialPropertyVisibility("detailLayerGroup.normalDetailFlipY", MaterialPropertyVisibility_Hidden)
    end

end
