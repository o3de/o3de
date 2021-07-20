----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript( "Scripts/Entities/Physics/BasicEntity.lua" );

SEQUENCE_NOT_STARTED = 0;
SEQUENCE_PLAYING = 1;
SEQUENCE_STOPPED = 2;

AnimObject =
{
    Properties =
    {
        Animation =
        {
            Animation = "Default",
            Speed = 1,
            bLoop = 1,
            bPlaying = 1,
            bAlwaysUpdate = 0,
            playerAnimationState = "",
            bPhysicalizeAfterAnimation = 0,
        },
        Physics =
        {
            bArticulated = 0,
            bRigidBody = 0,
            bPushableByPlayers = 0,
            bBulletCollisionEnabled = 1,
        },
        Rendering =
        {
            bWrinkleMap = 0,
        },
        Cinematic =
        {
            bOnDemandModelLoad = 0,
            bRenderAlways = 0,
        },
        ActivatePhysicsThreshold = 0,
        ActivatePhysicsDist = 50,
        bNoFriendlyFire = 0,
        object_Model = "",
        MultiplayerOptions =
        {
            bNetworked = 0,
        },
    },

    PHYSICALIZEAFTER_TIMER = 1,
    POSTQL_TIMER = 2,

    Client = {},
    Server = {},

    Editor =
    {
        Icon = "animobject.bmp",
        IconOnTop = 0,
    }
};

Net.Expose
{
    Class = AnimObject,
    ClientMethods =
    {
        ClEvent_StartAnimation = { RELIABLE_ORDERED, NO_ATTACH, FLOAT, },
        ClEvent_ResetAnimation = { RELIABLE_ORDERED, NO_ATTACH, },
        ClSync = { RELIABLE_ORDERED, NO_ATTACH, FLOAT, FLOAT, FLOAT, },
    },
    ServerMethods =
    {
        SVSync = { RELIABLE_ORDERED, NO_ATTACH, },
    },
    ServerProperties = {},
};


MakeDerivedEntityOverride( AnimObject,BasicEntity )

function AnimObject:LoadModelOnDemand()
    return self.Properties.Cinematic.bOnDemandModelLoad;
end

------------------------------------------------------------------------------------------------------
function AnimObject:SetFromProperties()
    self.controllingAnimHere = true;
    self.isModelLoaded = false;
    self.isRagdollized = false;
    self.__super.SetFromProperties(self); -- Call parent function.
    self.touchedByFlownode = false;

    self.animstarted = 0;
    self.sequenceStatus = SEQUENCE_NOT_STARTED;

    local Properties = self.Properties;

--    if (Properties.Animation.bPlaying ~= self.bAnimPlaying or Properties.Animation.bLoop ~= self.bAnimLoop or
--            Properties.Animation.Animation ~= self.animName or Properties.Animation.Speed ~= self.animationSpeed) then

        self.bAnimPlaying = Properties.Animation.bPlaying;
        self.bAnimLoop = Properties.Animation.bLoop;
        self.animName = Properties.Animation.Animation;
        if (Properties.Animation.bPlaying == 1) then
            self:DoStartAnimation();

        else
            self:ResetAnimation(0, -1);
        end
--    end

    if (Properties.Animation.bAlwaysUpdate == 1) then
        self:Activate(1);
    end
    self:SetAnimationSpeed( 0, 0, Properties.Animation.Speed )
    self.animationSpeed = Properties.Animation.Speed;
    self.curAnimTime = 0;
    if (self.Properties.ActivatePhysicsThreshold>0) then
        local apd = { threshold = self.Properties.ActivatePhysicsThreshold, detach_distance = self.Properties.ActivatePhysicsDist }
        self:SetPhysicParams(PHYSICPARAM_AUTO_DETACHMENT, apd);
    end

    self:CheckShaderParamCallbacks();
end

------------------------------------------------------------------------------------------------------
function AnimObject:SetupModel()
    if (self:LoadModelOnDemand()==0 or System.IsEditor()) then
        self:LoadAndPhysicalizeModel();
    else
        Game.CacheResource("AnimObject.lua", self.Properties.object_Model, eGameCacheResourceType_StaticObject, 0);
    end
end

-------------------------------------------------------------------------------------------------------
function AnimObject:LoadAndPhysicalizeModel()
    if (not self.isModelLoaded) then
        self:LoadObject(0,self.Properties.object_Model);
        self:RenderAlways(self.Properties.Cinematic.bRenderAlways);

        if (self.Properties.Physics.bPhysicalize == 1) then
            self:PhysicalizeThis();
        end
        self.isModelLoaded = true;
        return 1;
    end
    return 0;
