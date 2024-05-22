/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "MemoryCategories.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include <MCore/Source/RefCounted.h>
#include "DebugDraw.h"
#include "EventManager.h"
#include "Importer/Importer.h"


namespace EMotionFX
{
    enum EventTypes
    {
        EVENT_TYPE_ON_PLAY_MOTION,
        EVENT_TYPE_ON_DELETE_MOTION,

        // Keep this block together since MotionInstance only cares about these. Also keep EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT
        // at the beginning and EVENT_TYPE_MOTION_INSTANCE_LAST_EVENT at the end (and updated)
        EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT,
        EVENT_TYPE_ON_EVENT = EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT,
        EVENT_TYPE_ON_START_MOTION_INSTANCE,
        EVENT_TYPE_ON_DELETE_MOTION_INSTANCE,
        EVENT_TYPE_ON_STOP,
        EVENT_TYPE_ON_HAS_LOOPED,
        EVENT_TYPE_ON_HAS_REACHED_MAX_NUM_LOOPS,
        EVENT_TYPE_ON_HAS_REACHED_MAX_PLAY_TIME,
        EVENT_TYPE_ON_IS_FROZEN_AT_LAST_FRAME,
        EVENT_TYPE_ON_CHANGED_PAUSE_STATE,
        EVENT_TYPE_ON_CHANGED_ACTIVE_STATE,
        EVENT_TYPE_ON_START_BLENDING,
        EVENT_TYPE_ON_STOP_BLENDING,
        EVENT_TYPE_ON_QUEUE_MOTION_INSTANCE,
        EVENT_TYPE_MOTION_INSTANCE_LAST_EVENT = EVENT_TYPE_ON_QUEUE_MOTION_INSTANCE,

        EVENT_TYPE_ON_DELETE_ACTOR,
        EVENT_TYPE_ON_SIMULATE_PHYSICS,
        EVENT_TYPE_ON_CUSTOM_EVENT,
        EVENT_TYPE_ON_DRAW_LINE,
        EVENT_TYPE_ON_DRAW_TRIANGLE,
        EVENT_TYPE_ON_DRAW_TRIANGLES,
        EVENT_TYPE_ON_CREATE_ANIM_GRAPH,
        EVENT_TYPE_ON_CREATE_ANIM_GRAPH_INSTANCE,
        EVENT_TYPE_ON_CREATE_MOTION,
        EVENT_TYPE_ON_CREATE_MOTION_SET,
        EVENT_TYPE_ON_CREATE_MOTION_INSTANCE,
        EVENT_TYPE_ON_CREATE_MOTION_SYSTEM,
        EVENT_TYPE_ON_CREATE_ACTOR,
        EVENT_TYPE_ON_POST_CREATE_ACTOR,
        EVENT_TYPE_ON_DELETE_ANIM_GRAPH,
        EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE,
        EVENT_TYPE_ON_DELETE_MOTION_SET,
        EVENT_TYPE_ON_DELETE_MOTION_SYSTEM,
        EVENT_TYPE_ON_RAY_INTERSECTION_TEST,

        // Keep this block together since AnimGraphInstance only cares about these. Also keep EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT
        // at the beginning and EVENT_TYPE_ANIM_GRAPH_INSTANCE_LAST_EVENT at the end (and updated)
        EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT,
        EVENT_TYPE_ON_STATE_ENTER = EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT,
        EVENT_TYPE_ON_STATE_ENTERING,
        EVENT_TYPE_ON_STATE_EXIT,
        EVENT_TYPE_ON_STATE_END,
        EVENT_TYPE_ON_START_TRANSITION,
        EVENT_TYPE_ON_END_TRANSITION,
        EVENT_TYPE_ANIM_GRAPH_INSTANCE_LAST_EVENT = EVENT_TYPE_ON_END_TRANSITION,

