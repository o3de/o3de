----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local DisplayMouseCursor = 
{
    Properties = 
    {
    },
}

function DisplayMouseCursor:OnActivate()
    -- Display the mouse cursor
    LyShineLua.ShowMouseCursor(true)
end

function DisplayMouseCursor:OnDeactivate()
    -- Hide the mouse cursor
    LyShineLua.ShowMouseCursor(false)
end

return DisplayMouseCursor
