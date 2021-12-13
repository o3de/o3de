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

function TryGetShaderByTag(context, shaderTag)
    if context:HasShaderWithTag(shaderTag) then
        return context:GetShaderByTag(shaderTag)
    else
        return nil
    end
end

function TrySetShaderEnabled(shader, enabled)
    if shader then 
        shader:SetEnabled(enabled)
    end
end

function Process(context)
    
    local depthPass = context:GetShaderByTag("DepthPass")
    local shadowMap = context:GetShaderByTag("Shadowmap")
    local forwardPassEDS = context:GetShaderByTag("ForwardPass_EDS")

    -- Use TryGetShaderByTag because these shaders only exist in BasePBR but this script is also used for EnhancedPBR
    local lowEndForwardEDS = TryGetShaderByTag(context, "LowEndForward_EDS")

    depthPass:SetEnabled(true);
    shadowMap:SetEnabled(true);
    forwardPassEDS:SetEnabled(true);

    TrySetShaderEnabled(lowEndForwardEDS, true)
end