        EVENT_TYPE_ON_SET_VISUAL_MANIPULATOR_OFFSET,
        EVENT_TYPE_ON_INPUT_PORTS_CHANGED,
        EVENT_TYPE_ON_OUTPUT_PORTS_CHANGED,
        EVENT_TYPE_ON_RENAMED_NODE,
        EVENT_TYPE_ON_CREATED_NODE,
        EVENT_TYPE_ON_REMOVE_NODE,
        EVENT_TYPE_ON_REMOVED_CHILD_NODE,
        EVENT_TYPE_ON_PROGRESS_START,
        EVENT_TYPE_ON_PROGRESS_END,
        EVENT_TYPE_ON_PROGRESS_TEXT,
        EVENT_TYPE_ON_PROGRESS_VALUE,
        EVENT_TYPE_ON_SUB_PROGRESS_TEXT,
        EVENT_TYPE_ON_SUB_PROGRESS_VALUE,
        EVENT_TYPE_ON_SCALE_ACTOR_DATA,
        EVENT_TYPE_ON_SCALE_MOTION_DATA,
        EVENT_TYPE_ON_SCALE_ANIM_GRAPH_DATA,

        EVENT_TYPE_COUNT
    };

    /**
     * The event handler, which is responsible for processing the events.
     * This class contains several methods which you can overload to perform custom things event comes up.
     * You can inherit your own event handler from this base class and add it to the event manager using EMFX_EVENTMGR.AddEventHandler(...) method
     * to make it use your custom event handler.
     * Every event your derived class handles has to be returned by the GetHandledEventTypes method. This helps filtering
     * the event dispatching to only the handlers that are interested about such event.
     */
    class EMFX_API EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EventHandler() = default;
        virtual ~EventHandler() = default;

