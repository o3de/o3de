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
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/MultiThreadManager.h>
#include "MemoryCategories.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphNode.h"
#include "BlendTreeParameterNode.h"
#include "AnimGraphStateTransition.h"
#include "EventInfo.h"

#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>


namespace EMotionFX
{
    // forward declarations
    class EventHandler;
    class EventData;

    /**
     * Intersection information, used by the event system, to return the results of an intersection test.
     */
    struct EMFX_API IntersectionInfo
    {
        AZ::Vector3     m_position;
        AZ::Vector3     m_normal;
        AZ::Vector2     m_uv;
        float           m_baryCentricU;
        float           m_baryCentricV;
        ActorInstance*  m_actorInstance;
        ActorInstance*  m_ignoreActorInstance;
        Node*           m_node;
        Mesh*           m_mesh;
        uint32          m_startIndex;
        bool            m_isValid;

        IntersectionInfo()
        {
            m_position = AZ::Vector3::CreateZero();
            m_normal.Set(0.0f, 1.0f, 0.0f);
            m_uv = AZ::Vector2::CreateZero();
            m_baryCentricU   = 0.0f;
            m_baryCentricV   = 0.0f;
            m_actorInstance  = nullptr;
            m_startIndex     = 0;
            m_ignoreActorInstance = nullptr;
            m_node           = nullptr;
            m_mesh           = nullptr;
            m_isValid        = false;
        }
    };


    /**
     * The event manager, which is used to specify which event handler to use.
     * You can use the macro named GetEventManager() to quickly access this singleton class.
     * If you want to override the way events are processed, you have to create your own
     * class inherited from the EventHandler base class.
     * @see EventHandler.
     * @see MotionEvent.
     * @see MotionEventTable.
     */
    class EMFX_API EventManager
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class Initializer;
        friend class EMotionFXManager;

    public:
        static EventManager* Create();

        void Lock();
        void Unlock();

        /**
         * Add event handler to the manager.
         * @param eventHandler The new event handler to register, this must not be nullptr.
         */
        void AddEventHandler(EventHandler* eventHandler);

        /**
         * Remove the given event handler.
         * @param eventHandler A pointer to the event handler to remove.
         */
        void RemoveEventHandler(EventHandler* eventHandler);

        //---------------------------------------------------------------------
        template<typename T, typename ... Args>
        AZStd::shared_ptr<const T> FindOrCreateEventData(Args&& ... args)
        {
            AZStd::shared_ptr<const T> instance = AZStd::make_shared<const T>(args ...);
            return FindEventData(instance);
        }

