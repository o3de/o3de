/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/Source/EventInfo.h>
#include <EMotionFX/Source/MotionInstancePool.h>

#if defined GetCurrentTime
#undef GetCurrentTime
#endif

namespace EMotionFX
{
    class ActorInstance;
    class MotionInstanceEventHandler;
    class AnimGraphEventBuffer;

    /**
     * The MotionInstance class.
     * Since Motion objects can be shared between different Actors, there needs to be a mechanism which allows this.
     * By introducing this MotionInstance class, we can create instances from Motions, where the instance also contains
     * playback information. This playback information allows us to play the same animation data at different actors
     * all with unique play positions and speeds, etc.
     */
    class EMFX_API MotionInstance
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class RepositioningLayerPass;
        friend class AnimGraphMotionNode;
        friend class MotionInstancePool;

    public:
        struct EMFX_API PlayStateIn
        {
            float m_currentTime = 0.0f;
            float m_duration = 0.0f;
            float m_playSpeed = 0.0f;
            float m_freezeAtTime = -1.0f;
            AZ::u32 m_numLoops = 0;
            AZ::u32 m_maxLoops = EMFX_LOOPFOREVER;
            EPlayMode m_playMode = PLAYMODE_FORWARD;
            bool m_isFrozen = false;
            bool m_freezeAtLastFrame = false;
            bool m_isPaused = false;
        };

        struct EMFX_API PlayStateOut
        {
            float m_currentTime = 0.0f;
            float m_timeDiffToEnd = 0.0f;
            AZ::u32 m_numLoops = 0;
            bool m_isFrozen = false;
            bool m_hasLooped = false;
        };

        static MotionInstance* Create(Motion* motion, ActorInstance* actorInstance);
        static MotionInstance* Create(void* memLocation, Motion* motion, ActorInstance* actorInstance);

        /**
         * Update the motion info.
         * @param timePassed The time passed, in seconds.
         */
        void Update(float timePassed);

        /**
         * Update based on an old and new time value.
         * This will update the motion instance internally as it was previously at oldTime and now has progressed towards newTime.
         * This does not simply change the current time value, but really detects loops, increasing loop counts, triggering events, etc.
         * @param oldTime The previous time value of the current motion time.
         * @param newTime The new current motion time.
         * @param outEventBuffer The output event buffer. This can be nullptr if you want to skip triggering events.
         */
        void UpdateByTimeValues(float oldTime, float newTime, AnimGraphEventBuffer* outEventBuffer);

        void ProcessEvents(float oldTime, float newTime);

        // extract events to be executed
        void ExtractEvents(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer);
        void ExtractEventsNonLoop(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer);

        /**
         * Set the custom data pointer.
         * This can be used to link your own custom data to this MotionInstance object.
         * You are responsible later on for deleting any allocated data by this custom data.
         * You can use the MotionEventHandler::OnDeleteMotionInstance() callback to detect when a motion instance is being deleted.
         * @param customDataPointer The pointer to your custom data.
         */
        void SetCustomData(void* customDataPointer);

        /**
         * Get the custom data pointer value.
         * This can be used to link your own custom data to this MotionInstance object.
         * You are responsible later on for deleting any allocated data by this custom data.
         * You can use the MotionEventHandler::OnDeleteMotionInstance() callback to detect when a motion instance is being deleted.
         * On default the custom data value will return nullptr, unless you have used SetCustomData() to adjust the value.
         * @result The pointer to your custom data that is linked to this MotionInstance object.
         */
        void* GetCustomData() const;

        /**
         * Get the unique identification number for the motion instance.
         * @return The unique identification number.
         */
        AZ::u32 GetID() const;

        /**
         * Get the blend in time.
         * This is the time passed to the SetWeight(...) method where when the target weight is bigger than the current.
         * So only blend ins are counted and not blending out towards for example a weight of 0.
         * When you never call SetWeight(...) yourself, this means that this will contain the value specificied to PlayBackInfo::m_blendInTime
         * at the time of MotionSystem::PlayMotion(...).
         * @result The blend-in time, in seconds.
         */
        float GetBlendInTime() const;

        /**
         * Returns the current time in the playback of the motion.
         * @result The current time, in seconds.
         */
        float GetCurrentTime() const;

        /**
         * Get the maximum time of this motion.
         * @result The maximum time of this motion, in seconds.
         */
        AZ_DEPRECATED(float GetMaxTime() const, "This method has been deprecated. Please use MotionInstance::GetDuration() instead.");

