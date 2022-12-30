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

-- This functor handles general parallax properties that affect the entire material.

function GetMaterialPropertyDependencies()
    return { 
        "blend.blendSource",
        "blend.enableLayer2",
        "blend.enableLayer3",
        "parallax.enable",
        "parallax.pdo",
        "layer1.parallax.textureMap",
        "layer2.parallax.textureMap",
        "layer3.parallax.textureMap",
        "layer1.parallax.useTexture",
        "layer2.parallax.useTexture",
        "layer3.parallax.useTexture",
        "layer1.parallax.factor",
        "layer2.parallax.factor",
        "layer3.parallax.factor",
        "layer1.parallax.offset",
        "layer2.parallax.offset",
        "layer3.parallax.offset"
    }
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled"}
end

-- These values must align with LayerBlendSource in StandardMultilayerPBR_Common.azsli.
LayerBlendSource_BlendMaskTexture = 0
LayerBlendSource_BlendMaskVertexColors = 1
LayerBlendSource_Displacement = 2
LayerBlendSource_Displacement_With_BlendMaskTexture = 3
LayerBlendSource_Displacement_With_BlendMaskVertexColors = 4

function BlendSourceUsesDisplacement(context)
    local blendSource = context:GetMaterialPropertyValue_enum("blend.blendSource")
    local blendSourceIncludesDisplacement = (blendSource == LayerBlendSource_Displacement or 
                                             blendSource == LayerBlendSource_Displacement_With_BlendMaskTexture or 
                                             blendSource == LayerBlendSource_Displacement_With_BlendMaskVertexColors)
    return blendSourceIncludesDisplacement
end

function IsParallaxNeededForLayer(context, layerNumber)
    local enableLayer = true
    if(layerNumber > 1) then -- layer 1 is always enabled, it is the implicit base layer
        enableLayer = context:GetMaterialPropertyValue_bool("blend.enableLayer" .. layerNumber)
    end

    if not enableLayer then
        return false
    end
    
    local parallaxGroupName = "layer" .. layerNumber .. ".parallax."

    local factor = context:GetMaterialPropertyValue_float(parallaxGroupName .. "factor")
    local offset = context:GetMaterialPropertyValue_float(parallaxGroupName .. "offset")
    
    if factor == 0.0 and offset == 0.0 then
        return false
    end

    local hasTexture = nil ~= context:GetMaterialPropertyValue_Image(parallaxGroupName .. "textureMap")
    local useTexture = context:GetMaterialPropertyValue_bool(parallaxGroupName .. "useTexture")
    
    if not hasTexture or not useTexture then
        factorLayer = 0.0
    end
    
    if factor == 0.0 and offset == 0.0 then
        return false
    end
    
    return true
end

-- Calculates the min and max displacement height values encompassing all enabled layers.
-- @return a table with two values {min,max}. Negative values are below the surface and positive values are above the surface.
function CalcOverallHeightRange(context)
    
    local heightMinMax = {}

    local function GetMergedHeightRange(heightMinMax, offset, factor)
        top = offset
        bottom = offset - factor

        if(heightMinMax[1] == nil) then
            heightMinMax[1] = top
        else
            heightMinMax[1] = math.max(heightMinMax[1], top)
        end
    
        if(heightMinMax[0] == nil) then
            heightMinMax[0] = bottom
        else
            heightMinMax[0] = math.min(heightMinMax[0], bottom)
        end
    end
    
    local enableParallax = context:GetMaterialPropertyValue_bool("parallax.enable")

    if(enableParallax or BlendSourceUsesDisplacement(context)) then
        local hasTextureLayer1 = nil ~= context:GetMaterialPropertyValue_Image("layer1.parallax.textureMap")
        local hasTextureLayer2 = nil ~= context:GetMaterialPropertyValue_Image("layer2.parallax.textureMap")
        local hasTextureLayer3 = nil ~= context:GetMaterialPropertyValue_Image("layer3.parallax.textureMap")
        
        local useTextureLayer1 = context:GetMaterialPropertyValue_bool("layer1.parallax.useTexture")
        local useTextureLayer2 = context:GetMaterialPropertyValue_bool("layer2.parallax.useTexture")
        local useTextureLayer3 = context:GetMaterialPropertyValue_bool("layer3.parallax.useTexture")

        local factorLayer1 = context:GetMaterialPropertyValue_float("layer1.parallax.factor")
        local factorLayer2 = context:GetMaterialPropertyValue_float("layer2.parallax.factor")
        local factorLayer3 = context:GetMaterialPropertyValue_float("layer3.parallax.factor")

        if not hasTextureLayer1 or not useTextureLayer1 then factorLayer1 = 0 end
        if not hasTextureLayer2 or not useTextureLayer2 then factorLayer2 = 0 end
        if not hasTextureLayer3 or not useTextureLayer3 then factorLayer3 = 0 end

        local offsetLayer1 = context:GetMaterialPropertyValue_float("layer1.parallax.offset")
        local offsetLayer2 = context:GetMaterialPropertyValue_float("layer2.parallax.offset")
        local offsetLayer3 = context:GetMaterialPropertyValue_float("layer3.parallax.offset")
        
        local enableLayer2 = context:GetMaterialPropertyValue_bool("blend.enableLayer2")
        local enableLayer3 = context:GetMaterialPropertyValue_bool("blend.enableLayer3")

        GetMergedHeightRange(heightMinMax, offsetLayer1, factorLayer1)
        if(enableLayer2) then GetMergedHeightRange(heightMinMax, offsetLayer2, factorLayer2) end
        if(enableLayer3) then GetMergedHeightRange(heightMinMax, offsetLayer3, factorLayer3) end

    else
        heightMinMax[0] = 0
        heightMinMax[1] = 0
    end

    return heightMinMax
