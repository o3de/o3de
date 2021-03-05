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
        "parallax.enable",
        "layer1_parallax.enable",
        "layer2_parallax.enable",
        "layer3_parallax.enable",
        "parallax.factor",
        "layer1_parallax.factor",
        "layer2_parallax.factor",
        "layer3_parallax.factor" 
    }
end

function GetShaderOptionDependencies()
    return {"o_parallax_feature_enabled"}
end
 
function Process(context)
    local enableParallax = context:GetMaterialPropertyValue_bool("parallax.enable")
    local enable1 = context:GetMaterialPropertyValue_bool("layer1_parallax.enable")
    local enable2 = context:GetMaterialPropertyValue_bool("layer2_parallax.enable")
    local enable3 = context:GetMaterialPropertyValue_bool("layer3_parallax.enable")
    enableParallax = enableParallax and (enable1 or enable2 or enable3)
    context:SetShaderOptionValue_bool("o_parallax_feature_enabled", enableParallax)
    
    -- Smaller values for the main parallax factor used in GetParallaxOffset() give better quality.
    -- So increase the per-layer parallax factors by normalizing them, and reduce the main factor accordingly.
    if(enableParallax) then
        local factorLayer1 = context:GetMaterialPropertyValue_float("layer1_parallax.factor")
        local factorLayer2 = context:GetMaterialPropertyValue_float("layer2_parallax.factor")
        local factorLayer3 = context:GetMaterialPropertyValue_float("layer3_parallax.factor")
        local mainFactor = context:GetMaterialPropertyValue_float("parallax.factor")

        maxLayerFactor = 0.0
        if(enable1) then maxLayerFactor = math.max(maxLayerFactor, factorLayer1) end
        if(enable2) then maxLayerFactor = math.max(maxLayerFactor, factorLayer2) end
        if(enable3) then maxLayerFactor = math.max(maxLayerFactor, factorLayer3) end

        if(maxLayerFactor < 0.0001) then
            context:SetShaderOptionValue_bool("o_parallax_feature_enabled", false)
        else
            factorLayer1 = factorLayer1 / maxLayerFactor
            factorLayer2 = factorLayer2 / maxLayerFactor
            factorLayer3 = factorLayer3 / maxLayerFactor
            mainFactor = mainFactor * maxLayerFactor;
            context:SetShaderConstant_float("m_layer1_m_depthFactor", factorLayer1)
            context:SetShaderConstant_float("m_layer2_m_depthFactor", factorLayer2)
            context:SetShaderConstant_float("m_layer3_m_depthFactor", factorLayer3)
            context:SetShaderConstant_float("m_parallaxMainDepthFactor", mainFactor)
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
    context:SetMaterialPropertyVisibility("parallax.factor", visibility)
    context:SetMaterialPropertyVisibility("parallax.algorithm", visibility)
    context:SetMaterialPropertyVisibility("parallax.quality", visibility)
    context:SetMaterialPropertyVisibility("parallax.pdo", visibility)
end