        /**
         * Get the duration of the motion, which is the difference between the clip start and end time.
         * @result The playback duration of this motion instance, in seconds.
         */
        float GetDuration() const;

        /**
         * Return the play speed factor (1.0 is normal, 0.5 is half speed, etc.).
         * @result The play speed factor.
         */
        float GetPlaySpeed() const;

        /**
         * Returns motion it is using.
         * @result The original motion it's using.
         */
        Motion* GetMotion() const;
        void SetMotion(Motion* motion);

        /**
         * Set the current time in the animation (automatic wrapping/looping performed when out of range).
         * Normalized in this case means from 0.0 to 1.0. The maximum time of this animation is then 1.0.
         * @param normalizedTimeValue The new normalized time value, in seconds.
         */
        void SetCurrentTimeNormalized(float normalizedTimeValue);

        /**
         * Returns the current time in the playback of the motion. Normalized in this
         * case means from 0.0 to 1.0. The maximum time of this animation is then 1.0.
         * @result The normalized current time, in seconds.
         */
        float GetCurrentTimeNormalized() const;

        /**
         * Set the current time in the animation (automatic wrapping/looping performed when out of range).
         * @param time The current time in the animation, in seconds.
         * @param resetLastTime When set to true, the last frame time will be set equal to the new current time as well.
         */
        void SetCurrentTime(float time, bool resetLastTime = true);

        void SetLastCurrentTime(float timeInSeconds);

        /**
         * Get the current time of the previous update.
         * @result The time value, in seconds, of the current playback time, in the previous update.
         */
        float GetLastCurrentTime() const;

        /**
         * Set the current play speed (1.0 is normal, 0.5 is half speed, etc.).
         * The speed has to be bigger or equal to 0. You should not use negative playback speeds.
         * If you want to play backward, use the SetPlayMode( PLAYMODE_BACKWARD ), or use the PlayBackInfo::m_playMode value.
         * @param speed The current play speed (1.0 is normal, 0.5 is half speed, etc.).
         */
        void SetPlaySpeed(float speed);

        /**
         * Set the play mode, which defines the direction the motion is playing (forward or backward).
         * @param mode The playback mode to use.
         */
        void SetPlayMode(EPlayMode mode);

        /**
         * Get the play mode, which defines the deireciton the motion is playing (foward or backward).
         * @result The playback mode.
         */
        EPlayMode GetPlayMode() const;

        /**
         * Updates the current play time value.
         * This is automatically called.
         */
        void UpdateTime(float timePassed);

        /**
         * Set the fade-out time.
         * @param fadeTime The fade time, in seconds.
         */
        void SetFadeTime(float fadeTime);

        /**
         * Return the time spend to fade out the motion when it is being stopped automatically
         * when the motion has reached its end. This will only happen when the play mode is PLAYMODE_ONCE.
         * @result The fade time, in seconds.
         */
        float GetFadeTime() const;

        /**
         * Get the motion blending mode of this motion instance.
         * This describes how the motion gets blend with the other motions being played.
         * For more information about what the blendmodes exactly do, read the documentation of the SetBlendMode method.
         * @result The motion blend mode for this motion instance.
         * @see SetBlendMode
         */
        EMotionBlendMode GetBlendMode() const;

        bool GetIsInPlace() const;
        void SetIsInPlace(bool inPlace);

        /**
         * Returns the current weight of the layer.
         * This weight is in range of [0..1], where 0 means no influence and 1 means full influence.
         * @result The current weight value.
         */
        float GetWeight() const;

        /**
         * Returns the target weight. This is the weight we are blending towards.
         * If you specified a blendTimeInSeconds of zero in the SetWeight method, the target weight will return the same
         * as the value returned by GetWeight().
         * The value returned is in range of [0..1].
         * @result The target weight value, where we are blending towards.
         */
        float GetTargetWeight() const;

        /**
         * Set the target weight value.
         * This can be used to smoothly blend towards another weight value.
         * You specify the new (target) weight value, and the time in seconds in which we should blend into that weight.
         * A weight value of 0 means no influence, and a weight value of 1 means full influence.
         * Please keep in mind that motion layers inside the motion layer system will automatically be removed when we are in overwrite motion blend mode
         * and this motion reaches full influence. In order to prevent this from happening, you can blend towards a weight of for example 0.999. This will not
         * have any visual difference compared to a weight of 1, but will prevent motion instances and layers from being removed.
         * The same goes for motion weights of 0. Instead of motion weights of 0, you can use values like 0.001 in these cases.
         * @param targetWeight The weight value we want to blend towards.
         * @param blendTimeInSeconds The time, in seconds, in which we should blend from the current weight value into the specified target weight value.
         */
        void SetWeight(float targetWeight, float blendTimeInSeconds = 0);