end

function Process(context)
    local heightMinMax = CalcOverallHeightRange(context)
    context:SetShaderConstant_float("m_displacementMin", heightMinMax[0])
    context:SetShaderConstant_float("m_displacementMax", heightMinMax[1])
    
    local parallaxFeatureEnabled = context:GetMaterialPropertyValue_bool("parallax.enable")
    if parallaxFeatureEnabled then
        if not IsParallaxNeededForLayer(context, 1) and
           not IsParallaxNeededForLayer(context, 2) and
           not IsParallaxNeededForLayer(context, 3) then
            parallaxFeatureEnabled = false
        end
    end
    
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", parallaxFeatureEnabled)
    
    local pixelDepthOffsetEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    context:SetInternalMaterialPropertyValue_bool("hasPerPixelDepth", parallaxFeatureEnabled and pixelDepthOffsetEnabled)
end

function ProcessEditor(context)
    local enableParallaxSettings = context:GetMaterialPropertyValue_bool("parallax.enable")
    
    local parallaxSettingVisibility = MaterialPropertyVisibility_Enabled
    if(not enableParallaxSettings) then
        parallaxSettingVisibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("parallax.parallaxUv", parallaxSettingVisibility)
    context:SetMaterialPropertyVisibility("parallax.algorithm", parallaxSettingVisibility)
    context:SetMaterialPropertyVisibility("parallax.quality", parallaxSettingVisibility)
    context:SetMaterialPropertyVisibility("parallax.pdo", parallaxSettingVisibility)
    context:SetMaterialPropertyVisibility("parallax.shadowFactor", parallaxSettingVisibility)
    context:SetMaterialPropertyVisibility("parallax.showClipping", parallaxSettingVisibility)
    
    local pdoEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    if(not pdoEnabled) then
        context:SetMaterialPropertyVisibility("parallax.shadowFactor", MaterialPropertyVisibility_Hidden)
    end

    if BlendSourceUsesDisplacement(context) then
        context:SetMaterialPropertyVisibility("blend.displacementBlendDistance", MaterialPropertyVisibility_Enabled)
    else
        context:SetMaterialPropertyVisibility("blend.displacementBlendDistance", MaterialPropertyVisibility_Hidden)
    end
    
    -- We set the displacementBlendDistance slider range to match the range of displacement, so the slider will feel good
    -- regardless of how big the overall displacement is. Using a soft max allows the user to exceed the limit if desired,
    -- but the main reason for the *soft* max is to avoid impacting the value of displacementBlendDistance which could 
    -- otherwise lead to edge cases.
    local heightMinMax = CalcOverallHeightRange(context)
    local totalDisplacementRange = heightMinMax[1] - heightMinMax[0]
    context:SetMaterialPropertySoftMaxValue_float("blend.displacementBlendDistance", math.max(totalDisplacementRange, 0.001))
end

