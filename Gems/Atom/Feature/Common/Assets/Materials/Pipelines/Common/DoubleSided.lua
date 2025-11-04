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
    return {"doubleSided"}
end
 
function Process(context)
    local doubleSided = context:GetMaterialPropertyValue_bool("doubleSided")
    local lastShader = context:GetShaderCount() - 1;

    if(doubleSided) then
        for i=0,lastShader do
            context:GetShader(i):GetRenderStatesOverride():SetCullMode(CullMode_None)
        end
    else
        for i=0,lastShader do
            context:GetShader(i):GetRenderStatesOverride():ClearCullMode()
        end
    end
end
