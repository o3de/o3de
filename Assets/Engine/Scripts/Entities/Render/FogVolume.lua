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
FogVolume =
{
    type = "FogVolume",

    Properties =
    {
        bActive = 1, --[0,1,1,"If true, fog volume will be enabled."]
        eiVolumeType = 0, --[0,1,1,"Specifies the volume type. The following types are currently supported: 0 - Ellipsoid, 1 - Cube."
        Size = { x = 1, y = 1, z = 1 },
        color_Color = { x = 1, y = 1, z = 1 },
        fHDRDynamic = 0, --[-10,20,0.01,"Specifies how much brighter than the default 255,255,255 white the fog is."]
        bUseGlobalFogColor = 0, --[0,1,1,"If true, the Color property is ignored. Instead, the current global fog color is used."]
        GlobalDensity = 1.0, --[0,1000,1,"Controls the density of the fog. The higher the value the more dense the fog and the less you'll be able to see objects behind or inside the fog volume."]
        DensityOffset = 0.0, --[-1000,1000,1,"Offset fog density, used in conjunction with the GlobalDensity parameter."]
        NearCutoff = 0.0,  --[0,2,0.1,"Stop rendering the object, depending on camera distance to object."]
        FallOffDirLong = 0.0, --[0,360,1,"Controls the longitude of the world space fall off direction of the fog. 0 represents East, rotation is counter-clockwise."]
        FallOffDirLati = 90.0, --[0,360,1,"Controls the latitude of the world space fall off direction of the fog. 90 lets the fall off direction point upwards in world space."]
        FallOffShift = 0.0, --[-100,100,0.1,"Controls how much to shift the fog density distribution along the fall off direction in world units (m)."]
        FallOffScale = 1.0, --[-100,100,0.01,"Scales the density distribution along the fall off direction. Higher values will make the fog fall off more rapidly and generate thicker fog layers along the negative fall off direction."]
        SoftEdges = 1.0, --[0,1,0.01,"Specifies a factor that is used to soften the edges of the fog volume when viewed from outside."]
        RampStart = 0.0, --[0,30000,1.0,"Specifies the start distance of fog density ramp in world units (m)."]
        RampEnd = 50.0, --[0,30000,1.0,"Specifies the end distance of fog density ramp in world units (m)."]
        RampInfluence = 0.0, --[0,1,0.0,"Controls the influence of fog density ramp."]
        WindInfluence = 1.0, --[0,20,0.0,"Controls the influence of the wind."]
        DensityNoiseScale = 1.0, --[0.0,10.0,0.0,"Scales the noise for the density."]
        DensityNoiseOffset = 1.0, --[-2,2,0.0,"Offsets the noise for the density."]
        DensityNoiseTimeFrequency = 0.0, --[0,1,0.0,"Controls the time frequency of the noise for the density."]
        DensityNoiseFrequency = { x = 10, y = 10, z = 10 }, --[1,1000,0.1,"Controls the spatial frequency of the noise for the density."]
        bIgnoresVisAreas = 0, --[0,1,0,"Controls whether this entity should respond to visareas."]
        bAffectsThisAreaOnly = 0, --[0,1,0,"Set this parameter to false to make this entity affect in multiple visareas."]
    },

    Fader = 
    {
        fadeTime = 0.0,
        fadeToValue = 0.0,
    },

    Editor = 
    {
        Model = "Editor/Objects/invisiblebox.cgf",
        Icon = "FogVolume.bmp",
        ShowBounds = 1,
    },
}


function FogVolume:OnSpawn()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
    self:SetFlags(ENTITY_FLAG_NO_PROXIMITY, 0);
end

-------------------------------------------------------
function FogVolume:InitFogVolumeProperties()
    --System.Log( "FogVolume:InitFogVolumeProperties" )
    local props = self.Properties;
    self:LoadFogVolume( 0, self.Properties );
end;

-------------------------------------------------------
function FogVolume:CreateFogVolume()
    --System.Log( "FogVolume:CreateFogVolume" )
    self:InitFogVolumeProperties()
    self.active = true;
end

-------------------------------------------------------
function FogVolume:DeleteFogVolume()
    --System.Log( "FogVolume:DeleteFogVolume" )
    self:FreeSlot( 0 );
    self.active = false;
end

-------------------------------------------------------
function FogVolume:OnInit()
    self.active = false;
    if( self.Properties.bActive == 1 ) then
        self:CreateFogVolume();
    end
end

-------------------------------------------------------
function FogVolume:CheckMove()
end

-------------------------------------------------------
function FogVolume:OnShutDown()
end

-------------------------------------------------------
function FogVolume:OnPropertyChange()
    --System.Log( "FogVolume:OnPropertyChange" )
    if( self.Properties.bActive == 1 ) then
        self:CreateFogVolume();
    else
        self:DeleteFogVolume();
    end
end

-------------------------------------------------------
-- optimization for common animated trackview properties, to avoid fully recreating everything on every animated frame
function FogVolume:OnPropertyAnimated( name )
    local changeTakenCareOf = false;
    if (name=="GlobalDensity") then
        self:FadeGlobalDensity(0, 0, self.Properties.GlobalDensity);  -- using fade with 0 time as there is not a 'set' function.
        changeTakenCareOf = true;
    end
    return changeTakenCareOf;
end


-------------------------------------------------------
function FogVolume:OnReset()
    self.active = false;
    if( self.Properties.bActive == 1 ) then
        self:CreateFogVolume();
    end
end

-------------------------------------------------------
-- Hide Event
-------------------------------------------------------
function FogVolume:Event_Hide()
    self:DeleteFogVolume();
    BroadcastEvent(self, "Hide");
end

-------------------------------------------------------
-- Show Event
-------------------------------------------------------
function FogVolume:Event_Show()
    self:CreateFogVolume();
    BroadcastEvent(self, "Show");
end

-------------------------------------------------------
-- Fade Event
-------------------------------------------------------
function FogVolume:Event_Fade()
    --System.Log("Do Fading");
    self:FadeGlobalDensity(0, self.Fader.fadeTime, self.Fader.fadeToValue);
end

-------------------------------------------------------
-- Fade Time Event
-------------------------------------------------------
function FogVolume:Event_FadeTime(i, time)
    --System.Log("Fade time "..tostring(time));
    self.Fader.fadeTime = time;
end

-------------------------------------------------------
-- Fade Value Event
-------------------------------------------------------
function FogVolume:Event_FadeValue(i, val)
    --System.Log("Fade val "..tostring(val));
    self.Fader.fadeToValue = val;
end

-------------------------------------------------------
-- Set Enabled Event
-------------------------------------------------------
function FogVolume:Event_Enabled(i, enable)
    if (enable) then
        self:CreateFogVolume();
        self:ActivateOutput( "Enabled", true );
    else
        self:DeleteFogVolume();
        self:ActivateOutput( "Enabled", false );
    end
end

-------------------------------------------------------
-- Set GlobalDensity Event
-------------------------------------------------------
function FogVolume:Event_SetGlobalDensity(i, val)
    self.Properties.GlobalDensity = val;
    self:FadeGlobalDensity(0, 0, self.Properties.GlobalDensity);
end

-------------------------------------------------------
-- Set DensityNoiseOffset Event
-------------------------------------------------------
function FogVolume:Event_SetDensityNoiseOffset(i, val)
    self.Properties.DensityNoiseOffset = val;
    self:CreateFogVolume();
end

-------------------------------------------------------
-- Set DensityNoiseScale Event
-------------------------------------------------------
function FogVolume:Event_SetDensityNoiseScale(i, val)
    self.Properties.DensityNoiseScale = val;
    self:CreateFogVolume();
end

-------------------------------------------------------
-- Set WindInfluence Event
-------------------------------------------------------
function FogVolume:Event_SetWindInfluence(i, val)
    self.Properties.WindInfluence = val;
    self:CreateFogVolume();
end

-------------------------------------------------------
-- Serialization
-------------------------------------------------------

function FogVolume:OnLoad(table)
    if(self.active and not table.active) then
        self:DeleteFogVolume();
    elseif(not self.active and table.active) then
        self:CreateFogVolume();
    end
    self.active = table.active;
end

-------------------------------------------------------

function FogVolume:OnSave(table)
    table.active = self.active;
end

-------------------------------------------------------

FogVolume.FlowEvents =
{
    Inputs =
    {
        AO_Enabled  = { FogVolume.Event_Enabled, "bool" },
        EV_Density  = { FogVolume.Event_SetGlobalDensity, "float" },
        EV_DensityNoiseOffset  = { FogVolume.Event_SetDensityNoiseOffset, "float" },
        EV_DensityNoiseScale  = { FogVolume.Event_SetDensityNoiseScale, "float" },
        EV_WindInfluence  = { FogVolume.Event_SetWindInfluence, "float" },
    },
    Outputs =
    {
        Enabled = "bool"
    },
}
