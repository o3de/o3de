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
GravityValve =
{
    Properties =
    {
        bActive = 1,
        Strength = 10,
        Radius = 10,
        bSpline = 0,
    },

    Server = {},
    Client = {},

    _PhysTableCyl = { Area={}, },
    _PhysTableSph = { Area={}, },
    _PhysTableSpl = { Area={points={},}, },
    _Points = {},
    _Caps = {},

    Editor =
    {
        Icon = "GravitySphere.bmp",
        ShowBounds = 1,
    },
}

-------------------------------------------------------
function GravityValve:OnLoad(table)
    self.bActive = table.bActive
    self._Points = table._Points
    self._Caps = table._Caps
end

-------------------------------------------------------
function GravityValve:OnSave(table)
    table.bActive = self.bActive
    table._Points = self._Points
    table._Caps = self._Caps
end

function GravityValve:OnSpawn()
    self:SetActive( self.Properties.bActive )
end

function GravityValve:OnDestroy()
    local Caps = self._Caps
    while table.getn(Caps)>0 do
        System.RemoveEntity( Caps[1] )
        table.remove( Caps, 1 )
    end
end

-- OnPropertyChange called only by the editor
function GravityValve:OnPropertyChange()
    self:NeedSetup()
end

function GravityValve:OnReset()
    self:NeedSetup()
end

function GravityValve:FillPoints()
    local MAX_POINTS = 50
    local prefix = self:GetName().."_p"
    local name
    local nPoints = 1
    local Points = self._Points

    Points[1] = self

    for i=1,MAX_POINTS do
        name = prefix..tostring(i)
        local ent = System.GetEntityByName( name )
        if not ent then break end
        Points[i+1] = ent
        nPoints = i+1
        Log("npoints "..nPoints);
    end

    while table.getn(Points)>nPoints do
        Log("removed");
        table.remove(Points)
    end

    for i=1, table.getn(Points) do
        Log(".. "..Points[i]:GetName())
    end
end

function GravityValve:CreateCylinderArea( from, to, radius, strength )
    local Area = self._PhysTableCyl.Area
    local axis = DifferenceVectors( to, from )
    Area.type = AREA_CYLINDER
    Area.radius = radius
    Area.height = LengthVector( axis ) * .5
    Area.center = ScaleVector( SumVectors( to, from ), 0.5 )
    Area.axis = NormalizeVector( axis )
    Area.gravity = ScaleVector( Area.axis, strength )
    Area.uniform = 1
    Area.falloff = 0

    return Area
end

function GravityValve:CreateSphereArea( radius, gravity )
    local Area = self._PhysTableSph.Area
    Area.type = AREA_SPHERE
    Area.radius = radius
    Area.uniform = 1
    Area.falloff = 0
    Area.gravity = ScaleVector( gravity, 8 )

    return Area
end

function GravityValve:NeedSetup()
    self.bNeedSetup = 1
    self:Activate(1)
end

function GravityValve:OnChange( index )
    self:NeedSetup()
end

function GravityValve:OnUpdate()
    --Log("GV:OU "..self:GetName())
    if self.bNeedSetup then
        local wasActive = self.bActive
        self:SetActive(0)
        self:SetActive(wasActive)
        self.bNeedSetup = nil
        self:Activate(0)
    end
end

GravityValve.Server.OnUpdate = GravityValve.OnUpdate
GravityValve.Client.OnUpdate = GravityValve.OnUpdate

function GravityValve:SetActive( bActive )
    if bActive == self.bActive then return end
    self:FillPoints()
    local p, n
    local Points = self._Points
    local Caps = self._Caps
    n = table.getn(Points)
    --Log("GravityValve:SetActive "..tostring(bActive).." on valve "..self:GetName())
    if bActive == 1 then
        if (self.Properties.bSpline == 1) then

            local Area = self._PhysTableSpl.Area;
            local splinePoints = self._PhysTableSpl.Area.points;

            for p = 1, n do
                splinePoints[p] = {x=0,y=0,z=0};
                Points[p]:GetWorldPos(splinePoints[p]);
                --Log("Gravity Stream point "..p..":"..Vec2Str(splinePoints[p]));
            end

            Area.type = AREA_SPLINE;
            Area.radius = self.Properties.Radius;
            --Area.uniform = 1;
            Area.falloff = 0;
            Area.gravity = {x=0,y=0,z=self.Properties.Strength};
            Area.damping = 2.0;

            self:Physicalize(0, PE_AREA, self._PhysTableSpl);

            self:SetPhysicParams(PHYSICPARAM_FOREIGNDATA,{foreignData = ZEROG_AREA_ID});
        else
            local params = {}
            params.class = "GravityStreamCap"
            params.orientation = {x=1,y=0,z=0}

            for p = 2, n do
                self:CreateCylinderArea( Points[p-1]:GetPos(), Points[p]:GetPos(), self.Properties.Radius, self.Properties.Strength )
                Points[p]:Physicalize(0, PE_AREA, self._PhysTableCyl)

                Points[p]:SetPhysicParams(PHYSICPARAM_FOREIGNDATA,{foreignData = ZEROG_AREA_ID});

                if p > 2 then
                    params.position = Points[p-1]:GetPos()
                    local ent = System.SpawnEntity( params )
                    table.insert( Caps, ent.id )
                    self:CreateSphereArea( self.Properties.Radius, self._PhysTableCyl.Area.gravity )
                    ent:Physicalize(0, PE_AREA, self._PhysTableSph)

                    ent:SetPhysicParams(PHYSICPARAM_FOREIGNDATA,{foreignData = ZEROG_AREA_ID});
                end
            end
        end
    elseif bActive == 0 and self.bActive then

        self:DestroyPhysics();

        for p = 2, n do
            Points[p]:DestroyPhysics()
        end

        self:OnDestroy()
    end
    self.bActive = bActive
end

function GravityValve:Event_Enable()
    --Log("GravityValve: GetEnable")
    self:SetActive(1)
end

function GravityValve:Event_Disable()
    --Log("GravityValve: GetDisable")
    self:SetActive(0)
end

GravityValve.FlowEvents =
{
    Inputs =
    {
        Disable = { GravityValve.Event_Disable, "bool" },
        Enable = { GravityValve.Event_Enable, "bool" },
    },
    Outputs =
    {
        Disable = "bool",
        Enable = "bool",
    },
}