        /**
         * Set the motion blend mode of this motion instance.
         * If you want to switch between two different motions, for example going from walk into run, you most likely
         * want to use the BLENDMODE_OVERWRITE mode. However, there are some cases where overwriting doesn't work well.
         * Think of a skateboard game, where you play a looping base animation, which is played while moving forwards.
         * Now on top of this you want to bend the character's upper body to the right. So you play a mixing motion of
         * the upper body. You now have a character, which is skate boarding, while bending his upper body to the right.
         * Now imagine you want to have some motion which shoots. You want the character to shoot, while it is skating and
         * while it is has bend his upper body to the right. If you would use overwrite mode, and the shoot animation adjusts
         * the bones in the upper body, the motion of the bend will be overwritten. This means you will only see a shoot animation
         * while skating, and not the desired, shoot, while bending right, while skating.
         * In order to solve this, you can play the shoot motion additive, on top of the bend and skate motions. EMotion FX will then
         * add all relative changes (compared to the original pose of the actor) to the current result, which in our case is the
         * skating, while bending right.
         * Playing an additive motion in mix-mode, will act the same as playing one in non-mixing mode.
         * @param mode The blendmode to use. The default blendmode of motion instances are set to BLEND_OVERWRITE.
         * @see EMotionBlendMode
         */
        void SetBlendMode(EMotionBlendMode mode);

        /**
         * Enable or disable motion mirroring.
         * On default motion mirroring is disabled.
         * Motion mirroring is often very useful in sport games, where you can choose whether your character is left or right-handed.
         * The motion mirroring feature allows you to create just one set of motions, for example right handed.
         * By enabling mirroring of EMotion FX we can turn the right handed motions into left handed motions on the fly, by using the right handed motion source data
         * and modifying it into a left handed motion. This does not really take more memory. Of course there is a little performance impact, but it is definitely worth
         * the memory savings and art time savings.
         * When mirroring is enabled the motion mirror plane normal is used to determine how to mirror the motion. This can be set with SetMirrorPlaneNormal().
         * @param enabled Set to true if you want to enable motion mirroring, or false when you want to disable it.
         */
        void SetMirrorMotion(bool enabled);

        /**
         * On default motion mirroring is disabled, so set to false.
         * Motion mirroring is often very useful in sport games, where you can choose whether your character is left or right-handed.
         * The motion mirroring feature allows you to create just one set of motions, for example right handed.
         * By enabling mirroring of EMotion FX we can turn the right handed motions into left handed motions on the fly, by using the right handed motion source data
         * and modifying it into a left handed motion. This does not really take more memory. Of course there is a little performance impact, but it is definitely worth
         * the memory savings and art time savings.
         * @result Returns true when motion mirroring is enabled, or false when it is disabled. On default it is disabled.
         */
        bool GetMirrorMotion() const;

        /**
         * Rewinds the motion instance. It sets the current time to 0 seconds.
         */
        void Rewind();

        /**
         * Check if this motion instance has ended or not.
         * This will only happen when the play mode is PLAYMODE_ONCE, because a looping motion will of course never end.
         * @result Returns true when the motion has reached the end of the animation, otherwise false is returned.
         */
        bool GetHasEnded() const;

        /**
         * Set the motion to mix mode or not.
         * @param mixModeEnabled Set to true when the motion has to mix, otherwise set to false.
         */
        void SetMixMode(bool mixModeEnabled);

        /**
         * Checks if the motion is currently stopping or not, so if it is fading out.
         * @result Returns true when the motion is currently stopping, so is fading out.
         */
        bool GetIsStopping() const;

        /**
         * Checks if the motion is currently playing or not.
         * @result Returns true when the motion is playing, otherwise false.
         */
        bool GetIsPlaying() const;

        /**
         * Checks if the motion is in mix mode or not.
         * @result Returns true when the motion is being mixed, otherwise false.
         */
        bool GetIsMixing() const;

        /**
         * Checks if the motion is being blended or not.
         * A blend could be the smooth fade in after starting the motion, or a smooth fade out when stopping the animation.
         * @result Returns true when the motion is currently in a blend process, otherwise false is returned.
         */
        bool GetIsBlending() const;

        /**
         * Pause the motion instance.
         */
        void Pause();