end

-------------------------------------------------------------------------------------------------------
function AnimObject:UnloadModel()
    if (self.isModelLoaded) then
        self:DestroyPhysics();
        self:FreeSlot(0);
        self.isModelLoaded = false;
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:OnSpawn()
    if (self.Properties.MultiplayerOptions.bNetworked == 0) then
        self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
    end

    self.isRagdollized = false;
    self.__super.OnSpawn(self); -- Call parent

    if (self.Properties.Animation.bAlwaysUpdate == 1) then
        CryAction.CreateGameObjectForEntity(self.id);
        CryAction.BindGameObjectToNetwork(self.id);
        CryAction.ForceGameObjectUpdate(self.id, true);
        self:Activate(1);
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:OnReset()
    self.__super.OnReset(self); -- Call parent
    self.bAnimPlaying = 0;
    self:SetFromProperties();
    self.sequenceStatus = SEQUENCE_NOT_STARTED;
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_ResetAnimation()
    self.controllingAnimHere = true;
    self:ResetAnimation(0, -1);
    self.animstarted=0;
    --
    local PhysProps = self.Properties.Physics;
    if( PhysProps.Mass>0 ) then
        self:OnReset();
    else
        self.animName = self.Properties.Animation.Animation;
        self:StartAnimation( 0,self.Properties.Animation.Animation,0,0,0,false );
    end;
    -- net
    if( CryAction.IsServer() and self.allClients ) then
        self.allClients:ClEvent_ResetAnimation();
    end

end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_StartAnimation(sender)
    self.controllingAnimHere = true;
    self:StartEntityAnimation();
    self.animstarted=1;

    if( CryAction.IsServer() and self.allClients) then
        self.allClients:ClEvent_StartAnimation(CryAction.GetServerTime());
        --Log("Server:ClEvent_StartAnimation call"..self:GetName());
    end

end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_StopAnimation(sender)
    self.controllingAnimHere = true;
    if (self.animstarted == 1 and self:IsAnimationRunning(0,0)) then
        self.curAnimTime = self:GetAnimationTime(0,0);
    else
        self.curAnimTime = 0;
    end
    self:StopAnimation(0, -1);    -- all layers
    self.animstarted = 0;
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_RagdollizeDerived()
    self.isRagdollized = true;
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_ModelLoad()
    local justLoaded = self:LoadAndPhysicalizeModel();
    if(justLoaded ~= 0) then
        self:RelinkAllEntities();
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_ModelUnload()
    if (not System.IsEditor()) then
        self:UnloadModel();
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_RenderAlwaysEnable()
    self:RenderAlways(1);
end

------------------------------------------------------------------------------------------------------
function AnimObject:Event_RenderAlwaysDisable()
    self:RenderAlways(0);
end

------------------------------------------------------------------------------------------------------
function AnimObject:DoStartAnimation()
    self.animName = self.Properties.Animation.Animation;
    self:StartAnimation( 0,self.Properties.Animation.Animation,0,0,self.Properties.Animation.Speed,self.Properties.Animation.bLoop,1 );
    self:ForceCharacterUpdate(0, true);
    self.animstarted = 1;
    -- save curAnimTime for QS/QL
    if (self.Properties.Animation.Speed < 0) then
        self.curAnimTime = 0;
    else
        self.curAnimTime = self:GetAnimationLength(0, self.Properties.Animation.Animation);
    end

--    local currTime = System.GetCurrTime();
    self.startTime = CryAction.GetServerTime();--System.GetCurrAsyncTime();
    if( self.timeDiff ) then
        self.startTime=self.startTime-self.timeDiff;
    end

end

