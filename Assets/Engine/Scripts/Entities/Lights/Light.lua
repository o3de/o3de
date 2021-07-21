----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Light =
{
    Properties =
    {
        _nVersion = -1,
        bActive = 1, --[0,1,1,"Turns the light on/off."]
        Radius = 10, --[0,100,1,"Specifies how far from the source the light affects the surrounding area."]
        fAttenuationBulbSize = 0.05, --[0,100,0.1,"Specifies the radius of the area light bulb."]
        Style =
        {
            nLightStyle = 0, --[0,50,1,"Specifies a preset animation for the light to play."]
            fAnimationSpeed = 1, --[0,100,0.1,"Specifies the speed at which the light animation should play."]
            nAnimationPhase = 0, --[0,100,1,"This will start the light style at a different point along the sequence."]
            bAttachToSun = 0, --[0,1,1,"When enabled, sets the Sun to use the Flare properties for this light."]
            lightanimation_LightAnimation = "",
            bTimeScrubbingInTrackView = 0,
            _fTimeScrubbed = 0,
            bFlareEnable = 1, --[0,1,1,"Toggles the flare effect on or off for this light."]
            flare_Flare = "",
            fFlareFOV = 360, --[0,360,1,"FOV for the flare."]
        },
        Projector =
        {
            texture_Texture = "",
            fProjectorFov = 90, --[0,180,1,"Specifies the Angle on which the light texture is projected."]
            fProjectorNearPlane = 0, --[-100,100,0.1,"Set the near plane for the projector, any surfaces closer to the light source than this value will not be projected on."]
        },
        Color =
        {
            clrDiffuse = { x=1,y=1,z=1 },
            fDiffuseMultiplier = 1, --[0,999,0.1,"Control the strength of the diffuse color."]
            fSpecularMultiplier = 1, --[0,999,0.1,"Control the strength of the specular brightness."]
        },
        Options =
        {
            bAffectsThisAreaOnly = 1, --[0,1,1,"Set this parameter to false to make light cast in multiple visareas."]
            bIgnoresVisAreas = 0, --[0,1,1,"Controls whether the light should respond to visareas."]
            bAmbient = 0, --[0,1,1,"Makes the light behave like an ambient light source, with no point of origin."]
            bFakeLight = 0, --[0,1,1,"Disables light projection, useful for lights which you only want to have Flare effects from."]
            bVolumetricFog = 1, --[0,1,1,"Enables the light to affect volumetric fog."]
            bAffectsVolumetricFogOnly = 0, --[0,1,0,"Enables the light to affect only volumetric fog."]
            fFogRadialLobe = 0, --[0,1,0,"Set the blend ratio of main and side radial lobe for volumetric fog."]
        },
        Shadows = 
        {
            nCastShadows = 0,
            fShadowBias = 1, --[0,1000,1,"Moves the shadow cascade toward or away from the shadow-casting object."]
            fShadowSlopeBias = 1, --[0,1000,1,"Allows you to adjust the gradient (slope-based) bias used to compute the shadow bias."]
            fShadowResolutionScale = 1,
            nShadowMinResPercent = 0, --[0,100,1,"Percentage of the shadow pool the light should use for its shadows."]
            fShadowUpdateMinRadius = 10, --[0,100,0.1,"Define the minimum radius from the light source to the player camera that the ShadowUpdateRatio setting will be ignored."]
            fShadowUpdateRatio = 1, --[0,10,0.01,"Define the update ratio for shadow maps cast from this light."]
        },
        Shape =
        {
            bAreaLight = 0, --[0,1,1,"Used to turn the selected light entity into a Rectangular Area Light."]
            fPlaneWidth = 1, --[0,100,0.1,"Set the width of the Area Light shape."]
            fPlaneHeight = 1, --[0,100,0.1,"Set the height of the Area Light shape."]
        },
    },

    Editor =
    {
        Model="Editor/Objects/Light_Omni.cgf",
        Icon="Light.bmp",
        ShowBounds=0,
        AbsoluteRadius = 1,
        IsScalable = false;
    },

    _LightTable = {},
}

LightSlot = 1

function Light:OnInit()
    --self:NetPresent(0);
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
    self:OnReset();
    self:CacheResources("Light.lua");
end

function Light:CacheResources(requesterName)
    local textureFlags = 0;
end

function Light:OnShutDown()
    self:FreeSlot(LightSlot);
end

function Light:OnLoad(props)
    self:OnReset()
    self:ActivateLight(props.bActive)
end

function Light:OnSave(props)
    props.bActive = self.bActive
end