        /**
         * Unpause the motion instance.
         */
        void UnPause();

        /**
         * Set the pause mode.
         * @param pauseEnabled When true, the motion will be set to pause, else it will be unpaused.
         */
        void SetPause(bool pauseEnabled);

        /**
         * Check if the motion currently is paused or not.
         * @result Returns true when the motion is in pause mode.
         */
        bool GetIsPaused() const;

        /**
         * Set the number of loops the motion should play.
         * If you want to loop it forever, set the value to EMFX_LOOPFOREVER (which is defined in Actor.h).
         * @param numLoops The number of loops the motion should play, or EMFX_LOOPFOREVER in case it should play forever.
         */
        void SetMaxLoops(AZ::u32 numLoops);

        /**
         * Get the number of loops the motion will play.
         * @result The number of times the motion will be played. This value will be EMFX_LOOPFOREVER (see Actor.h for the define) in case
         *         the motion will play forever.
         * @see IsPlayingForever
         */
        AZ::u32 GetMaxLoops() const;

        /**
         * Check if the motion has looped since the last update.
         * This is the case when the number of loops returned by GetNumCurrentLoops is not equal to the number of loops before the
         * playback mode object has been updated.
         * @result Returns true when the motion has looped, otherwise false is returned.
         */
        bool GetHasLooped() const;

        /**
         * Set the new number of times the motion has been played. Changing this value will misrepresent the exact number.
         * @param numCurrentLoops The number of times the motion has been played.
         */
        void SetNumCurrentLoops(AZ::u32 numCurrentLoops);

        void SetNumLastLoops(AZ::u32 numCurrentLoops);
        AZ::u32 GetNumLastLoops() const;

        /**
         * Get the number of times the motion currently has been played.
         * @result The number of times the motion has been completely played.
         */
        AZ::u32 GetNumCurrentLoops() const;

        /**
         * Check if the motion will play forever or not.
         * @result Returns true when the motion is looping forever, or false when there will be a moment where it will be stopped.
         */
        bool GetIsPlayingForever() const;

        /**
         * Get the actor instance we are playing this motion instance on.
         * @result A pointer to the actor where this motion instance is playing on.
         */
        ActorInstance* GetActorInstance() const;

        /**
         * Get the priority level of the motion instance.
         * Higher values mean less change on getting overwritten by another motion.
         * A good example are facial motions being played on a walking character.
         * You would first play the walk motion, with priority of say 0.
         * After that you will play a facial motion, with mix mode, and priority level 5 for example.
         * Now you want to change the walk motion into a run motion. If we did not set the priority level for the
         * facial motion above 0, the run motion would overwrite the facial motion. But since the facial motion
         * has got a higher priority, it will not be overwritten by the run motion. If we now want to change the
         * facial motion with another one, we simply play the facial motion with the same or a higher priority level
         * as the previous facial motion. So a priority level of 5 or higher would work in the example case.
         * @result The priority level of the motion instance.
         */
        AZ::u32 GetPriorityLevel() const;

        /**
         * Set the priority level of the motion instance.
         * Higher values mean less change on getting overwritten by another motion.
         * A good example are facial motions being played on a walking character.
         * You would first play the walk motion, with priority of say 0.
         * After that you will play a facial motion, with mix mode, and priority level 5 for example.
         * Now you want to change the walk motion into a run motion. If we did not set the priority level for the
         * facial motion above 0, the run motion would overwrite the facial motion. But since the facial motion
         * has got a higher priority, it will not be overwritten by the run motion. If we now want to change the
         * facial motion with another one, we simply play the facial motion with the same or a higher priority level
         * as the previous facial motion. So a priority level of 5 or higher would work in the example case.
         * @result The priority level of the motion instance.
         */
        void SetPriorityLevel(AZ::u32 priorityLevel);

        /**
         * Check if this motion has motion extraction enabled or not.
         * @result Returns true if motion extraction is enabled, false if not.
         * @see Actor::SetMotionExtractionNode
         */
        bool GetMotionExtractionEnabled() const;

        /**
         * Enable or disable motion extraction.
         * @param enable Set to true when you want to enable motion extraction.
         * @see GetMotionExtractionEnabled
         */
        void SetMotionExtractionEnabled(bool enable);

