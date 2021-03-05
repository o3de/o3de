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
ParticlePhysics = 
{
    Properties = 
    {
        object_Model = "Objects/default/primitive_box.cgf",
        
        Particle =
        {
            mass = 50.0,
            radius = 1.0,
            air_resistance = 0.0,
            gravity = {x=0.0, y=0.0, z=-9.81},
            water_resistance = 0.5,
            water_gravity = {x=0.0, y=0.0, z=-9.81*0.8},
            min_bounce_vel = 1.5,
            accel_thrust = 0,
            accel_lift = 0,
            velocity = {x=0.0, y=0.0, z=0.0},
            thickness = 1.0,
            wspin = {x=0.0, y=0.0, z=0.0},
            normal = {x=0.0, y=0.0, z=1.0},
            pierceability = 15,
            constant_orientation = false,
            single_contact = true,
            no_roll = false,
            no_spin = true,
            no_path_alignment = false,
            collider_to_ignore = 0,
            material = "", -- this is supposed to be the surface type
        },
    },    
    
    Editor =
    {
        Icon = "physicsobject.bmp",
        IconOnTop = 1,
    },
}

function ParticlePhysics:SetFromProperties()
    local Properties = self.Properties;
    
    self:FreeSlot(0);
    
    if (Properties.object_Model ~= "") then
        self:LoadObject(0, Properties.object_Model);
    end
    
    NormalizeVector(Properties.Particle.normal);
    self:Physicalize(0, PE_PARTICLE, Properties);
    self:SetSlotPos(0, {0.0, 0.0, -Properties.Particle.thickness*0.5});
end

function ParticlePhysics:OnSpawn()
    self:OnReset();
end

function ParticlePhysics:OnPropertyChange()
    self:OnReset();
end

function ParticlePhysics:OnReset()
    self:SetFromProperties();
end
