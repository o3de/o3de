----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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