----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript("scripts/Utils/EntityUtils.lua")

RopeEntity =
{
    Properties=
    {
        MultiplayerOptions = {
            bNetworked        = 0,
        },
    },
}

------------------------------------------------------------------------------------------------------
function RopeEntity:OnSpawn()
    if (self.Properties.MultiplayerOptions.bNetworked == 0) then
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end
end

------------------------------------------------------------------------------------------------------
function RopeEntity:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
    self:ActivateOutput("Break",nPartId+1 );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_Remove()
    self:DrawSlot(0,0);
    self:DestroyPhysics();
    self:ActivateOutput( "Remove", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_Hide()
    self:Hide(1);
    self:ActivateOutput( "Hide", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_UnHide()
    self:Hide(0);
    self:ActivateOutput( "UnHide", true );
end

------------------------------------------------------------------------------------------------------
function RopeEntity:Event_BreakStart( vPos,nPartId,nOtherPartId )
    local RopeParams = {}
    RopeParams.entity_name_1 = "#unattached";

    self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_BreakEnd( vPos,nPartId,nOtherPartId )
    local RopeParams = {}
    RopeParams.entity_name_2 = "#unattached";

    self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_BreakDist( sender, dist )
    local RopeParams = {}
    RopeParams.break_point = dist;

    self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_Disable()
    local RopeParams = {}
    RopeParams.bDisabled = 1;
    self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end
function RopeEntity:Event_Enable()
    local RopeParams = {}
    RopeParams.bDisabled = 0;
    self:SetPhysicParams(PHYSICPARAM_ROPE,RopeParams);
end


RopeEntity.FlowEvents =
{
    Inputs =
    {
        Hide = { RopeEntity.Event_Hide, "bool" },
        UnHide = { RopeEntity.Event_UnHide, "bool" },
        Remove = { RopeEntity.Event_Remove, "bool" },
        BreakStart = { RopeEntity.Event_BreakStart, "bool" },
    BreakEnd = { RopeEntity.Event_BreakEnd, "bool" },
        BreakDist = { RopeEntity.Event_BreakDist, "float" },
        Disable = { RopeEntity.Event_Disable, "bool" },
        Enable = { RopeEntity.Event_Enable, "bool" },
    },
    Outputs =
    {
        Hide = "bool",
        UnHide = "bool",
        Remove = "bool",
        Break = "int",
    },
}

