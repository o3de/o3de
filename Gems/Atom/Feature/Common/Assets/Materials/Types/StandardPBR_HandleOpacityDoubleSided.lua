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
    return {"opacity.doubleSided"}
end
 
ForwardPassIndex = 0
ForwardPassEdsIndex = 1

function Process(context)
    local doubleSided = context:GetMaterialPropertyValue_bool("opacity.doubleSided")
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
