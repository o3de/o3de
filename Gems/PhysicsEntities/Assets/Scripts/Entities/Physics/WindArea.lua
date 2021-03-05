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
-- Wind area
WindArea = 
{
    Properties = 
    {
        bActive = 1, --[0,1,1,"Defines whether wind is blowing or not."]
        Size = { x=10,y=10,z=10 },
        bEllipsoidal = 1, --[0,1,1,"Forces an ellipsoidal falloff."]
        FalloffInner = 0, --[0,100,0.1,"Distance after which the distance-based falloff begins."]
        Dir = { x=0,y=0,z=0 },
        Speed = 20, --[0,100,0.1,"Defines whether wind is blowing or not."]
        AirResistance = 0, --[0,100,0.1,"Causes very light physicalised objects to experience a buoyancy force, if >0."]
        AirDensity = 0, --[0,100,0.1,"Causes physicalised objects moving through the air to slow down, if >0."]
    },
    Editor = 
    {
        Icon = "Tornado.bmp",
    },
    _PhysTable = { Area={}, },
}

-------------------------------------------------------
function WindArea:OnLoad(table)
  self.bActive = table.bActive  
end

-------------------------------------------------------
function WindArea:OnSave(table)
  table.bActive = self.bActive
end

------------------------------------------------------------------------------------------------------
function WindArea:OnInit()
    self.bActive = self.Properties.bActive;
    if (self.bActive == 1) then
        self:PhysicalizeThis();
    end
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function WindArea:OnPropertyChange()
    self.bActive = self.Properties.bActive;
    self:PhysicalizeThis();
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function WindArea:OnReset()
end

------------------------------------------------------------------------------------------------------
function WindArea:PhysicalizeThis()
    if (self.bActive == 1) then
        local Properties = self.Properties;
        local Area = self._PhysTable.Area;
        if (Properties.bEllipsoidal == 1) then
            Area.type = AREA_SPHERE;
            Area.radius = LengthVector(Properties.Size) * 0.577;
        else
            Area.type = AREA_BOX;
        end
        Area.box_max = Properties.Size;
        Area.box_min = { x = -Area.box_max.x, y = -Area.box_max.y, z = -Area.box_max.z };
        if (Properties.bEllipsoidal == 1 or Properties.FalloffInner < 1) then
            Area.falloffInner = Properties.FalloffInner;
        else
            Area.falloffInner = -1;
        end
        if (Properties.Dir.x == 0 and Properties.Dir.y == 0 and Properties.Dir.z == 0) then
            Area.uniform = 0;
            Area.wind = {x = 0, y = 0, z = Properties.Speed};
        else
            Area.uniform = 2;
            Area.wind = { x = Properties.Dir.x * Properties.Speed, y = Properties.Dir.y * Properties.Speed, z = Properties.Dir.z * Properties.Speed };
        end
        Area.resistance = Properties.AirResistance;
        Area.density = Properties.AirDensity;
        self:Physicalize( 0,PE_AREA,self._PhysTable );
    else
        self:DestroyPhysics();
    end
end


------------------------------------------------------------------------------------------------------
-- Event Handlers
------------------------------------------------------------------------------------------------------
function WindArea:Event_Activate()
    if (self.bActive ~= 1) then
        self.bActive = 1;
        self:PhysicalizeThis();
    end
end

function WindArea:Event_Deactivate()
    if (self.bActive ~= 0) then
        self.bActive = 0;
        self:PhysicalizeThis();
    end
end

function WindArea:Event_Speed( sender, Speed )
    self.Properties.Speed = Speed;
    self:OnPropertyChange();
end

WindArea.FlowEvents =
{
    Inputs =
    {
        Deactivate = { WindArea.Event_Deactivate, "bool" },
        Activate = { WindArea.Event_Activate, "bool" },
        Speed = { WindArea.Event_Speed, "float" },
    },
    Outputs =
    {
        Deactivate = "bool",
        Activate = "bool",
    },
}
