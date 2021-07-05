----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
--
----------------------------------------------------------------------------------------------------
SmartObject = {
  type = "SmartObject",
  Properties = 
  {
        soclasses_SmartObjectClass = "",
  },
    
    Editor={
    Model="Editor/Objects/anchor.cgf",
    Icon="smartobject.bmp",
    IconOnTop=1,
    },
}


-------------------------------------------------------
function SmartObject:OnInit()
end

-------------------------------------------------------
function SmartObject:OnReset()
end

-------------------------------------------------------
function SmartObject:OnUsed()
    -- this function will be called from ACT_USEOBJECT
    BroadcastEvent(self, "Used");
end

-------------------------------------------------------
function SmartObject:OnNavigationStarted(userId)
    self:ActivateOutput("NavigationStarted", userId or NULL_ENTITY)
end

-------------------------------------------------------
function SmartObject:Event_Used( sender )
    BroadcastEvent(self, "Used");
end

SmartObject.FlowEvents =
{
    Inputs =
    {
        Used = { SmartObject.Event_Used, "bool" },
    },
    Outputs =
    {
        Used = "bool",
        NavigationStarted = "entity",
    },
}
