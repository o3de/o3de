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
Script.ReloadScript("Scripts/Entities/Lights/Light.lua")

EnvironmentLight =
{
    Properties =
    {
        _nVersion = -1,
        bActive = 0,
        BoxSizeX = 10,
        BoxSizeY = 10,
        BoxSizeZ = 10,
        Color = 
        {
            clrDiffuse = { x=1,y=1,z=1 },
            fDiffuseMultiplier = 1,
            fSpecularMultiplier = 1,
        },
        Projection = 
        {
            bBoxProject = 0,
            fBoxWidth = 10,
            fBoxHeight = 10,
            fBoxLength = 10,
        },
        Options = 
        {
            bAffectsThisAreaOnly = 1,
            bIgnoresVisAreas = 0,
            bDeferredClipBounds = 0,
            _texture_deferred_cubemap = "",
            SortPriority = 0,
            fAttenuationFalloffMax = 0.3,
            bVolumetricFog = 1, --[0,1,1,"Enables the light to affect volumetric fog."]
            bAffectsVolumetricFogOnly = 0, --[0,1,0,"Enables the light to affect only volumetric fog."]
        },
        OptionsAdvanced = 
        {
            texture_deferred_cubemap = "",
        },
    },

    Editor =
    {
        ShowBounds = 0,
        AbsoluteRadius = 1,
    },

    _LightTable = {},
}

LightSlot = 1

function EnvironmentLight:OnInit()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
    self:OnReset();
    self:Activate(1); -- Force OnUpdate to get called
    self:CacheResources("EnvironmentLight.lua");
end

function EnvironmentLight:CacheResources(requesterName)
    if ( self.Properties.OptionsAdvanced.texture_deferred_cubemap == "" ) then
        self.Properties.OptionsAdvanced.texture_deferred_cubemap = self.Properties.Options._texture_deferred_cubemap;
    end
end

function EnvironmentLight:OnShutDown()
    self:FreeSlot(LightSlot);
end

function EnvironmentLight:OnLoad(props)
    self:OnReset()
    self:ActivateLight(props.bActive)
end

function EnvironmentLight:OnSave(props)
    props.bActive = self.bActive
end

function EnvironmentLight:OnLevelLoaded()
    if (self.Properties.Options.bDeferredClipBounds) then
        self:UpdateLightClipBounds(LightSlot);
    end
end

function EnvironmentLight:OnPropertyChange()
    self:OnReset();
    self:ActivateLight( self.bActive );
    if (self.Properties.Options.bAffectsThisAreaOnly == 1) then
        self:UpdateLightClipBounds(LightSlot);
    end
end

-- optimization for common animated trackview properties, to avoid fully recreating everything on every animated frame
function EnvironmentLight:OnPropertyAnimated( name )
    local changeTakenCareOf = false;

    if (name=="fDiffuseMultiplier" or name=="fSpecularMultiplier") then
        changeTakenCareOf = true;
        local Color = self.Properties.Color;
        local diffuse_mul = Color.fDiffuseMultiplier;
        local specular_multiplier = Color.fSpecularMultiplier;
        local diffuse_color = { x=Color.clrDiffuse.x*diffuse_mul, y=Color.clrDiffuse.y*diffuse_mul, z=Color.clrDiffuse.z*diffuse_mul };
        self:SetLightColorParams( LightSlot, diffuse_color, specular_multiplier);
    end

    return changeTakenCareOf;
end

function EnvironmentLight:OnUpdate(dt)
    if (self.bActive == 1 and self.Properties.Options.bAffectsThisAreaOnly == 1) then
        self:UpdateLightClipBounds(LightSlot);
    end

    if (not System.IsEditor()) then
        self:Activate(0);
    end
end

function EnvironmentLight:OnReset()
    if (self.bActive ~= self.Properties.bActive) then
        self:ActivateLight( self.Properties.bActive );
    end
end

function EnvironmentLight:ActivateLight( enable )
    if (enable and enable ~= 0) then
        self.bActive = 1;
        self:LoadLightToSlot(LightSlot);
        self:ActivateOutput( "Active",true );
    else
        self.bActive = 0;
        self:FreeSlot(LightSlot);
        self:ActivateOutput( "Active",false );
    end
