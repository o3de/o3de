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
--
----------------------------------------------------------------------------------------------------
AreaBezierVolume = {

  type = "AreaBezierVolume",
  Properties = 
  {
      bEnabled = 1,      
    MultiplayerOptions = {
        bNetworked        = 0,
    },
  },
}

-------------------------------------------------------------------------------------------------------
function AreaBezierVolume:OnSpawn()
    if (self.Properties.MultiplayerOptions.bNetworked == 0) then
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end
end

-------------------------------------------------------
function AreaBezierVolume:OnLoad(table)
  self.bEnabled = table.bEnabled
    if(self.bEnabled == 1) then
        self:Event_Enable();
    else
        self:Event_Disable();
    end
end

-------------------------------------------------------
function AreaBezierVolume:OnSave(table)
  table.bEnabled = self.bEnabled
end


-------------------------------------------------------
function AreaBezierVolume:OnInit()
    if(self.Properties.bEnabled == 1) then
        self:Event_Enable();
    else
        self:Event_Disable();
    end
end

function AreaBezierVolume:OnPropertyChange()
    if(self.Properties.bEnabled == 1) then
        self:Event_Enable();
    else
        self:Event_Disable();
    end
end


-------------------------------------------------------
function AreaBezierVolume:OnEnable(enable)
    --Log("AreaBezierVolume:OnEnable");
    self:SetPhysicParams(PHYSICPARAM_FOREIGNDATA,{foreignData = ZEROG_AREA_ID});
end

-------------------------------------------------------
function AreaBezierVolume:Event_Enable()
    --self:TriggerEvent(AIEVENT_ENABLE);
    self.bEnabled = 1;
    BroadcastEvent(self, "Enable");
end

-------------------------------------------------------
function AreaBezierVolume:Event_Disable()
    --self:TriggerEvent(AIEVENT_DISABLE);
    self.bEnabled = 0;
    BroadcastEvent(self, "Disable");
end

AreaBezierVolume.FlowEvents =
{
    Inputs =
    {
        Disable = { AreaBezierVolume.Event_Disable, "bool" },
        Enable = { AreaBezierVolume.Event_Enable, "bool" },
    },
    Outputs =
    {
        Disable = "bool",
        Enable = "bool",
    },
}