        /**
         * Check if this motion instance is allowed to overwrite (and thus delete) other motion instances/layers.
         * This happens in the motion layer system when the weight of this motion instance becomes 1, which means it
         * would completely overwrite other motions because the other motions will not have any influence anymore.
         * On default this value is set to true.
         * @result Returns true when the motion instance will automatically delete other motion instances when its weight reaches
         *         a value of 1. If the motion instance will not delete/overwrite any other motion instances, false is returned.
         */
        bool GetCanOverwrite() const;

        /**
         * Enable or disable this motion instance to overwrite and so delete other motion instances.
         * This happens in the motion layer system when the weight of this motion instance becomes 1, which means it
         * would completely overwrite other motions because the other motions will not have any influence anymore.
         * On default overwriting is enabled, so the value would be true.
         * @param canOverwrite Set to true to allow this motion instance to overwrite/delete other motion instances/layers. Otherwise set to false.
         */
        void SetCanOverwrite(bool canOverwrite);

        /**
         * Check if this motion instance can delete itself when its weight equals zero, which means the motion would have no visual influence.
         * On default this value is set to true.
         * @result Returns true when the motion instance will delete itself when its weight equals a value of zero. Otherwise false is returned.
         */
        bool GetDeleteOnZeroWeight() const;

        /**
         * Allow or disallow the motion instance to delete itself when its weight equals zero.
         * When a motion instance has a weight of zero it means it would have no visual influence in the final result.
         * On default deletion on zero weights is enabled, so the value would be true.
         * @param deleteOnZeroWeight Set to true when you wish to allow the motion instance to delete itself when it has a weight of zero. Otherwise set it to false.
         */
        void SetDeleteOnZeroWeight(bool deleteOnZeroWeight);

        /**
         * Stop the motion, using a given fade-out time.
         * This will first modify the fade-out time and then fade to a zero weight.
         * @param fadeOutTime The time it takes, in seconds, to fade out the motion.
         */
        void Stop(float fadeOutTime);

        /**
         * Stop the motion, using the currently set fade-out time.
         * This will blend the weight into zero.
         */
        void Stop();

        /**
         * Check if motion retargeting on this motion instance is enabled or not.
         * @result Returns true when motion retargeting is enabled on this motion instance. Otherwise false is returned.
         */
        bool GetRetargetingEnabled() const;

        /**
         * Enable or disable motion retargeting on this motion instance.
         * Retargeting takes a bit more speed, but it is a very small performance difference.
         * @param enabled Set to true when you wish to enable motion retargeting, otherwise set to false (false is the default).
         */
        void SetRetargetingEnabled(bool enabled);

        /**
         * Check if the motion instance is active or not. On default the motion instance will be active.
         * Inactive motion instances do not get processed at all. They will not update their weight blending or time values.
         * The difference with paused motion instances is that paused instances do process their weight blending, while inactive
         * motion instances do not.
         * @result Returns true when the motion instance is active, otherwise false is returned.
         */
        bool GetIsActive() const;

        /**
         * Activate or deactivate this motion instance. On default the motion instance will be active.
         * Inactive motion instances do not get processed at all. They will not update their weight blending or time values.
         * The difference with paused motion instances is that paused instances do process their weight blending, while inactive
         * motion instances do not.
         * @param enabled Set to true when you want to activate the motion instance, or false when you wish to deactivate it.
         */
        void SetIsActive(bool enabled);

        /**
         * Check if we are frozen in the last frame or not.
         * This would only happen when the SetFreezeAtLastFrame(...) was set to true and when the maximum number of loops
         * has been reached. Instead of fading out the motion, it will remain in its last frame.
         * @result Returns true when the motion is frozen in its last frame.
         */
        bool GetIsFrozen() const;

        /**
         * Set if we are frozen in the last frame or not.
         * Instead of fading out the motion, it will remain in its last frame.
         */
        void SetIsFrozen(bool isFrozen);

        /**
         * Check if motion event processing is enabled for this motion instance.
         * @result Returns true when processing of motion events for this motion event is enabled. Otherwise false is returned.
         */
        bool GetMotionEventsEnabled() const;

        /**
         * Enable or disable processing of motion events for this motion instance.
         * @param enabled Set to true when you wish to enable processing of motion events. Otherwise set it to false.
         */
        void SetMotionEventsEnabled(bool enabled);

        /**
         * Set the motion event weight threshold for this motion instance.
         * The threshold value represents the minimum weight value the motion instance should have in order for it
         * to have its motion events executed.
         * For example if the motion event threshold is set to 0.3, and the motion instance weight value is 0.1, then
         * no motion events will be processed. If however the weight value of the motion instance is above or equal to this 0.3 value
         * then all events will be processed. On default the value is 0.0, which means that all events will be processed.
         * @param weightThreshold The motion event weight threshold. If the motion instance weight is below this value, no motion
         *                        events will be processed for this motion instance. Basically this value should be in range of [0..1].
         */
        void SetEventWeightThreshold(float weightThreshold);