end

function EnvironmentLight:LoadLightToSlot( nSlot )
    local props = self.Properties;
    local Color = props.Color;
    local Options = props.Options;
    local OptionsAdvanced = props.OptionsAdvanced;
    local Projection = props.Projection;

    local diffuse_mul = Color.fDiffuseMultiplier;
    local specular_mul = Color.fSpecularMultiplier;

    local lt = self._LightTable;
    lt.radius = 0.5 * (props.BoxSizeX*props.BoxSizeX + props.BoxSizeY*props.BoxSizeY + props.BoxSizeZ*props.BoxSizeZ) ^ 0.5;
    lt.box_size_x = props.BoxSizeX;
    lt.box_size_y = props.BoxSizeY;
    lt.box_size_z = props.BoxSizeZ;
    lt.diffuse_color = { x=Color.clrDiffuse.x*diffuse_mul, y=Color.clrDiffuse.y*diffuse_mul, z=Color.clrDiffuse.z*diffuse_mul };
    lt.specular_multiplier = specular_mul;

    if ( OptionsAdvanced.texture_deferred_cubemap == "" ) then
        OptionsAdvanced.texture_deferred_cubemap = Options._texture_deferred_cubemap;
    end

    lt.deferred_cubemap = OptionsAdvanced.texture_deferred_cubemap;
    lt.this_area_only = Options.bAffectsThisAreaOnly;
    lt.ignore_visareas = Options.bIgnoresVisAreas;
    lt.volumetric_fog = Options.bVolumetricFog;
    lt.volumetric_fog_only = Options.bAffectsVolumetricFogOnly;

    lt.box_projection = Projection.bBoxProject; -- settings for box projection
    lt.box_width = Projection.fBoxWidth;
    lt.box_height = Projection.fBoxHeight;
    lt.box_length = Projection.fBoxLength;

    lt.sort_priority = Options.SortPriority;
    lt.attenuation_falloff_max = Options.fAttenuationFalloffMax;

    lt.lightmap_linear_attenuation = 1;
    lt.is_rectangle_light = 0;
    lt.is_sphere_light = 0;
    lt.area_sample_number = 1;

    lt.RAE_AmbientColor = { x = 0, y = 0, z = 0 };
    lt.RAE_MaxShadow = 1;
    lt.RAE_DistMul = 1;
    lt.RAE_DivShadow = 1;
    lt.RAE_ShadowHeight = 1;
    lt.RAE_FallOff = 2;
    lt.RAE_VisareaNumber = 0;

    self:LoadLight( nSlot,lt );
end

function EnvironmentLight:Event_Enable()
    if (self.bActive == 0) then
        self:ActivateLight( 1 );
    end
end

function EnvironmentLight:Event_Disable()
    if (self.bActive == 1) then
        self:ActivateLight( 0 );
    end
end

function Light:NotifySwitchOnOffFromParent(wantOn)
  local wantOff = wantOn~=true;
    if (self.bActive == 1 and wantOff) then
        self:ActivateLight( 0 );
    elseif (self.bActive == 0 and wantOn) then
        self:ActivateLight( 1 );
    end
end

------------------------------------------------------------------------------------------------------
-- Event Handlers
------------------------------------------------------------------------------------------------------
function EnvironmentLight:Event_Active( bActive )
    if (self.bActive == 0 and bActive == true) then
        self:ActivateLight( 1 );
    else
        if (self.bActive == 1 and bActive == false) then
            self:ActivateLight( 0 );
        end
    end
end

------------------------------------------------------------------------------------------------------
-- Event descriptions.
------------------------------------------------------------------------------------------------------
EnvironmentLight.FlowEvents =
{
    Inputs =
    {
        Active = { EnvironmentLight.Event_Active,"bool" },
        Enable = { EnvironmentLight.Event_Enable,"bool" },
        Disable = { EnvironmentLight.Event_Disable,"bool" },
    },
    Outputs =
    {
        Active = "bool",
    },
}