------------------------------------------------------------------------------------------------------
function AnimObject:StartEntityAnimation()
    self:StopAnimation(0, -1);
    self:DoStartAnimation();
    self.bStopAnimAfterQL = false;
    self:KillTimer(self.POSTQL_TIMER);

    local playerAnimationState = self.Properties.Animation.playerAnimationState;
    if (g_localActor and playerAnimationState ~= "") then
        g_localActor.actor:CreateCodeEvent(
            {
                event = "animationControl",pos=self:GetWorldPos(),angle=self:GetWorldAngles()
            }
        ); --,entId=self.id})
        g_localActor.actor:QueueAnimationState(playerAnimationState);
        if (self.Properties.Animation.bPhysicalizeAfterAnimation == 1) then
            local animLen = self:GetAnimationLength(0,self.Properties.Animation.Animation) * 1000.0 / self.Properties.Animation.Speed;
            self:SetTimer(self.PHYSICALIZEAFTER_TIMER,animLen);
            --Log("timer set to:"..animLen.."ms");
        end
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject.Client:OnTimer(timerId,mSec)
    if (timerId == self.PHYSICALIZEAFTER_TIMER) then
        local PhysProps = self.Properties.Physics;

        PhysProps.bRigidBodyActive = 1;
        PhysProps.bPhysicalize=1;
        PhysProps.bRigidBody=1;
        PhysProps.bResting = 0;

        if (self.bRigidBodyActive ~= PhysProps.bRigidBodyActive) then
            self.bRigidBodyActive = PhysProps.bRigidBodyActive;
            self:PhysicalizeThis();
        end
        if (PhysProps.bRigidBody == 1) then
            self:AwakePhysics(1-PhysProps.bResting);
            self.bRigidBodyActive = PhysProps.bRigidBodyActive;
        end
    end
    if (timerId == self.POSTQL_TIMER and self.sequenceStatus == SEQUENCE_NOT_STARTED) then
        self:StopAnimation(0, -1);
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:CorrectTiming(frameTime)

--    local skip = 0;
--    if( skip==0 ) then
    if( self.animstarted==1 and self:IsAnimationRunning(0,0) and self.Properties.Animation.Speed>0 ) then
        local curTime = CryAction.GetServerTime();--System.GetCurrAsyncTime();
        local diffRealTime = (curTime-self.startTime)*self.Properties.Animation.Speed;
        local curAnimTime = self:GetAnimationTime(0,0);
        if( curAnimTime<=self.curAnimTime ) then
            local diff = diffRealTime-curAnimTime;
            if( diff<-0.02 ) then
                    -- correct speed
                    local frameTimeAnim = self.Properties.Animation.Speed*frameTime;
                    local reqTime = frameTimeAnim-diff;
                    local ratio = (frameTimeAnim)/reqTime;
                    if( ratio<0.5 ) then
                        -- clamp
                        ratio=0.5;
                    end
                    --
                    newSpeed = ratio*self.Properties.Animation.Speed;
                    self:SetAnimationSpeed( 0, 0, newSpeed );

                    --System.LogToConsole(self:GetName().." RealLess="..diff.." Speed="..newSpeed);
            else
                if( diff>0.02 ) then
                    -- correct speed
                    local frameTimeAnim = self.Properties.Animation.Speed*frameTime;
                    local reqTime = frameTimeAnim+diff;
                    local ratio = reqTime/(frameTimeAnim);
                    if( ratio>1.1 ) then
                        -- clamp
                        ratio=1.1;
                    end

                    newSpeed = ratio*self.Properties.Animation.Speed;
                    self:SetAnimationSpeed( 0, 0, newSpeed );

                    --System.LogToConsole(self:GetName().." RealMore="..diff.." Speed="..newSpeed);
                else
                    -- restore speed
                    if( self.Properties.Animation.Speed>0 ) then
                        self:SetAnimationSpeed( 0, 0, self.Properties.Animation.Speed );
                    end
                end
            end
        end
    end
--    end
end

------------------------------------------------------------------------------------------------------
function AnimObject.Server:OnUpdate(dt)
    if( CryAction.IsServer() ) then
        self:CorrectTiming(dt);
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject.Client:OnUpdate(dt)

    if( CryAction.IsClient() and not CryAction.IsServer() ) then
        self:CorrectTiming(dt);
    end

    if (self.bStopAnimAfterQL) then
        self.bStopAnimAfterQL = false;
        self:SetTimer(self.POSTQL_TIMER, 0.2);
        if (self.Properties.Animation.bAlwaysUpdate ~= 1) then
            self:Activate(0);
        end
    end

end