        /**
         * Get the motion event weight threshold for this motion instance.
         * The threshold value represents the minimum weight value the motion instance should have in order for it
         * to have its motion events executed.
         * For example if the motion event threshold is set to 0.3, and the motion instance weight value is 0.1, then
         * no motion events will be processed. If however the weight value of the motion instance is above or equal to this 0.3 value
         * then all events will be processed. On default the value is 0.0, which means that all events will be processed.
         * @result The motion event threshold value.
         */
        float GetEventWeightThreshold() const;

        /**
         * Check if this motion instance will freeze at its last frame (when not using looping).
         * This only happens when using a maximum play time, or a given number of loops that is not infinite.
         * @result Returns true when the motion instance will freeze at the last frame. Otherwise false is returned.
         */
        bool GetFreezeAtLastFrame() const;

        /**
         * This option allows you to specify whether a motion should start fading out before the motion has ended (before it reached
         * its maximum number of loops) or if it should fade out after the maximum number of loops have been reached. In the latter case
         * it would mean that the motion will start another loop while it starts fading out. On default this option is enabled, which means
         * the motion will be faded out exactly when it reaches the last frame it should play.
         * @param enabled Set to true to to enable this option, or false to disable it. On default it is set to true.
         */
        void SetBlendOutBeforeEnded(bool enabled);

        /**
         * The "blend out before end"-option allows you to specify whether a motion should start fading out before the motion has ended (before it reached
         * its maximum number of loops) or if it should fade out after the maximum number of loops have been reached. In the latter case
         * it would mean that the motion will start another loop while it starts fading out. On default this option is enabled, which means
         * the motion will be faded out exactly when it reaches the last frame it should play.
         * @result Returns true when this option is enabled, otherwise false is returned.
         */
        bool GetBlendOutBeforeEnded() const;

        /**
         * Enable or disable freezing at the last frame.
         * This only happens when using a maximum play time, or a given number of loops that is not infinite.
         * When you play a full body death motion, you probably want to enable this.
         * If in that case this option would be disabled (default) it will blend back into its base/bind pose instead of freezing
         * at the last frame, keeping the character dead on the ground.
         * @param enabled Set to true when you want to enable the freeze at last frame option, otherwise set to false.
         */
        void SetFreezeAtLastFrame(bool enabled);

        /**
         * Get the total time this motion has been playing already.
         * @result The total time, in seconds, that this motion is already playing.
         */
        float GetTotalPlayTime() const;

        /**
         * Adjust the total play time that this motion is already playing.
         * @param playTime The total play time, in seconds.
         */
        void SetTotalPlayTime(float playTime);

        /**
         * Get the maximum play time of this motion instance (well actually of the motion where this is an instance from).
         * So this returns the duration of the motion, in seconds.
         * When this value is zero or a negative value, then the maximum play time option has been disabled for this motion instance.
         * @result The duration of the motion, in seconds.
         */
        float GetMaxPlayTime() const;

        /**
         * Set the maximum play time, in seconds, that this motion instance is allowed to play.
         * When you set this to zero (default), or a negative value, then this option is disabled.
         * @param playTime The maximum play time, in seconds, or zero or a negative value to disable the maximum play time.
         */
        void SetMaxPlayTime(float playTime);

        /**
         * Initialize the motion instance from PlayBackInfo settings.
         * @param info The playback info settings to initialize from.
         * @param resetCurrentPlaytime Set back the current playtime, even though this is not an attribute of the playback info in case of true. In case of false the current time won't be modified.
         */
        void InitFromPlayBackInfo(const PlayBackInfo& info, bool resetCurrentPlaytime = true);

        /**
         * Get the time difference between the current play time and the end of the motion.
         * With end of motion we mean the point where there is no more motion data, so where it would need to loop to continue to play.
         * @result The time remaining until the loop point of the motion would be reached.
         */
        float GetTimeDifToLoopPoint() const;

        /**
         * Add an event handler to this motion instance.
         * After adding, the event handler will receive events.
         * @param eventHandler The new event handler to register.
         */
        void AddEventHandler(MotionInstanceEventHandler* eventHandler);

