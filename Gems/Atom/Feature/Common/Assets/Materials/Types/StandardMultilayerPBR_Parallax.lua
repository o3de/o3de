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
        "parallax.enable",
        "layer1_parallax.enable",
        "layer2_parallax.enable",
        "layer3_parallax.enable",
        "layer1_parallax.factor",
        "layer2_parallax.factor",
        "layer3_parallax.factor",
        "layer1_parallax.offset",
        "layer2_parallax.offset",
        "layer3_parallax.offset" 
    }
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled"}
end
 
function GetMergedHeightRange(heightMinMax, offset, factor)
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

-- These values must align with LayerBlendSource in StandardMultilayerPBR_Common.azsli.
LayerBlendSource_BlendMaskTexture = 0
LayerBlendSource_BlendMaskVertexColors = 1
LayerBlendSource_Displacement = 2
LayerBlendSource_Displacement_With_BlendMaskTexture = 3
LayerBlendSource_Displacement_With_BlendMaskVertexColors = 4

function Process(context)
    local enableParallax = context:GetMaterialPropertyValue_bool("parallax.enable")
    local enable1 = context:GetMaterialPropertyValue_bool("layer1_parallax.enable")
    local enable2 = context:GetMaterialPropertyValue_bool("layer2_parallax.enable")
    local enable3 = context:GetMaterialPropertyValue_bool("layer3_parallax.enable")
    enableParallax = enableParallax and (enable1 or enable2 or enable3)
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", enableParallax)
    
    blendSource = context:GetMaterialPropertyValue_enum("blend.blendSource")
    blendSourceIncludesDisplacement = (blendSource == LayerBlendSource_Displacement or 
                                       blendSource ==LayerBlendSource_Displacement_With_BlendMaskTexture or 
                                       blendSource == LayerBlendSource_Displacement_With_BlendMaskVertexColors)

    if(enableParallax or blendSourceIncludesDisplacement) then
        local factorLayer1 = context:GetMaterialPropertyValue_float("layer1_parallax.factor")
        local factorLayer2 = context:GetMaterialPropertyValue_float("layer2_parallax.factor")
        local factorLayer3 = context:GetMaterialPropertyValue_float("layer3_parallax.factor")

        local offsetLayer1 = context:GetMaterialPropertyValue_float("layer1_parallax.offset")
        local offsetLayer2 = context:GetMaterialPropertyValue_float("layer2_parallax.offset")
        local offsetLayer3 = context:GetMaterialPropertyValue_float("layer3_parallax.offset")

        local heightMinMax = {nil, nil}
        if(enable1) then GetMergedHeightRange(heightMinMax, offsetLayer1, factorLayer1) end
        if(enable2) then GetMergedHeightRange(heightMinMax, offsetLayer2, factorLayer2) end
        if(enable3) then GetMergedHeightRange(heightMinMax, offsetLayer3, factorLayer3) end

        if(heightMinMax[1] - heightMinMax[0] < 0.0001) then
            context:SetShaderOptionValue_bool("o_parallax_feature_enabled", false)
        else
            context:SetShaderConstant_float("m_displacementMin", heightMinMax[0])
            context:SetShaderConstant_float("m_displacementMax", heightMinMax[1])
        end
    end
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
end