-------------------------------------------------------
function AnimObject:OnLoad(table)
    local wasRagollized = table.isRagdollized;
    if (self.isRagdollized and (not wasRagdollized)) then   -- for now we dont care about the oposite situation: the object was ragdollized before the save, and is not ragdollized now.
        self:OnReset();
    end

    --self.__super.OnLoad( self,table ); -- Call parent
    self.bAnimPlaying = table.bAnimPlaying;
    self.bAnimLoop = table.bAnimLoop;
    self.animName = table.animName;
    self.animstarted = table.animstarted;
    self.health = table.health;
    self.dead = table.dead;
    self.controllingAnimHere = table.bControllingAnimHere;
    -- self.movedByFlowgraph = table.movedByFlowgraph;

    if (self.controllingAnimHere) then
        if (self.animstarted == 1) then -- restart animation
            self:StartEntityAnimation();
            self:SetAnimationTime(0, 0, table.animTime);
        else
            --we have to stop the animation
            -- either at the point stored in the file
            if (table.animTime > 0) then
                if (self.animName~=self.Properties.Animation.Animation) then
                    self:StartAnimation( 0, self.animName, 0, 0, self.Properties.Animation.Speed,self.Properties.Animation.bLoop,1 );
                    self:SetAnimationTime(0, 0, table.animTime);
                else
                    self:StartEntityAnimation();
                end
                self:SetAnimationSpeed( 0, 0, 0.0 );
                self:SetAnimationTime(0, 0, table.animTime);
                self.bStopAnimAfterQL = true;
                self:Activate(1);
                self.curAnimTime = table.animTime;
            end

            if (table.animTime==0) then
                local bTouchedByTrackview = (table.sequenceStatus == SEQUENCE_NOT_STARTED and self.sequenceStatus ~= SEQUENCE_NOT_STARTED);
                -- this check makes no sense imo. But im not removing it at this time (c3 last weeks) to avoid any risk
                if (bTouchedByTrackview or self.touchedByFlownode) then
                    -- or just at the beginning
                    self:ResetAnimation(0, -1);
                    self:StartEntityAnimation();
                    self:SetAnimationSpeed(0, 0, 0.0);
                    self:SetAnimationTime(0, 0, 0.0);
                    self.bStopAnimAfterQL = true;
                    self:Activate(1);
                    self.curAnimTime = 0;
                end
            end
        end
    else
        self.externalAnim_anim = table.externalAnim_anim;
        self.externalAnim_layer = table.externalAnim_layer;
        self.externalAnim_loop = table.externalAnim_loop;
        self:StartAnimation( 0, self.externalAnim_anim, self.externalAnim_layer, 0, 1, self.externalAnim_loop );
        self:SetAnimationTime(0, self.externalAnim_layer, table.animTime);
    end
    self.touchedByFlownode = false;

    -- physicalized ones that are neither articulated neither rigidbody become static. Static physical entities are not serialized at all. So we just rephysicallize in that case.
    if (self.Properties.Physics.bArticulated==0 and self.Properties.Physics.bRigidBody==0 and self.Properties.Physics.bPhysicalize==1) then
        self:PhysicalizeThis();
    end

    self.sequenceStatus = table.sequenceStatus;
end

-------------------------------------------------------
function AnimObject:OnSave(table)
    table.isRagdollized = self.isRagdollized;
    table.bAnimPlaying = self.bAnimPlaying
    table.bAnimLoop = self.bAnimLoop
    table.animName = self.animName
    table.sequenceStatus = self.sequenceStatus;
    table.health = self.health;
    table.dead = self.dead;
    table.bControllingAnimHere = self.controllingAnimHere;

    if (self.controllingAnimHere) then
        if (self.animstarted == 1 and self:IsAnimationRunning(0,0)) then
            table.animTime = self:GetAnimationTime(0,0);
            table.animstarted = 1;
        else
            table.animstarted = 0;
            if (self.curAnimTime) then
              table.animTime = self.curAnimTime;
            else
                table.animTime = 0;
            end
        end
    else
        table.externalAnim_anim = self.externalAnim_anim;
        table.externalAnim_layer = self.externalAnim_layer;
        table.externalAnim_loop = self.externalAnim_loop;
        table.animTime = self:GetAnimationTime(0,self.externalAnim_layer);
    end
end

------------------------------------------------------------------------------------------------------
-- Additional Flow events.
------------------------------------------------------------------------------------------------------
AnimObject.FlowEvents.Inputs.ResetAnimation = { AnimObject.Event_ResetAnimation, "bool" }
AnimObject.FlowEvents.Inputs.StartAnimation = { AnimObject.Event_StartAnimation, "bool" }
AnimObject.FlowEvents.Inputs.StopAnimation = { AnimObject.Event_StopAnimation, "bool" }

AnimObject.FlowEvents.Inputs.ModelLoad = { AnimObject.Event_ModelLoad, "bool" }
AnimObject.FlowEvents.Inputs.ModelUnload = { AnimObject.Event_ModelUnload, "bool" }