        /**
         * Remove the given event handler.
         * Even if the function returns false, if delFromMem is set to true it will delete the handler from memory.
         * @param eventHandler A pointer to the event handler to remove.
         */
        void RemoveEventHandler(MotionInstanceEventHandler* eventHandler);

        /**
         * Remove all motion event handlers from this motion instance.
         */
        void RemoveAllEventHandlers();

        /**
         * Get the event handler at the given index.
         * @result A pointer to the event handler at the given index.
         */
        MotionInstanceEventHandler* GetEventHandler(size_t index) const;

        /**
         * Get the number of event handlers.
         * @result The number of event handlers assigned to the motion instance.
         */
        size_t GetNumEventHandlers() const;

        //--------------------------------

        /**
         * The method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        void OnEvent(const EventInfo& eventInfo) const;

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param info The playback info used to play the motion.
         */
        void OnStartMotionInstance(PlayBackInfo* info);

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         */
        void OnDeleteMotionInstance();

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         */
        void OnStop();

        /**
         * This event gets triggered once a given motion instance has looped.
         */
        void OnHasLooped();

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         */
        void OnHasReachedMaxNumLoops();

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         */
        void OnHasReachedMaxPlayTime();

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         */
        void OnIsFrozenAtLastFrame();

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         */
        void OnChangedPauseState();

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         */
        void OnChangedActiveState();

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        void OnStartBlending();

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        void OnStopBlending();

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::m_playNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param info The playback information used to play this motion instance.
         */
        void OnQueueMotionInstance(PlayBackInfo* info);

        /**
         * Marks the object as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        float GetFreezeAtTime() const;
        void SetFreezeAtTime(float timeInSeconds);

        void CalcRelativeTransform(Node* rootNode, float curTime, float oldTime, Transform* outTransform) const;
        bool ExtractMotion(Transform& outTrajectoryDelta);
        void CalcGlobalTransform(const AZStd::vector<size_t>& hierarchyPath, float timeValue, Transform* outTransform) const;
        void ResetTimes();

        AZ_DEPRECATED(void CalcNewTimeAfterUpdate(float timePassed, float* outNewTime) const, "MotionInstance::CalcNewTimeAfterUpdate has been deprecated, please use MotionInstance::CalcPlayStateAfterUpdate(timeDelta).m_currentTime instead.");
        PlayStateOut CalcPlayStateAfterUpdate(float timePassed) const;

        PlayStateIn ConstructInputPlayState() const;
        void SetPlayState(const PlayStateIn& inState, const PlayStateOut& outState, bool triggerEvents);
        static PlayStateOut CalcPlayState(const PlayStateIn& inState, float timePassed);
        void ExtractMotionEvents(const PlayStateIn& inState, PlayStateOut& outState, AnimGraphEventBuffer& eventBuffer) const;
        void ProcessMotionEvents(const PlayStateIn& inState, PlayStateOut& outState) const;

        MotionInstancePool::SubPool* GetSubPool() const;

        const MotionLinkData* FindMotionLinkData(const Actor* actor) const;

    private:
        /**
         * Instead of storing a bunch of booleans we use bits.
         */
        enum
        {
            BOOL_ISPAUSED               = 1 << 0,   /**< Is the motion paused? */
            BOOL_ISSTOPPING             = 1 << 1,   /**< Are we stopping and fading out? */
            BOOL_ISBLENDING             = 1 << 2,   /**< Are we blending in or out? (changing weight with time interval). */
            BOOL_ISMIXING               = 1 << 3,   /**< Is this motion a mixing motion? */
            BOOL_USEMOTIONEXTRACTION    = 1 << 4,   /**< Use motion extraction?  */
            BOOL_CANOVERWRITE           = 1 << 5,   /**< Can this motion instance overwrite and remove other motion instances? */
            BOOL_DELETEONZEROWEIGHT     = 1 << 6,   /**< Will this motion instance be deleted when it reaches a weight of zero? */
            BOOL_RETARGET               = 1 << 7,   /**< Is retargeting enabled? */
            BOOL_FREEZEATLASTFRAME      = 1 << 8,   /**< Should we freeze at the last frame? */
            BOOL_ENABLEMOTIONEVENTS     = 1 << 9,   /**< Enable motion events for this motion instance? */
            BOOL_ISACTIVE               = 1 << 10,  /**< Is the motion instance active? */
            BOOL_ISFIRSTREPOSUPDATE     = 1 << 11,  /**< Is this the first time the repositioning is being updated for this motion instance? */
            BOOL_ISFROZENATLASTFRAME    = 1 << 12,  /**< Is the motion in a frozen state? */
            BOOL_BLENDBEFOREENDED       = 1 << 13,  /**< Start blending out before the motion has ended, so that it exactly is faded out when the motion is in its last frame? */
            BOOL_MIRRORMOTION           = 1 << 14,  /**< Mirror the motion? */

#if defined(EMFX_DEVELOPMENT_BUILD)
            BOOL_ISOWNEDBYRUNTIME       = 1 << 15,  /**< Is motion owned by the engine runtime? */
#endif // EMFX_DEVELOPMENT_BUILD

