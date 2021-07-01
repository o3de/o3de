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