        /**
         * The main method that processes a event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        virtual void OnEvent(const EventInfo& eventInfo)
        {
            MCORE_UNUSED(eventInfo);
            // find the event index in the array
            //          const uint32 eventIndex  = GetEventManager().FindEventTypeIndex( eventInfo.m_eventTypeID );

            // get the name of the event
            //          const char* eventName = (eventIndex != MCORE_INVALIDINDEX32) ? GetEventManager().GetEventTypeString( eventIndex ) : "<unknown>";

            // log it
            //          MCore::LogDetailedInfo("EMotionFX: *** Motion event %s at time %f of type ID %d ('%s') with parameters '%s' for actor '%s' on motion '%s' has been triggered", isEventStart ? "START" : "END", timeValue, eventTypeID, eventName, parameters, actorInstance->GetActor()->GetName(), motionInstance->GetMotion()->GetName());

            /*
                // when you implement your own event handler, of course you can just do something like a switch statement.
                // then the FindEventTypeIndex and GetEventTypeString aren't needed at all
                switch (eventTypeID)
                {
                    case EVENT_SOUND:
                        //....
                        break;

                    case EVENT_SCRIPT:
                        //....
                        break;

                    //....
                };
            */
        }

        /**
         * Event handlers need to overload this function and return the list of events they are interested about
         */
        virtual const AZStd::vector<EventTypes> GetHandledEventTypes() const = 0;

        /**
         * The event that gets triggered when a MotionSystem::PlayMotion(...) is being executed.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motion The motion that is being played.
         * @param info The playback info used to play the motion.
         */
        virtual void OnPlayMotion(Motion* motion, PlayBackInfo* info)                                                                       { MCORE_UNUSED(motion); MCORE_UNUSED(info); }

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motionInstance The motion instance that is being played.
         * @param info The playback info used to play the motion.
         */
        virtual void OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)                                              { MCORE_UNUSED(motionInstance); MCORE_UNUSED(info); }

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         * @param motionInstance The motion instance that is being deleted.
         */
        virtual void OnDeleteMotionInstance(MotionInstance* motionInstance)                                                                 { MCORE_UNUSED(motionInstance); }

        /**
         * The event that gets triggered once a Motion object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Motion object.
         * You can get and set this data object with the Motion::GetCustomData() and Motion::SetCustomData(...) methods.
         * @param motion The motion that is being deleted.
         */
        virtual void OnDeleteMotion(Motion* motion)                                                                                         { MCORE_UNUSED(motion); }

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         * @param motionInstance The motion instance that is being stopped.
         */
        virtual void OnStop(MotionInstance* motionInstance)                                                                                 { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has looped.
         * @param motionInstance The motion instance that got looped.
         */
        virtual void OnHasLooped(MotionInstance* motionInstance)                                                                            { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         * @param motionInstance The motion instance that reached its maximum number of allowed loops.
         */
        virtual void OnHasReachedMaxNumLoops(MotionInstance* motionInstance)                                                                { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         * @param motionInstance The motion instance that reached its maximum playback time.
         */
        virtual void OnHasReachedMaxPlayTime(MotionInstance* motionInstance)                                                                { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         * @param motionInstance The motion instance that got into a frozen state.
         */
        virtual void OnIsFrozenAtLastFrame(MotionInstance* motionInstance)                                                                  { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         * @param motionInstance The motion instance that changed its paused state.
         */
        virtual void OnChangedPauseState(MotionInstance* motionInstance)                                                                    { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         * @param motionInstance The motion instance that changed its active state.
         */
        virtual void OnChangedActiveState(MotionInstance* motionInstance)                                                                   { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance which is changing its weight value over time.
         */
        virtual void OnStartBlending(MotionInstance* motionInstance)                                                                        { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance for which the automatic weight blending just stopped.
         */
        virtual void OnStopBlending(MotionInstance* motionInstance)                                                                         { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::m_playNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param motionInstance The motion instance that gets added to the motion queue.
         * @param info The playback information used to play this motion instance.
         */
        virtual void OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)                                              { MCORE_UNUSED(motionInstance); MCORE_UNUSED(info); }

        //---------------------------------------------------------------------

        /**
         * The event that gets triggered once an Actor object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Actor object.
         * You can get and set this data object with the Actor::GetCustomData() and Actor::SetCustomData(...) methods.
         * @param actor The actor that is being deleted.
         */
        virtual void OnDeleteActor(Actor* actor)                                                                                            { MCORE_UNUSED(actor); }

        virtual void OnSimulatePhysics(float timeDelta)                                                                                     { MCORE_UNUSED(timeDelta); }
        virtual void OnCustomEvent(uint32 eventType, void* data)                                                                            { MCORE_UNUSED(eventType); MCORE_UNUSED(data); }

        virtual void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color) { MCORE_UNUSED(posA); MCORE_UNUSED(posB); MCORE_UNUSED(posC); MCORE_UNUSED(normalA); MCORE_UNUSED(normalB); MCORE_UNUSED(normalC); MCORE_UNUSED(color); }
        virtual void OnDrawTriangles() {}

        // creation callbacks
        virtual void OnCreateAnimGraph(AnimGraph* animGraph)                                                                             { MCORE_UNUSED(animGraph); }
        virtual void OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance)                                                     { MCORE_UNUSED(animGraphInstance); }
        virtual void OnCreateMotion(Motion* motion)                                                                                         { MCORE_UNUSED(motion); }
        virtual void OnCreateMotionSet(MotionSet* motionSet)                                                                                { MCORE_UNUSED(motionSet); }
        virtual void OnCreateMotionInstance(MotionInstance* motionInstance)                                                                 { MCORE_UNUSED(motionInstance); }
        virtual void OnCreateMotionSystem(MotionSystem* motionSystem)                                                                       { MCORE_UNUSED(motionSystem); }
        virtual void OnCreateActor(Actor* actor)                                                                                            { MCORE_UNUSED(actor); }
        virtual void OnPostCreateActor(Actor* actor)                                                                                        { MCORE_UNUSED(actor); }

        // delete callbacks
        virtual void OnDeleteAnimGraph(AnimGraph* animGraph)                                                                             { MCORE_UNUSED(animGraph); }
        virtual void OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance)                                                     { MCORE_UNUSED(animGraphInstance); }
        virtual void OnDeleteMotionSet(MotionSet* motionSet)                                                                                { MCORE_UNUSED(motionSet); }
        virtual void OnDeleteMotionSystem(MotionSystem* motionSystem)                                                                       { MCORE_UNUSED(motionSystem); }

        /**
         * Perform a ray intersection test and return the intersection info.
         * The first event handler registered that sets the IntersectionInfo::m_isValid to true will be outputting to the outIntersectInfo parameter.
         * @param start The start point, in world space.
         * @param end The end point, in world space.
         * @param outIntersectInfo The resulting intersection info.
         * @result Returns true when an intersection occurred and false when no intersection occurred.
         */
        virtual bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo)      { MCORE_UNUSED(start); MCORE_UNUSED(end); MCORE_UNUSED(outIntersectInfo); return false; }

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                 { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                  { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, size_t paramIndex, const AZ::Vector3& offset)         { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(paramIndex); MCORE_UNUSED(offset); }

        virtual void OnInputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue)   { AZ_UNUSED(node); AZ_UNUSED(newInputPorts); AZ_UNUSED(memberName); AZ_UNUSED(memberValue); }
        virtual void OnOutputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue) { AZ_UNUSED(node); AZ_UNUSED(newOutputPorts); AZ_UNUSED(memberName); AZ_UNUSED(memberValue); }

        virtual void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName)                              { MCORE_UNUSED(animGraph); MCORE_UNUSED(node); MCORE_UNUSED(oldName); }
        virtual void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)                                                            { MCORE_UNUSED(animGraph); MCORE_UNUSED(node); }
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)                                                     { MCORE_UNUSED(animGraph); MCORE_UNUSED(nodeToRemove); }
        virtual void OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode)                                                 { MCORE_UNUSED(animGraph); MCORE_UNUSED(parentNode); }

        virtual void OnProgressStart()                                                                                                      {}
        virtual void OnProgressEnd()                                                                                                        {}
        virtual void OnProgressText(const char* text)                                                                                       { MCORE_UNUSED(text); }
        virtual void OnProgressValue(float percentage)                                                                                      { MCORE_UNUSED(percentage); }
        virtual void OnSubProgressText(const char* text)                                                                                    { MCORE_UNUSED(text); }
        virtual void OnSubProgressValue(float percentage)                                                                                   { MCORE_UNUSED(percentage); }

        virtual void OnScaleActorData(Actor* actor, float scaleFactor)                                                                      { MCORE_UNUSED(actor); MCORE_UNUSED(scaleFactor); }
        virtual void OnScaleMotionData(Motion* motion, float scaleFactor)                                                                   { MCORE_UNUSED(motion); MCORE_UNUSED(scaleFactor); }
    };


    /**
     * The per anim graph instance event handlers.
     * This allows you to capture events triggered on a specific anim graph instance, rather than globally.
     */
    class EMFX_API AnimGraphInstanceEventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphInstanceEventHandler() = default;
        virtual ~AnimGraphInstanceEventHandler() = default;

        /**
         * Event handlers need to overload this function and return the list of events they are interested about
         */
        virtual const AZStd::vector<EventTypes> GetHandledEventTypes() const = 0;

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                 { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                  { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
    };


    /**
     * The per motion instance event handlers.
     * This allows you to capture events triggered on a specific motion instance, rather than globally.
     */
    class EMFX_API MotionInstanceEventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        MotionInstanceEventHandler() = default;
        virtual ~MotionInstanceEventHandler() = default;

        /**
         * Event handlers need to overload this function and return the list of events they are interested about
        */
        virtual const AZStd::vector<EventTypes> GetHandledEventTypes() const = 0;

        void SetMotionInstance(MotionInstance* motionInstance)          { m_motionInstance = motionInstance; }
        MCORE_INLINE MotionInstance* GetMotionInstance()                { return m_motionInstance; }

        /**
         * The method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        virtual void OnEvent(const EventInfo& eventInfo)                { MCORE_UNUSED(eventInfo); }

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param info The playback info used to play the motion.
         */
        virtual void OnStartMotionInstance(PlayBackInfo* info)          { MCORE_UNUSED(info); }

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         */
        virtual void OnDeleteMotionInstance() {}

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         */
        virtual void OnStop() {}

        /**
         * This event gets triggered once a given motion instance has looped.
         */
        virtual void OnHasLooped() {}

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         */
        virtual void OnHasReachedMaxNumLoops() {}

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         */
        virtual void OnHasReachedMaxPlayTime() {}

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         */
        virtual void OnIsFrozenAtLastFrame() {}

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         */
        virtual void OnChangedPauseState() {}

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         */
        virtual void OnChangedActiveState() {}

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        virtual void OnStartBlending() {}

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        virtual void OnStopBlending() {}

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::m_playNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param info The playback information used to play this motion instance.
         */
        virtual void OnQueueMotionInstance(PlayBackInfo* info)      { MCORE_UNUSED(info); }

    protected:
        MotionInstance* m_motionInstance = nullptr;
    };
}   // namespace EMotionFX