            BOOL_INPLACE                = 1 << 16   /**< Is in place animation enabled? */
        };

        /**
         * The constructor.
         * @param motion The motion from which this is an instance.
         * @param actorInstance The actor instance we are playing this motion instance on.
         */
        MotionInstance(Motion* motion, ActorInstance* actorInstance);
        ~MotionInstance();

        void SetSubPool(MotionInstancePool::SubPool* subPool);
        void EnableFlag(AZ::u32 flag);
        void DisableFlag(AZ::u32 flag);
        void SetFlag(AZ::u32 flag, bool enabled);

    private:
        using EventHandlerVector = AZStd::vector<MotionInstanceEventHandler*>;

        AZStd::vector<EventHandlerVector> m_eventHandlersByEventType; /**< The event handler to use to process events organized by EventTypes. */
        float               m_currentTime = 0.0f;           /**< The current playtime. */
        float               m_timeDiffToEnd = 0.0f;         /**< The time it takes until we reach the loop point in the motion. This also takes the playback direction into account (backward or forward play). */
        float               m_freezeAtTime = -1.0f;         /**< Freeze at a given time offset in seconds. The current play time would continue running though, and a blend out would be triggered, unlike the m_freezeAtLastFrame. Set to negative value to disable. Default=-1.*/
        float               m_playSpeed = 1.0f;             /**< The playspeed (1.0=normal speed). */
        float               m_lastCurTime = 0.0f;           /**< The last current time, so the current time in the previous update. */
        float               m_totalPlayTime = 0.0f;         /**< The current total play time that this motion is already playing. */
        float               m_maxPlayTime = 0.0f;           /**< The maximum play time of the motion. If the m_totalPlayTime is higher than this, the motion will be stopped, unless the max play time is zero or negative. */
        float               m_eventWeightThreshold = 0.0f;  /**< If the weight of the motion instance is below this value, the events won't get processed (default = 0.0f). */
        float               m_weight = 0.0f;                /**< The current weight value, in range of [0..1]. */
        float               m_weightDelta = 0.0f;           /**< The precalculated weight delta value, used during blending between weights. */
        float               m_targetWeight = 1.0f;          /**< The target weight of the layer, when activating the motion. */
        float               m_blendInTime = 0.0f;           /**< The blend in time. */
        float               m_fadeTime = 0.3f;              /**< Fadeout speed, when playing the animation once. So when it is done playing once, it will fade out in 'm_fadeTime' seconds. */
        AZ::u32             m_curLoops = 0;                 /**< Number of loops it currently has made (so the number of times the motion played already). */
        AZ::u32             m_maxLoops = EMFX_LOOPFOREVER;  /**< The maximum number of loops, before it has to stop. */
        AZ::u32             m_lastLoops = 0;                /**< The current number of loops in the previous update. */
        AZ::u32             m_priorityLevel = 0;            /**< The priority level, where higher values mean higher priority. */
        AZ::u32             m_id = InvalidIndex32;          /**< The unique identification number for the motion instance. */
        Motion*             m_motion = nullptr;             /**< The motion that this motion instance is using the keyframing data from. */
        ActorInstance*      m_actorInstance = nullptr;      /**< The actor instance where we are playing this motion instance on. */
        void*               m_customData = nullptr;         /**< The custom data pointer, which is nullptr on default. */
        EMotionBlendMode    m_blendMode = BLENDMODE_OVERWRITE; /**< The motion blend mode [default=BLENDMODE_OVERWRITE]. */
        AZ::u32             m_boolFlags = 0;                /**< The boolean flags mask. */
        EPlayMode           m_playMode = PLAYMODE_FORWARD;  /**< The motion playback mode [default=PLAYMODE_FORWARD]. */
        MotionInstancePool::SubPool* m_subPool = nullptr;   /**< The subpool this motion instance is part of, or nullptr when it isn't part of any subpool. */
    };
} // namespace EMotionFX
