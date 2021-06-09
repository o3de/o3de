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

function GetMaterialPropertyDependencies()
    return {"parallax.enable", "parallax.pdo"}
end

function Process(context)
    local parallaxEnabled = context:GetMaterialPropertyValue_bool("parallax.enable")
    local parallaxPdoEnabled = context:GetMaterialPropertyValue_bool("parallax.pdo")
    
    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")
    local depthPassWithPS = context:GetShaderByTag("DepthPass_WithPS")
    local shadowMapWithPS = context:GetShaderByTag("Shadowmap_WithPS")
    local forwardPass = context:GetShaderByTag("ForwardPass")
    
    local shadingAffectsDepth = parallaxEnabled and parallaxPdoEnabled;
    
    depthPass:SetEnabled(not shadingAffectsDepth)
    shadowMap:SetEnabled(not shadingAffectsDepth)
    forwardPassEDS:SetEnabled(not shadingAffectsDepth)
        
    depthPassWithPS:SetEnabled(shadingAffectsDepth)
    shadowMapWithPS:SetEnabled(shadingAffectsDepth)
    forwardPass:SetEnabled(shadingAffectsDepth)
end
