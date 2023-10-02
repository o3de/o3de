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
        "silhouetteType"
        }
end

SilhouetteType_AlwaysDraw = 0
SilhouetteType_Visible = 1
SilhouetteType_XRay = 2
SilhouetteType_NeverDraw = 3

ComparisonFunc_Never = 0
ComparisonFunc_Less = 1
ComparisonFunc_Equal = 2
ComparisonFunc_LessEqual = 3
ComparisonFunc_Greater = 4
ComparisonFunc_NotEqual = 5
ComparisonFunc_GreaterEqual = 6
ComparisonFunc_Always = 7

function Process(context)
    local silhouetteType = context:GetMaterialPropertyValue_enum("silhouetteType")
    local shaderItem = context:GetShaderByTag("SilhouetteGather")
    if(silhouetteType == SilhouetteType_AlwaysDraw) then
        -- "AlwaysDraw" ignores depth check and draws everywhere except where silhouettes are blocked
        shaderItem:GetRenderStatesOverride():SetDepthEnabled(false)
        shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(ComparisonFunc_Never)
        -- if you want to let the full silhouette draw even on top of meshes that block
        -- silhouettes, you can disable the stencil check using SetStencilEnabled()
        -- e.g. shaderItem:GetRenderStatesOverride():SetStencilEnabled(false)
    elseif(silhouetteType == SilhouetteType_Visible) then
        -- "Visible" draws where the silhouette is NOT obscured
        shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
        shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(ComparisonFunc_GreaterEqual)
    elseif(silhouetteType == SilhouetteType_XRay) then
        -- "XRay" draws where the silhouette IS obscured
        shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
        shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(ComparisonFunc_Less)
    elseif(silhouetteType == SilhouetteType_NeverDraw) then
        -- "NeverDraw" doesn't draw the silhouette
        shaderItem:GetRenderStatesOverride():SetDepthEnabled(true)
        shaderItem:GetRenderStatesOverride():SetDepthComparisonFunc(ComparisonFunc_Never)
    end
end
