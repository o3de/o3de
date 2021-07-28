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
WaterVolume = 
{
    type = "WaterVolume",
    
    Properties = 
    {
        StreamSpeed = 0,
        FogDensity = 0.5,
        color_FogColor = {x=0.005,y=0.01,z=0.02},
        FogColorMultiplier = 0.5,
        bFogColorAffectedBySun = 1,
        FogShadowing = 0.5,
        bCapFogAtVolumeDepth = 0,
        bCaustics = 1,
        CausticIntensity = 1,
        CausticTiling = 1,
        CausticHeight = 0.5,
        UScale = 1,
        VScale = 1,
        Depth = 5,
        ViewDistancemMultiplier = 1.0, 
        MinSpec = 0,
        MaterialLayerMask = 0,
        bAwakeAreaWhenMoving = 0, --[0,1,1,"Entities in area are physically awake when game volume is moving"]
        bIsRiver = 0,
        MultiplayerOptions = 
        {
            bNetworked        = 0,
        },
    },

    Editor = 
    {
        Model = "Editor/Objects/T.cgf",
        Icon = "Water.bmp",
        ShowBounds = 1,
        IsScalable = false;
        IsRotatable = true;
    },
}

-------------------------------------------------------------------------------------------------------
function WaterVolume:OnSpawn()
    if (self.Properties.MultiplayerOptions.bNetworked == 0) then
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end
end

-------------------------------------------------------------------------------------------------------
function WaterVolume:OnPropertyChange()

end

-------------------------------------------------------------------------------------------------------
function WaterVolume:IsShapeOnly()
    return 1;
end

-------------------------------------------------------------------------------------------------------
function WaterVolume:Event_Hide()
    self:Hide(1);
    self:ActivateOutput( "Hidden", true );
end;

-------------------------------------------------------------------------------------------------------
function WaterVolume:Event_UnHide()
    self:Hide(0);
    self:ActivateOutput( "UnHidden", true );
end;

-------------------------------------------------------------------------------------------------------
function WaterVolume:Event_PhysicsEnable()
    Game.SendEventToGameObject( self.id, "PhysicsEnable" );
end;

-------------------------------------------------------------------------------------------------------
function WaterVolume:Event_PhysicsDisable()
    Game.SendEventToGameObject( self.id, "PhysicsDisable" );
end;

WaterVolume.FlowEvents =
{
    Inputs =
    {
        Hide = { WaterVolume.Event_Hide, "bool" },
        UnHide = { WaterVolume.Event_UnHide, "bool" },
        PhysicsEnable = { WaterVolume.Event_PhysicsEnable, "bool" },
        PhysicsDisable = { WaterVolume.Event_PhysicsDisable, "bool" },
    },
    Outputs =
    {
        Hidden = "bool",
        UnHidden = "bool",
    },
}