function Light:OnPropertyChange()
    self:OnReset();
    self:ActivateLight( self.bActive );
    if (self.Properties.Options.bAffectsThisAreaOnly == 1) then
        self:UpdateLightClipBounds(LightSlot);
    end
end

-- Optimization for common animated trackview properties, to avoid fully recreating everything on every animated frame
function Light:OnPropertyAnimated( name )
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

function Light:OnSysSpecLightChanged()
    self:OnPropertyChange();
end

function Light:OnLevelLoaded()
    if (self.Properties.Options.bAffectsThisAreaOnly == 1) then
        self:UpdateLightClipBounds(LightSlot);
    end
end

function Light:OnReset()
    if (self.bActive ~= self.Properties.bActive) then
        self:ActivateLight( self.Properties.bActive );
    end
end

function Light:ActivateLight( enable )
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

function Light:LoadLightToSlot( nSlot )
    local props = self.Properties;
    local Style = props.Style;
    local Projector = props.Projector;
    local Color = props.Color;
    local Options = props.Options;
    local Shape = props.Shape;
    local Shadows = props.Shadows;

    local diffuse_mul = Color.fDiffuseMultiplier;
    local specular_mul = Color.fSpecularMultiplier;

    local lt = self._LightTable;

    lt.radius = props.Radius;
    lt.attenuation_bulbsize = props.fAttenuationBulbSize;
    lt.diffuse_color = { x=Color.clrDiffuse.x*diffuse_mul, y=Color.clrDiffuse.y*diffuse_mul, z=Color.clrDiffuse.z*diffuse_mul };
    lt.specular_multiplier = specular_mul;

    lt.this_area_only = Options.bAffectsThisAreaOnly;
    lt.ambient = props.Options.bAmbient;
    lt.fake = Options.bFakeLight;
    lt.ignore_visareas = Options.bIgnoresVisAreas;
    lt.volumetric_fog = Options.bVolumetricFog;
    lt.volumetric_fog_only = Options.bAffectsVolumetricFogOnly;
    lt.fog_radial_lobe = Options.fFogRadialLobe;

    lt.cast_shadow = Shadows.nCastShadows;
    lt.shadow_bias = Shadows.fShadowBias;
    lt.shadow_slope_bias = Shadows.fShadowSlopeBias;
    lt.shadowResolutionScale = Shadows.fShadowResolutionScale;
    lt.shadowMinResolution = Shadows.nShadowMinResPercent;
    lt.shadowUpdate_MinRadius = Shadows.fShadowUpdateMinRadius;
    lt.shadowUpdate_ratio = Shadows.fShadowUpdateRatio;

    lt.projector_texture = Projector.texture_Texture;
    lt.proj_fov = Projector.fProjectorFov;
    lt.proj_nearplane = Projector.fProjectorNearPlane;

    lt.area_light = Shape.bAreaLight;
    lt.area_width = Shape.fPlaneWidth;
    lt.area_height = Shape.fPlaneHeight;

    lt.style = Style.nLightStyle;
    lt.attach_to_sun = Style.bAttachToSun;
    lt.anim_speed = Style.fAnimationSpeed;
    lt.anim_phase = Style.nAnimationPhase;
    lt.light_animation = Style.lightanimation_LightAnimation;
    lt.time_scrubbing_in_trackview = Style.bTimeScrubbingInTrackView;
    lt.time_scrubbed = Style._fTimeScrubbed;
    lt.flare_enable = Style.bFlareEnable;
    lt.flare_Flare = Style.flare_Flare;
    lt.flare_FOV = Style.fFlareFOV;

    lt.lightmap_linear_attenuation = 1;
    lt.is_rectangle_light = 0;
    lt.is_sphere_light = 0;
    lt.area_sample_number = 1;
    lt.indoor_only = 0;

    self:LoadLight( nSlot,lt );
end

function Light:Event_Enable()
    if (self.bActive == 0) then
        self:ActivateLight( 1 );
    end
end

function Light:Event_Disable()
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
function Light:Event_Active(sender, bActive)
    if (self.bActive == 0 and bActive == true) then
        self:ActivateLight( 1 );
    else
        if (self.bActive == 1 and bActive == false) then
            self:ActivateLight( 0 );
        end
    end
end

------------------------------------------------------------------------------------------------------
-- Event descriptions
------------------------------------------------------------------------------------------------------
Light.FlowEvents =
{
    Inputs =
    {
        Active = { Light.Event_Active,"bool" },
        Enable = { Light.Event_Enable,"bool" },
        Disable = { Light.Event_Disable,"bool" },
    },
    Outputs =
    {
        Active = "bool",
    },
}
