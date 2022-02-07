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
        "wrinkleLayers.enable", 
        "wrinkleLayers.count", 
        "wrinkleLayers.enableBaseColor", 
        "wrinkleLayers.enableNormal",
        "wrinkleLayers.showBlendValues",
        "wrinkleLayers.baseColorMap1",
        "wrinkleLayers.baseColorMap2",
        "wrinkleLayers.baseColorMap3",
        "wrinkleLayers.baseColorMap4",
        "wrinkleLayers.normalMap1",
        "wrinkleLayers.normalMap2",
        "wrinkleLayers.normalMap3",
        "wrinkleLayers.normalMap4"
        }
end

function GetShaderOptionDependencies()
    return {
        "o_wrinkleLayers_enabled",
        "o_wrinkleLayers_count",
        "o_wrinkleLayers_baseColor_enabled", 
        "o_wrinkleLayers_baseColor_useTexture1",
        "o_wrinkleLayers_baseColor_useTexture2",
        "o_wrinkleLayers_baseColor_useTexture3",
        "o_wrinkleLayers_baseColor_useTexture4",
        "o_wrinkleLayers_normal_enabled",
        "o_wrinkleLayers_normal_useTexture1",
        "o_wrinkleLayers_normal_useTexture2",
        "o_wrinkleLayers_normal_useTexture3",
        "o_wrinkleLayers_normal_useTexture4",
        "o_wrinkleLayers_showBlendMaskValues"
        }
end

MAX_WRINKLE_LAYER_COUNT = 4

function Process(context)
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("wrinkleLayers.enable")
    local isBaseColorEnabled = isFeatureEnabled and context:GetMaterialPropertyValue_bool("wrinkleLayers.enableBaseColor")
    local isNormalEnabled = isFeatureEnabled and context:GetMaterialPropertyValue_bool("wrinkleLayers.enableNormal")
    local showBlendValues = isFeatureEnabled and context:GetMaterialPropertyValue_bool("wrinkleLayers.showBlendValues")
    context:SetShaderOptionValue_bool("o_wrinkleLayers_enabled", isFeatureEnabled)
    context:SetShaderOptionValue_bool("o_wrinkleLayers_baseColor_enabled", isBaseColorEnabled)
    context:SetShaderOptionValue_bool("o_wrinkleLayers_normal_enabled", isNormalEnabled)
    context:SetShaderOptionValue_bool("o_wrinkleLayers_showBlendMaskValues", showBlendValues)

    local count = context:GetMaterialPropertyValue_uint("wrinkleLayers.count")
    context:SetShaderOptionValue_uint("o_wrinkleLayers_count", count)

    for i=1,MAX_WRINKLE_LAYER_COUNT do
        if(i <= count) then
            isBaseColorTextureMissing = isBaseColorEnabled and nil == context:GetMaterialPropertyValue_Image("wrinkleLayers.baseColorMap" .. i)
            isNormalTextureMissing = isNormalEnabled and nil == context:GetMaterialPropertyValue_Image("wrinkleLayers.normalMap" .. i)
            context:SetShaderOptionValue_bool("o_wrinkleLayers_baseColor_useTexture" .. i, not isBaseColorTextureMissing)
            context:SetShaderOptionValue_bool("o_wrinkleLayers_normal_useTexture" .. i, not isNormalTextureMissing)
        else
            context:SetShaderOptionValue_bool("o_wrinkleLayers_baseColor_useTexture" .. i, false)
            context:SetShaderOptionValue_bool("o_wrinkleLayers_normal_useTexture" .. i, false)
        end
    end
end

function ProcessEditor(context)
    local isFeatureEnabled = context:GetMaterialPropertyValue_bool("wrinkleLayers.enable")

    mainVisibility = MaterialPropertyVisibility_Enabled
    if(not isFeatureEnabled) then
        mainVisibility = MaterialPropertyVisibility_Hidden
    end
    
    local count = context:GetMaterialPropertyValue_uint("wrinkleLayers.count")

    context:SetMaterialPropertyVisibility("wrinkleLayers.count", mainVisibility)
    context:SetMaterialPropertyVisibility("wrinkleLayers.enableBaseColor", mainVisibility)
    context:SetMaterialPropertyVisibility("wrinkleLayers.enableNormal", mainVisibility)
    context:SetMaterialPropertyVisibility("wrinkleLayers.showBlendValues", mainVisibility)

    for i=1,MAX_WRINKLE_LAYER_COUNT do
        context:SetMaterialPropertyVisibility("wrinkleLayers.baseColorMap" .. i, mainVisibility)
        context:SetMaterialPropertyVisibility("wrinkleLayers.normalMap" .. i, mainVisibility)
    end

    local isBaseColorEnabled = context:GetMaterialPropertyValue_bool("wrinkleLayers.enableBaseColor")
    local isNormalEnabled = context:GetMaterialPropertyValue_bool("wrinkleLayers.enableNormal")

    for i=1,MAX_WRINKLE_LAYER_COUNT do
        if(i > count or not isBaseColorEnabled) then
            context:SetMaterialPropertyVisibility("wrinkleLayers.baseColorMap" .. i, MaterialPropertyVisibility_Hidden)
        end
        if(i > count or not isNormalEnabled) then
            context:SetMaterialPropertyVisibility("wrinkleLayers.normalMap" .. i, MaterialPropertyVisibility_Hidden)
        end
    end
end
