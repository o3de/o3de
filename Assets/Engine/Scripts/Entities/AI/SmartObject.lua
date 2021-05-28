----------------------------------------------------------------------------------------------------
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
-- Original file Copyright Crytek GMBH or its affiliates, used under license.
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