        template<typename T>
        AZStd::shared_ptr<const T> FindEventData(const AZStd::shared_ptr<const T>& data)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_eventDataLock);
            for (AZStd::weak_ptr<const EMotionFX::EventData> entry : m_allEventData)
            {
                if (entry.expired())
                {
                    continue;
                }

                const AZStd::shared_ptr<const T> typedEntry = AZStd::rtti_pointer_cast<const T>(entry.lock());
                if (typedEntry && (*data.get() == *typedEntry.get()))
                {
                    return typedEntry;
                }
            }

            m_allEventData.emplace_back(data);
            return data;
        }

        //---------------------------------------------------------------------

        /**
         * The main method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        void OnEvent(const EventInfo& eventInfo);

        /**
         * The event that gets triggered when a MotionSystem::PlayMotion(...) is being executed.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motion The motion that is being played.
         * @param info The playback info used to play the motion.
         */
        void OnPlayMotion(Motion* motion, PlayBackInfo* info);

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
        void OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info);

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         * @param motionInstance The motion instance that is being deleted.
         */
        void OnDeleteMotionInstance(MotionInstance* motionInstance);

        /**
         * The event that gets triggered once a Motion object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Motion object.
         * You can get and set this data object with the Motion::GetCustomData() and Motion::SetCustomData(...) methods.
         * @param motion The motion that is being deleted.
         */
        void OnDeleteMotion(Motion* motion);

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         * @param motionInstance The motion instance that is being stopped.
         */
        void OnStop(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has looped.
         * @param motionInstance The motion instance that got looped.
         */
        void OnHasLooped(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         * @param motionInstance The motion instance that reached its maximum number of allowed loops.
         */
        void OnHasReachedMaxNumLoops(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         * @param motionInstance The motion instance that reached its maximum playback time.
         */
        void OnHasReachedMaxPlayTime(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         * @param motionInstance The motion instance that got into a frozen state.
         */
        void OnIsFrozenAtLastFrame(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         * @param motionInstance The motion instance that changed its paused state.
         */
        void OnChangedPauseState(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         * @param motionInstance The motion instance that changed its active state.
         */
        void OnChangedActiveState(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance which is changing its weight value over time.
         */
        void OnStartBlending(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance for which the automatic weight blending just stopped.
         */
        void OnStopBlending(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::m_playNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param motionInstance The motion instance that gets added to the motion queue.
         * @param info The playback information used to play this motion instance.
         */
        void OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info);

        //---------------------------------------------------------------------

        /**
         * The event that gets triggered once an Actor object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Actor object.
         * You can get and set this data object with the Actor::GetCustomData() and Actor::SetCustomData(...) methods.
         * @param actor The actor that is being deleted.
         */
        void OnDeleteActor(Actor* actor);

        void OnSimulatePhysics(float timeDelta);
        void OnCustomEvent(uint32 eventType, void* data);
        void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color);
        void OnDrawTriangles();

        void OnScaleActorData(Actor* actor, float scaleFactor);
        void OnScaleMotionData(Motion* motion, float scaleFactor);

        /**
         * Perform a ray intersection test and return the intersection info.
         * The first event handler registered that sets the IntersectionInfo::m_isValid to true will be outputting to the outIntersectInfo parameter.
         * @param start The start point, in world space.
         * @param end The end point, in world space.
         * @param outIntersectInfo The resulting intersection info.
         * @result Returns true when an intersection occurred and false when no intersection occurred.
         */
        bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo);

        void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition);
        void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition);
        void OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset);

        void OnInputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue);
        void OnOutputPortsChanged(AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue);

        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName);
        void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node);
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove);
        void OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode);

        void OnProgressStart();
        void OnProgressEnd();
        void OnProgressText(const char* text);
        void OnProgressValue(float percentage);
        void OnSubProgressText(const char* text);
        void OnSubProgressValue(float percentage);

        void OnCreateAnimGraph(AnimGraph* animGraph);
        void OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void OnCreateMotion(Motion* motion);
        void OnCreateMotionSet(MotionSet* motionSet);
        void OnCreateMotionInstance(MotionInstance* motionInstance);
        void OnCreateMotionSystem(MotionSystem* motionSystem);
        void OnCreateActor(Actor* actor);
        void OnPostCreateActor(Actor* actor);

        // delete callbacks
        void OnDeleteAnimGraph(AnimGraph* animGraph);
        void OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void OnDeleteMotionSet(MotionSet* motionSet);
        void OnDeleteMotionSystem(MotionSystem* motionSystem);


    private:
        /**
         * The structure that links a given event type name with a unique ID.
         * The unique ID is passed to the ProcessEvent function to eliminate string compares to check what type of
         * event has been triggered.
         */
        struct EMFX_API RegisteredEventType
        {
            AZStd::string   m_eventType;                             /**< The string that describes the event, this is what artists type in 3DSMax/Maya. */
            uint32          m_eventId;                               /**< The unique ID for this event. */
        };

        using EventHandlerVector = AZStd::vector<EventHandler*>;
        AZStd::vector<EventHandlerVector> m_eventHandlersByEventType; /**< The event handler to use to process events organized by EventTypes. */

        MCore::MutexRecursive               m_lock;
        AZStd::mutex m_eventDataLock;

        AZStd::vector<AZStd::weak_ptr<const EMotionFX::EventData> > m_allEventData;

        /**
         * The constructor, which will initialize the manager to use the default dummy event handler.
         */
        EventManager();

        /**
         * The destructor, which deletes the event handler automatically.
         */
        ~EventManager();
    };
} // namespace EMotionFX
