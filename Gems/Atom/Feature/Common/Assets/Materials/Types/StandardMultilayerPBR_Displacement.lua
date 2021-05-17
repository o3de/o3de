--------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        "layer1_parallax.textureMap",
        "layer2_parallax.textureMap",
        "layer3_parallax.textureMap",
        "layer1_parallax.useTexture",
        "layer2_parallax.useTexture",
        "layer3_parallax.useTexture",
        "layer1_parallax.factor",
        "layer2_parallax.factor",
        "layer3_parallax.factor",
        "layer1_parallax.offset",
        "layer2_parallax.offset",
        "layer3_parallax.offset"
    }
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

-- Calculates the min and max displacement height values encompassing all enabled layers.
-- @return a table with two values {min,max}. Negative values are below the surface and positive values are above the surface.
function CalcOverallHeightRange(context)
    
    local heightMinMax = {nil, nil}

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
        local hasTextureLayer1 = nil ~= context:GetMaterialPropertyValue_Image("layer1_parallax.textureMap")
        local hasTextureLayer2 = nil ~= context:GetMaterialPropertyValue_Image("layer2_parallax.textureMap")
        local hasTextureLayer3 = nil ~= context:GetMaterialPropertyValue_Image("layer3_parallax.textureMap")
        
        local useTextureLayer1 = context:GetMaterialPropertyValue_bool("layer1_parallax.useTexture")
        local useTextureLayer2 = context:GetMaterialPropertyValue_bool("layer2_parallax.useTexture")
        local useTextureLayer3 = context:GetMaterialPropertyValue_bool("layer3_parallax.useTexture")

        local factorLayer1 = context:GetMaterialPropertyValue_float("layer1_parallax.factor")
        local factorLayer2 = context:GetMaterialPropertyValue_float("layer2_parallax.factor")
        local factorLayer3 = context:GetMaterialPropertyValue_float("layer3_parallax.factor")

        if not hasTextureLayer1 or not useTextureLayer1 then factorLayer1 = 0 end
        if not hasTextureLayer2 or not useTextureLayer2 then factorLayer2 = 0 end
        if not hasTextureLayer3 or not useTextureLayer3 then factorLayer3 = 0 end

        local offsetLayer1 = context:GetMaterialPropertyValue_float("layer1_parallax.offset")
        local offsetLayer2 = context:GetMaterialPropertyValue_float("layer2_parallax.offset")
        local offsetLayer3 = context:GetMaterialPropertyValue_float("layer3_parallax.offset")
        
        local enableLayer2 = context:GetMaterialPropertyValue_bool("blend.enableLayer2")
        local enableLayer3 = context:GetMaterialPropertyValue_bool("blend.enableLayer3")

        GetMergedHeightRange(heightMinMax, offsetLayer1, factorLayer1)
        if(enableLayer2) then GetMergedHeightRange(heightMinMax, offsetLayer2, factorLayer2) end
        if(enableLayer3) then GetMergedHeightRange(heightMinMax, offsetLayer3, factorLayer3) end

    else
        heightMinMax = {0,0}
    end

    return heightMinMax
end

function Process(context)
    local heightMinMax = CalcOverallHeightRange(context)
    context:SetShaderConstant_float("m_displacementMin", heightMinMax[0])
    context:SetShaderConstant_float("m_displacementMax", heightMinMax[1])
end

function ProcessEditor(context)
    local enable = context:GetMaterialPropertyValue_bool("parallax.enable")
    
    local visibility = MaterialPropertyVisibility_Enabled
    if(not enable) then
        visibility = MaterialPropertyVisibility_Hidden
    end
    
    context:SetMaterialPropertyVisibility("parallax.parallaxUv", visibility)
    context:SetMaterialPropertyVisibility("parallax.algorithm", visibility)
    context:SetMaterialPropertyVisibility("parallax.quality", visibility)
    context:SetMaterialPropertyVisibility("parallax.pdo", visibility)
    context:SetMaterialPropertyVisibility("parallax.showClipping", visibility)
    
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