-- client functions
function AnimObject.Client:ClEvent_StartAnimation(servertime)

    --Log("ClEvent_StartAnimation recieved"..self:GetName());

    self.timeDiff = CryAction.GetServerTime()-servertime;
--    local localDiff = System.GetCurrTime()-servertime;
--    if( self.timeDiff>0.1 ) then
--            System.LogToConsole(self:GetName().." Diff="..self.timeDiff.." localDiff="..localDiff);
--    else
--        if( self.timeDiff<-0.1 ) then
--                System.LogToConsole(self:GetName().." Diff="..self.timeDiff);
--        end
--    end

    if( not CryAction.IsServer() ) then
        self:Event_StartAnimation();
    end
end

function AnimObject.Client:ClEvent_ResetAnimation()
    if( not CryAction.IsServer() ) then
        self:Event_ResetAnimation();
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:SavePhysicalState()
    self.initPos = self:GetPos();
    self.initRot = self:GetWorldAngles();
    self.initScale = self:GetScale();
end

function AnimObject:RestorePhysicalState()
    self:SetPos(self.initPos);
    self:SetWorldAngles(self.initRot);
    self:SetScale(self.initScale);

    -- restore
    self:ResetAnimation(0, -1);
    self.animstarted=0;
    local PhysProps = self.Properties.Physics;
    if( PhysProps.Mass>0 ) then
        self:OnReset();
    else
        self.animName = self.Properties.Animation.Animation;
        self:StartAnimation( 0,self.Properties.Animation.Animation,0,0,0,false );
    end;
end

------------------------------------------------------------------------------------------------------
function AnimObject:PhysicalizeThis()

    BasicEntity.PhysicalizeThis(self);

    -- Remove bullet collision if desired
    local Physics = self.Properties.Physics;
    if (Physics.bBulletCollisionEnabled == 0) then
        local flagstab = { flags_mask= geom_colltype_ray + geom_colltype_foliage_proxy, flags=geom_colltype_player*Physics.bPushableByPlayers };
        self:SetPhysicParams(PHYSICPARAM_PART_FLAGS, flagstab);
    end
end

------------------------------------------------------------------------------------------------------
function AnimObject:SendSyncToClient( channelId )
    if( self.animstarted==1 ) then
        animTime = self:GetAnimationTime(0,0);
        self.onClient:ClSync( channelId, animTime, self.startTime, CryAction.GetServerTime() )
    end
end


function AnimObject.Server:OnPostInitClient( channelId )
    self:SendSyncToClient(channelId);
end


function AnimObject.Server:SVSync(channelId)
    self:SendSyncToClient(channelId);
end


function AnimObject.Client:ClSync( animTimeValue, startTimeValue, serverTimeValue )
--    if( self.animstarted==0 ) then
        --self:Event_ResetAnimation();
        --self.timeDiff = CryAction.GetServerTime()-serverTimeValue;
        self:StartEntityAnimation();
        self.startTime = startTimeValue;
        self:SetAnimationTime(0,0,animTimeValue);
        --Log("CLSync recieved"..self:GetName()..animTimeValue);
--    end
end


function AnimObject:UpdateFromServer()
    self.server:SVSync();
end


-- notifications from PlaySequence FG node (entire sequence starts/stops)
function AnimObject:OnSequenceStart()
    self.sequenceStatus = SEQUENCE_PLAYING;
end


function AnimObject:OnSequenceStop()
    self.sequenceStatus = SEQUENCE_STOPPED;
end


-- Notifications from trackview (animation in sequence starts/stops)
function AnimObject:OnSequenceAnimationStart( animName )
    self.sequenceStatus = SEQUENCE_PLAYING;
    self.animName = animName;
end


function AnimObject:OnSequenceAnimationStop()
    self.sequenceStatus = SEQUENCE_STOPPED;
end


function AnimObject:OnFlowGraphAnimationStart( animName, layer, loop )
    self.animName = animName;
    self.externalAnim_anim = animName;
    self.controllingAnimHere = false;
    self.externalAnim_layer = layer;
    self.externalAnim_loop = loop;
    self.touchedByFlownode = true;
end


function AnimObject:OnFlowGraphAnimationStop()
    if (self.externalAnim_layer) then
        self.curAnimTime = self:GetAnimationTime(0, self.externalAnim_layer);
    end
    self.controllingAnimHere = true;
end


function AnimObject:OnFlowGraphAnimationEnd()
    self.curAnimTime = 1;  -- is a normalized time, so 1 == end
    self.controllingAnimHere = true;
end